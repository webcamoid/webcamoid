/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#include <limits>
#include <qrgb.h>
#include <QFileInfo>
#include <QLibrary>
#include <QMutex>
#include <QSharedPointer>
#include <QSize>
#include <QTemporaryFile>
#include <QThreadPool>
#include <QVector>
#include <QtMath>
#include <akfrac.h>
#include <akcaps.h>
#include <akpacket.h>
#include <media/NdkMediaMuxer.h>

#include "mediawriterndkmedia.h"
#include "audiostream.h"
#include "videostream.h"

class OutputFormatsInfo;
using OutputFormatsInfoVector = QVector<OutputFormatsInfo>;
using SupportedCodecsType = QMap<QString, QMap<QString, QStringList>>;

class OutputFormatsInfo
{
    public:
        QString mimeType;
        OutputFormat format;
        QString description;
        QStringList fileExtensions;

        inline static const OutputFormatsInfoVector &info();
        inline static const OutputFormatsInfo *byMimeType(const QString &mimeType);
        inline static const OutputFormatsInfo *byFormat(OutputFormat format);
};

class CodecsInfo;
using CodecsInfoVector = QVector<CodecsInfo>;

class CodecsInfo
{
    public:
        QString mimeType;
        QString description;

        inline static const CodecsInfoVector &info();
        inline static const CodecsInfo *byMimeType(const QString &mimeType);
};

class MediaWriterNDKMediaPrivate
{
    public:
        MediaWriterNDKMedia *self;
        QString m_outputFormat;
        QFile m_outputFile;
        QList<QVariantMap> m_streamConfigs;
        AMediaMuxer *m_mediaMuxer {nullptr};
        QThreadPool m_threadPool;
        qint64 m_maxPacketQueueSize {15 * 1024 * 1024};
        QMutex m_packetMutex;
        QMutex m_audioMutex;
        QMutex m_videoMutex;
        QMutex m_subtitleMutex;
        QMutex m_writeMutex;
        QMutex m_startMuxingMutex;
        QMap<int, AbstractStreamPtr> m_streamsMap;
        SupportedCodecsType m_supportedCodecs;
        bool m_isRecording {false};
        bool m_muxingStarted {false};

        explicit MediaWriterNDKMediaPrivate(MediaWriterNDKMedia *self);
        QString guessFormat();
        static const QStringList &availableCodecs();
        static const SupportedCodecsType &supportedCodecs();
};

MediaWriterNDKMedia::MediaWriterNDKMedia(QObject *parent):
    MediaWriter(parent)
{
    this->d = new MediaWriterNDKMediaPrivate(this);
    this->d->m_supportedCodecs = MediaWriterNDKMediaPrivate::supportedCodecs();
}

MediaWriterNDKMedia::~MediaWriterNDKMedia()
{
    this->uninit();
    delete this->d;
}

QString MediaWriterNDKMedia::defaultFormat()
{
    if (this->d->m_supportedCodecs.isEmpty())
        return {};

    if (this->d->m_supportedCodecs.contains("video/webm"))
        return QStringLiteral("video/webm");

    return this->d->m_supportedCodecs.firstKey();
}

QString MediaWriterNDKMedia::outputFormat() const
{
    return this->d->m_outputFormat;
}

QVariantList MediaWriterNDKMedia::streams() const
{
    QVariantList streams;

    for (auto &stream: this->d->m_streamConfigs)
        streams << stream;

    return streams;
}

qint64 MediaWriterNDKMedia::maxPacketQueueSize() const
{
    return this->d->m_maxPacketQueueSize;
}

QStringList MediaWriterNDKMedia::supportedFormats()
{
    QStringList formats;

    for (auto it = this->d->m_supportedCodecs.constBegin();
         it != this->d->m_supportedCodecs.constEnd();
         it++)
        if (!this->m_formatsBlackList.contains(it.key()))
            formats << it.key();

    std::sort(formats.begin(), formats.end());

    return formats;
}

QStringList MediaWriterNDKMedia::fileExtensions(const QString &format)
{
    auto info = OutputFormatsInfo::byMimeType(format);

    if (!info)
        return {};

    return info->fileExtensions;
}

