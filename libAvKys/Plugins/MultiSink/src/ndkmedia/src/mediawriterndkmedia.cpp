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
#include <QDebug>
#include <QFileInfo>
#include <QSharedPointer>
#include <QSize>
#include <QVector>
#include <QLibrary>
#include <QThreadPool>
#include <QMutex>
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

class OutputFormatsInfo
{
    public:
        QString mimeType;
        OutputFormat format;
        QString description;
        QStringList fileExtensions;
        QStringList audioCodecs;
        QStringList videoCodecs;

        inline static const OutputFormatsInfoVector &info();
        inline static const OutputFormatsInfo *byMimeType(const QString &mimeType);
};

class CodecsInfo;
using CodecsInfoVector = QVector<CodecsInfo>;

class CodecsInfo
{
    public:
        QString mimeType;
        QString description;
        QString type;
        QVariantMap params;

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
        QMap<int, AbstractStreamPtr> m_streamsMap;
        bool m_isRecording {false};

        explicit MediaWriterNDKMediaPrivate(MediaWriterNDKMedia *self);
        QString guessFormat();
};

MediaWriterNDKMedia::MediaWriterNDKMedia(QObject *parent):
    MediaWriter(parent)
{
    this->d = new MediaWriterNDKMediaPrivate(this);
}

MediaWriterNDKMedia::~MediaWriterNDKMedia()
{
    this->uninit();
    delete this->d;
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

    for (auto &info: OutputFormatsInfo::info())
        formats << info.mimeType;

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
    auto info = OutputFormatsInfo::byMimeType(format);

    if (!info)
        return {};

    QStringList supportedCodecs;

    if (type.isEmpty())
        supportedCodecs = info->audioCodecs + info->videoCodecs;
    else if (type == "audio/x-raw")
        supportedCodecs = info->audioCodecs;
    else if (type == "video/x-raw")
        supportedCodecs = info->videoCodecs;

    std::sort(supportedCodecs.begin(), supportedCodecs.end());

    return supportedCodecs;
}

QString MediaWriterNDKMedia::defaultCodec(const QString &format,
                                          const QString &type)
{
    auto info = OutputFormatsInfo::byMimeType(format);

    if (!info)
        return {};

    if (type == "audio/x-raw")
        return info->audioCodecs.first();
    else if (type == "video/x-raw")
        return info->videoCodecs.first();

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

    return info->type;
}

QVariantMap MediaWriterNDKMedia::defaultCodecParams(const QString &codec)
{
    auto info = CodecsInfo::byMimeType(codec);

    if (!info)
        return {};

    return info->params;
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

    if (!this->d->m_outputFile.open(QIODevice::WriteOnly))
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
                             SIGNAL(packetReady(AVPacket *)),
                             this,
                             SLOT(writePacket(AVPacket *)),
                             Qt::DirectConnection);

            mediaStream->init();
        }
    }

    if (AMediaMuxer_start(this->d->m_mediaMuxer) != AMEDIA_OK) {
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
}

void MediaWriterNDKMedia::writePacket(const AkPacket &packet)
{
    AMediaCodecBufferInfo info;
    memset(&info, 0, sizeof(AMediaCodecBufferInfo));

    this->d->m_writeMutex.lock();

    if (packet) {
        info.size = packet.buffer().size();
        info.presentationTimeUs = qRound(1e6
                                         * packet.pts()
                                         * packet.timeBase().value());
        info.flags = 0;
        AMediaMuxer_writeSampleData(this->d->m_mediaMuxer,
                                    size_t(packet.index()),
                                    reinterpret_cast<const uint8_t *>(packet.buffer().constData()),
                                    &info);
    } else {
        info.flags = AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM;
        AMediaMuxer_writeSampleData(this->d->m_mediaMuxer,
                                    size_t(packet.index()),
                                    nullptr,
                                    &info);
    }

    this->d->m_writeMutex.unlock();
}

const OutputFormatsInfoVector &OutputFormatsInfo::info()
{
    static const OutputFormatsInfoVector formatsInfo {
        {"video/webm", AMEDIAMUXER_OUTPUT_FORMAT_WEBM  , "WEBM", {"webm"}, {"audio/vorbis", "audio/opus"}                   , {"video/x-vnd.on2.vp8", "video/x-vnd.on2.vp9"}            },
        {"video/mp4" , AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4,  "MP4", {"mp4"} , {"audio/mp4a-latm", "audio/amr-wb", "audio/3gpp"}, {"video/avc", "video/hevc", "video/mp4v-es", "video/3gpp"}},
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

const CodecsInfoVector &CodecsInfo::info()
{
    static const CodecsInfoVector codecsInfo {
        // Audio
        {"audio/vorbis"   , "Vorbis", "audio/x-raw", {}},
        {"audio/opus"     , "Opus"  , "audio/x-raw", {}},
        {"audio/mp4a-latm", "AAC"   , "audio/x-raw", {}},
        {"audio/amr-wb"   , "AMR WB", "audio/x-raw", {}},
        {"audio/3gpp"     , "AMR NB", "audio/x-raw", {}},

        // Video
        {"video/x-vnd.on2.vp8", "VP8"       , "video/x-raw", {}},
        {"video/x-vnd.on2.vp9", "VP9"       , "video/x-raw", {}},
        {"video/avc"          , "H.264 AVC" , "video/x-raw", {}},
        {"video/hevc"         , "H.265 HEVC", "video/x-raw", {}},
        {"video/mp4v-es"      , "MPEG4"     , "video/x-raw", {}},
        {"video/3gpp"         , "H.263"     , "video/x-raw", {}},
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