QString MediaWriterNDKMedia::formatDescription(const QString &format)
{
    auto info = OutputFormatsInfo::byMimeType(format);

    if (!info)
        return {};

    return info->description;
}

QVariantList MediaWriterNDKMedia::formatOptions()
{
    return {};
}

QStringList MediaWriterNDKMedia::supportedCodecs(const QString &format)
{
    return this->supportedCodecs(format, "");
}

QStringList MediaWriterNDKMedia::supportedCodecs(const QString &format,
                                                 const QString &type)
{
    QStringList supportedCodecs;

    if (type.isEmpty()) {
        for (auto &codecs: this->d->m_supportedCodecs.value(format)) {
            for (auto &codec: codecs)
                if (!this->m_codecsBlackList.contains(codec))
                    supportedCodecs << codec;
        }
    } else {
        auto codecs = this->d->m_supportedCodecs.value(format).value(type);

        for (auto &codec: codecs)
            if (!this->m_codecsBlackList.contains(codec))
                supportedCodecs << codec;
    }

    std::sort(supportedCodecs.begin(), supportedCodecs.end());

    return supportedCodecs;
}

QString MediaWriterNDKMedia::defaultCodec(const QString &format,
                                          const QString &type)
{
    auto codecs = this->d->m_supportedCodecs.value(format).value(type);

    for (auto &codec: codecs)
        if (!this->m_codecsBlackList.contains(codec))
            return codec;

    return {};
}

QString MediaWriterNDKMedia::codecDescription(const QString &codec)
{
    auto info = CodecsInfo::byMimeType(codec);

    if (!info)
        return {};

    return info->description;
}

QString MediaWriterNDKMedia::codecType(const QString &codec)
{
    auto info = CodecsInfo::byMimeType(codec);

    if (!info)
        return {};

    return info->mimeType.startsWith("audio/")?
                QStringLiteral("audio/x-raw"):
                QStringLiteral("video/x-raw");
}

QVariantMap MediaWriterNDKMedia::defaultCodecParams(const QString &codec)
{
    auto info = CodecsInfo::byMimeType(codec);

    if (!info)
        return {};

    QVariantMap codecParams;

    if (info->mimeType.startsWith("audio/")) {
        static const QStringList supportedSampleFormats {"s16"};

        codecParams["supportedSampleFormats"] = supportedSampleFormats;
        codecParams["supportedSampleRates"] = QVariantList();
        codecParams["supportedChannelLayouts"] = QVariantList();
        codecParams["defaultSampleFormat"] = supportedSampleFormats.first();
        codecParams["defaultBitRate"] = 128000;
        codecParams["defaultSampleRate"] = 44100;
        codecParams["defaultChannelLayout"] = "stereo";
        codecParams["defaultChannels"] = 2;
    } else {
        static const QStringList supportedPixelFormats {"yuv420p", "nv12"};

        codecParams["supportedPixelFormats"] = supportedPixelFormats;
        codecParams["supportedFrameRates"] = QVariantList();
        codecParams["defaultGOP"] = 12;
        codecParams["defaultBitRate"] = 1500000;
        codecParams["defaultPixelFormat"] = supportedPixelFormats.first();
    }

    return codecParams;
}

QVariantMap MediaWriterNDKMedia::addStream(int streamIndex,
                                           const AkCaps &streamCaps)
{
    return this->addStream(streamIndex, streamCaps, {});
}

QVariantMap MediaWriterNDKMedia::addStream(int streamIndex,
                                           const AkCaps &streamCaps,
                                           const QVariantMap &codecParams)
{
    auto outputFormat = this->d->guessFormat();

    if (outputFormat.isEmpty())
        return {};

    QVariantMap outputParams;

    if (codecParams.contains("label"))
        outputParams["label"] = codecParams["label"];

    outputParams["index"] = streamIndex;
    auto codec = codecParams.value("codec").toString();

    if (codec.isEmpty())
        return {};

    auto supportedCodecs = this->supportedCodecs(outputFormat, streamCaps.mimeType());

    if (codec.isEmpty() || !supportedCodecs.contains(codec))
        codec = this->defaultCodec(outputFormat, streamCaps.mimeType());

    outputParams["codec"] = codec;
    outputParams["caps"] = QVariant::fromValue(streamCaps);

    auto defaultCodecParams = this->defaultCodecParams(codec);

    if (streamCaps.mimeType() == "audio/x-raw"
        || streamCaps.mimeType() == "video/x-raw") {
        int bitrate = codecParams.value("bitrate").toInt();

        if (bitrate < 1)
            bitrate = defaultCodecParams["defaultBitRate"].toInt();

        outputParams["bitrate"] = bitrate;
    }

    if (streamCaps.mimeType() == "video/x-raw") {
        int gop = codecParams.value("gop").toInt();

        if (gop < 1)
            gop = defaultCodecParams["defaultGOP"].toInt();

        outputParams["gop"] = gop;
    }

    this->d->m_streamConfigs << outputParams;
    emit this->streamsChanged(this->streams());

    return outputParams;
}

QVariantMap MediaWriterNDKMedia::updateStream(int index)
{
    return this->updateStream(index, {});
}

QVariantMap MediaWriterNDKMedia::updateStream(int index,
                                              const QVariantMap &codecParams)
{
    auto outputFormat = this->d->guessFormat();

    if (outputFormat.isEmpty())
        return {};

    bool streamChanged = false;

    if (codecParams.contains("label")
        && this->d->m_streamConfigs[index]["label"] != codecParams.value("label")) {
        this->d->m_streamConfigs[index]["label"] = codecParams.value("label");
        streamChanged = true;
    }

    auto streamCaps = this->d->m_streamConfigs[index]["caps"].value<AkCaps>();

    if (codecParams.contains("caps")
        && this->d->m_streamConfigs[index]["caps"] != codecParams.value("caps")) {
        this->d->m_streamConfigs[index]["caps"] = codecParams.value("caps");
        streamChanged = true;
    }

    QString codec;

    if (codecParams.contains("codec")) {
        if (this->supportedCodecs(outputFormat, streamCaps.mimeType())
            .contains(codecParams["codec"].toString())) {
            codec = codecParams["codec"].toString();
        } else
            codec = this->defaultCodec(outputFormat, streamCaps.mimeType());

        this->d->m_streamConfigs[index]["codec"] = codec;
        streamChanged = true;
    } else
        codec = this->d->m_streamConfigs[index]["codec"].toString();

    auto codecDefaults = this->defaultCodecParams(codec);

    if ((streamCaps.mimeType() == "audio/x-raw"
         || streamCaps.mimeType() == "video/x-raw")
        && codecParams.contains("bitrate")) {
        int bitRate = codecParams["bitrate"].toInt();
        this->d->m_streamConfigs[index]["bitrate"] =
                bitRate > 0? bitRate: codecDefaults["defaultBitRate"].toInt();
        streamChanged = true;
    }

    if (streamCaps.mimeType() == "video/x-raw"
        && codecParams.contains("gop")) {
        int gop = codecParams["gop"].toInt();
        this->d->m_streamConfigs[index]["gop"] =
                gop > 0? gop: codecDefaults["defaultGOP"].toInt();
        streamChanged = true;
    }

    if (streamChanged)
        emit this->streamsChanged(this->streams());

    return this->d->m_streamConfigs[index];
}

QVariantList MediaWriterNDKMedia::codecOptions(int index)
{
    Q_UNUSED(index)

    return {};
}

MediaWriterNDKMediaPrivate::MediaWriterNDKMediaPrivate(MediaWriterNDKMedia *self):
    self(self)
{
}

QString MediaWriterNDKMediaPrivate::guessFormat()
{
    if (self->supportedFormats().contains(this->m_outputFormat))
        return this->m_outputFormat;
    else {
        auto extension = QFileInfo(self->location()).completeSuffix();

        for (auto &info: OutputFormatsInfo::info())
            if (info.fileExtensions.contains(extension))
                return info.mimeType;
    }

    return {};
}

const QStringList &MediaWriterNDKMediaPrivate::availableCodecs()
{
    auto &codecsInfo = CodecsInfo::info();
    static QStringList availableCodecs;

    if (!availableCodecs.isEmpty())
        return availableCodecs;

    for (auto it = codecsInfo.constBegin(); it != codecsInfo.constEnd(); it++)
        if (auto codec = AMediaCodec_createEncoderByType(it->mimeType.toStdString().c_str())) {
            availableCodecs << it->mimeType;
            AMediaCodec_delete(codec);
        }

    return availableCodecs;
}

const SupportedCodecsType &MediaWriterNDKMediaPrivate::supportedCodecs()
{
    static SupportedCodecsType supportedCodecs;

    if (!supportedCodecs.isEmpty())
        return supportedCodecs;

    auto &codecs = MediaWriterNDKMediaPrivate::availableCodecs();

    auto audioMediaFormat = AMediaFormat_new();
    AMediaFormat_setInt32(audioMediaFormat,
                          AMEDIAFORMAT_KEY_BIT_RATE,
                          128000);
#if __ANDROID_API__ >= 28
    AMediaFormat_setInt32(audioMediaFormat,
                          AMEDIAFORMAT_KEY_PCM_ENCODING,
                          0x2); // s16
#endif
    AMediaFormat_setInt32(audioMediaFormat,
                          AMEDIAFORMAT_KEY_CHANNEL_MASK,
                          0x4 | 0x8); // stereo
    AMediaFormat_setInt32(audioMediaFormat,
                          AMEDIAFORMAT_KEY_CHANNEL_COUNT,
                          2);
    AMediaFormat_setInt32(audioMediaFormat,
                          AMEDIAFORMAT_KEY_SAMPLE_RATE,
                          44100);

    auto videoMediaFormat = AMediaFormat_new();
    AMediaFormat_setInt32(videoMediaFormat,
                          AMEDIAFORMAT_KEY_BIT_RATE,
                          1500000);
    AMediaFormat_setInt32(videoMediaFormat,
                          AMEDIAFORMAT_KEY_COLOR_FORMAT,
                          19); // yuv420p
    AMediaFormat_setInt32(videoMediaFormat,
                          AMEDIAFORMAT_KEY_WIDTH,
                          640);
    AMediaFormat_setInt32(videoMediaFormat,
                          AMEDIAFORMAT_KEY_HEIGHT,
                          480);
    AMediaFormat_setInt32(videoMediaFormat,
                          AMEDIAFORMAT_KEY_FRAME_RATE,
                          30);
    AMediaFormat_setInt32(videoMediaFormat,
                          AMEDIAFORMAT_KEY_I_FRAME_INTERVAL,
                          1);

    auto &formatsInfo = OutputFormatsInfo::info();

    for (auto &format: formatsInfo) {
        for (auto codec: codecs) {
            QTemporaryFile tempFile;
            tempFile.setAutoRemove(true);

            if (tempFile.open()) {
                if (auto muxer = AMediaMuxer_new(tempFile.handle(), format.format)) {
                    AMediaFormat *mediaFormat = nullptr;
                    QString mimeType;

                    if (codec.startsWith("audio/")) {
                        mediaFormat = audioMediaFormat;
                        mimeType = "audio/x-raw";
                    } else {
                        mediaFormat = videoMediaFormat;
                        mimeType = "video/x-raw";
                    }

                    AMediaFormat_setString(mediaFormat,
                                           AMEDIAFORMAT_KEY_MIME,
                                           codec.toStdString().c_str());

                    if (AMediaMuxer_addTrack(muxer, mediaFormat) >= 0)
                        supportedCodecs[format.mimeType][mimeType] << codec;

                    AMediaMuxer_start(muxer);
                    AMediaMuxer_delete(muxer);
                }
            }
        }
    }

    AMediaFormat_delete(audioMediaFormat);
    AMediaFormat_delete(videoMediaFormat);

    return supportedCodecs;
}

void MediaWriterNDKMedia::setOutputFormat(const QString &outputFormat)
{
    if (this->d->m_outputFormat == outputFormat)
        return;

    this->d->m_outputFormat = outputFormat;
    emit this->outputFormatChanged(outputFormat);
}

void MediaWriterNDKMedia::setFormatOptions(const QVariantMap &formatOptions)
{
    Q_UNUSED(formatOptions)
}

void MediaWriterNDKMedia::setCodecOptions(int index,
                                          const QVariantMap &codecOptions)
{
    Q_UNUSED(index)
    Q_UNUSED(codecOptions)
}

void MediaWriterNDKMedia::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    if (this->d->m_maxPacketQueueSize == maxPacketQueueSize)
        return;

    this->d->m_maxPacketQueueSize = maxPacketQueueSize;
    emit this->maxPacketQueueSizeChanged(maxPacketQueueSize);
}

void MediaWriterNDKMedia::resetOutputFormat()
{
    this->setOutputFormat("");
}

void MediaWriterNDKMedia::resetFormatOptions()
{
}

void MediaWriterNDKMedia::resetCodecOptions(int index)
{
    Q_UNUSED(index)
}

void MediaWriterNDKMedia::resetMaxPacketQueueSize()
{
    this->setMaxPacketQueueSize(15 * 1024 * 1024);
}

void MediaWriterNDKMedia::enqueuePacket(const AkPacket &packet)
{
    if (this->d->m_isRecording && this->d->m_streamsMap.contains(packet.index()))
        this->d->m_streamsMap[packet.index()]->packetEnqueue(packet);
}

void MediaWriterNDKMedia::clearStreams()
{
    this->d->m_streamConfigs.clear();
    emit this->streamsChanged(this->streams());
}

bool MediaWriterNDKMedia::init()
{
    auto outputFormat = this->d->guessFormat();

    if (outputFormat.isEmpty())
        return false;

    auto formatInfo = OutputFormatsInfo::byMimeType(outputFormat);

    if (!formatInfo)
        return false;

    this->d->m_outputFile.setFileName(this->m_location);

    if (!this->d->m_outputFile.open(QIODevice::ReadWrite
                                    | QIODevice::Truncate))
        return false;

    this->d->m_mediaMuxer =
            AMediaMuxer_new(this->d->m_outputFile.handle(),
                            formatInfo->format);

    if (!this->d->m_mediaMuxer) {
        this->d->m_outputFile.close();

        return false;
    }

    AMediaMuxer_setOrientationHint(this->d->m_mediaMuxer, 0);
    auto streamConfigs = this->d->m_streamConfigs.toVector();

    for (int i = 0; i < streamConfigs.count(); i++) {
        auto configs = streamConfigs[i];

        // Confihure streams parameters.
        AkCaps streamCaps = configs["caps"].value<AkCaps>();
        AbstractStreamPtr mediaStream;
        int inputId = configs["index"].toInt();

        if (streamCaps.mimeType() == "audio/x-raw") {
            mediaStream =
                    AbstractStreamPtr(new AudioStream(this->d->m_mediaMuxer,
                                                      uint(i), inputId,
                                                      configs,
                                                      this));
        } else if (streamCaps.mimeType() == "video/x-raw") {
            mediaStream =
                    AbstractStreamPtr(new VideoStream(this->d->m_mediaMuxer,
                                                      uint(i), inputId,
                                                      configs,
                                                      this));
        } else {
        }

        if (mediaStream) {
            this->d->m_streamsMap[inputId] = mediaStream;

            QObject::connect(mediaStream.data(),
                             SIGNAL(packetReady(size_t,
                                                const uint8_t *,
                                                const AMediaCodecBufferInfo *)),
                             this,
                             SLOT(writePacket(size_t,
                                              const uint8_t *,
                                              const AMediaCodecBufferInfo *)),
                             Qt::DirectConnection);
        }
    }

    for (auto &mediaStream: this->d->m_streamsMap)
        if (!mediaStream->init()) {
            this->d->m_streamsMap.clear();
            AMediaMuxer_stop(this->d->m_mediaMuxer);
            AMediaMuxer_delete(this->d->m_mediaMuxer);
            this->d->m_mediaMuxer = nullptr;
            this->d->m_outputFile.close();

            return false;
        }

    this->d->m_isRecording = true;

    return true;
}

void MediaWriterNDKMedia::uninit()
{
    if (!this->d->m_mediaMuxer)
        return;

    this->d->m_isRecording = false;
    this->d->m_streamsMap.clear();
    AMediaMuxer_stop(this->d->m_mediaMuxer);
    AMediaMuxer_delete(this->d->m_mediaMuxer);
    this->d->m_mediaMuxer = nullptr;
    this->d->m_outputFile.close();
    this->d->m_muxingStarted = false;
}

bool MediaWriterNDKMedia::startMuxing()
{
    this->d->m_startMuxingMutex.lock();

    if (!this->d->m_muxingStarted) {
        for (auto &stream: this->d->m_streamsMap)
            if (!stream->ready()) {
                this->d->m_startMuxingMutex.unlock();

                return false;
            }

        for (auto &stream: this->d->m_streamsMap)
            AMediaMuxer_addTrack(this->d->m_mediaMuxer, stream->mediaFormat());

        if (AMediaMuxer_start(this->d->m_mediaMuxer) != AMEDIA_OK) {
            this->d->m_startMuxingMutex.unlock();

            return false;
        }

        this->d->m_muxingStarted = true;
    }

    this->d->m_startMuxingMutex.unlock();

    return true;
}

void MediaWriterNDKMedia::writePacket(size_t trackIdx,
                                      const uint8_t *data,
                                      const AMediaCodecBufferInfo *info)
{
    this->d->m_writeMutex.lock();

    if (this->d->m_muxingStarted)
        AMediaMuxer_writeSampleData(this->d->m_mediaMuxer,
                                    trackIdx,
                                    data,
                                    info);

    this->d->m_writeMutex.unlock();
}

const OutputFormatsInfoVector &OutputFormatsInfo::info()
{
    static const OutputFormatsInfoVector formatsInfo {
        {"video/webm", AMEDIAMUXER_OUTPUT_FORMAT_WEBM  , "WEBM", {"webm"}},
        {"video/mp4" , AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4,  "MP4",  {"mp4"}},
    };

    return formatsInfo;
}

const OutputFormatsInfo *OutputFormatsInfo::byMimeType(const QString &mimeType)
{
    for (auto &info: info())
        if (info.mimeType == mimeType)
            return &info;

    return nullptr;
}

const OutputFormatsInfo *OutputFormatsInfo::byFormat(OutputFormat format)
{
    for (auto &info: info())
        if (info.format == format)
            return &info;

    return nullptr;
}

const CodecsInfoVector &CodecsInfo::info()
{
    static const CodecsInfoVector codecsInfo {
        // Video
        {"video/x-vnd.on2.vp8", "VP8"         },
        {"video/x-vnd.on2.vp9", "VP9"         },
        {"video/avc"          , "H.264 AVC"   },
        {"video/hevc"         , "H.265 HEVC"  },
        {"video/mp4v-es"      , "MPEG4"       },
        {"video/3gpp"         , "H.263"       },
        {"video/mpeg2"        , "MPEG-2"      },
        {"video/raw"          , "RAW"         },
        {"video/dolby-vision" , "Dolby Vision"},
        {"video/scrambled"    , "Scrambled"   },

        // Audio
        {"audio/3gpp"     , "AMR NB"       },
        {"audio/amr-wb"   , "AMR WB"       },
        {"audio/mpeg"     , "MPEG"         },
        {"audio/mpeg-L1"  , "MPEG Layer I" },
        {"audio/mpeg-L2"  , "MPEG Layer II"},
        {"audio/midi"     , "MIDI"         },
        {"audio/mp4a-latm", "AAC"          },
        {"audio/qcelp"    , "QCELP"        },
        {"audio/vorbis"   , "Vorbis"       },
        {"audio/opus"     , "Opus"         },
        {"audio/g711-alaw", "G.711 a-law"  },
        {"audio/g711-mlaw", "G.711 mu-law" },
        {"audio/raw"      , "RAW"          },
        {"audio/flac"     , "FLAC"         },
        {"audio/aac-adts" , "AAC ADTS"     },
        {"audio/gsm"      , "MS GSM"       },
        {"audio/ac3"      , "AC3"          },
        {"audio/eac3"     , "EAC3"         },
        {"audio/scrambled", "Scrambled"    },
        {"audio/alac"     , "ALAC"         },
    };

    return codecsInfo;
}

const CodecsInfo *CodecsInfo::byMimeType(const QString &mimeType)
{
    for (auto &info: info())
        if (info.mimeType == mimeType)
            return &info;

    return nullptr;
}

#include "moc_mediawriterndkmedia.cpp"
