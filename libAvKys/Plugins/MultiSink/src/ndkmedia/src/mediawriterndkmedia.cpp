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

#define AVCProfileBaseline            0x01
#define AVCProfileMain                0x02
#define AVCProfileExtended            0x04
#define AVCProfileHigh                0x08
#define AVCProfileHigh10              0x10
#define AVCProfileHigh422             0x20
#define AVCProfileHigh444             0x40
#define AVCProfileConstrainedBaseline 0x10000
#define AVCProfileConstrainedHigh     0x80000

#define AVCLevel1  0x01
#define AVCLevel1b 0x02
#define AVCLevel11 0x04
#define AVCLevel12 0x08
#define AVCLevel13 0x10
#define AVCLevel2  0x20
#define AVCLevel21 0x40
#define AVCLevel22 0x80
#define AVCLevel3  0x100
#define AVCLevel31 0x200
#define AVCLevel32 0x400
#define AVCLevel4  0x800
#define AVCLevel41 0x1000
#define AVCLevel42 0x2000
#define AVCLevel5  0x4000
#define AVCLevel51 0x8000
#define AVCLevel52 0x10000

#define H263ProfileBaseline           0x01
#define H263ProfileH320Coding         0x02
#define H263ProfileBackwardCompatible 0x04
#define H263ProfileISWV2              0x08
#define H263ProfileISWV3              0x10
#define H263ProfileHighCompression    0x20
#define H263ProfileInternet           0x40
#define H263ProfileInterlace          0x80
#define H263ProfileHighLatency        0x100

#define H263Level10 0x01
#define H263Level20 0x02
#define H263Level30 0x04
#define H263Level40 0x08
#define H263Level45 0x10
#define H263Level50 0x20
#define H263Level60 0x40
#define H263Level70 0x80

#define MPEG4ProfileSimple           0x01
#define MPEG4ProfileSimpleScalable   0x02
#define MPEG4ProfileCore             0x04
#define MPEG4ProfileMain             0x08
#define MPEG4ProfileNbit             0x10
#define MPEG4ProfileScalableTexture  0x20
#define MPEG4ProfileSimpleFace       0x40
#define MPEG4ProfileSimpleFBA        0x80
#define MPEG4ProfileBasicAnimated    0x100
#define MPEG4ProfileHybrid           0x200
#define MPEG4ProfileAdvancedRealTime 0x400
#define MPEG4ProfileCoreScalable     0x800
#define MPEG4ProfileAdvancedCoding   0x1000
#define MPEG4ProfileAdvancedCore     0x2000
#define MPEG4ProfileAdvancedScalable 0x4000
#define MPEG4ProfileAdvancedSimple   0x8000

#define MPEG4Level0  0x01
#define MPEG4Level0b 0x02
#define MPEG4Level1  0x04
#define MPEG4Level2  0x08
#define MPEG4Level3  0x10
#define MPEG4Level3b 0x18
#define MPEG4Level4  0x20
#define MPEG4Level4a 0x40
#define MPEG4Level5  0x80
#define MPEG4Level6  0x100

#define MPEG2ProfileSimple  0x00
#define MPEG2ProfileMain    0x01
#define MPEG2Profile422     0x02
#define MPEG2ProfileSNR     0x03
#define MPEG2ProfileSpatial 0x04
#define MPEG2ProfileHigh    0x05

#define MPEG2LevelLL  0x00
#define MPEG2LevelML  0x01
#define MPEG2LevelH14 0x02
#define MPEG2LevelHL  0x03
#define MPEG2LevelHP  0x04

#define VP8Level_Version0 0x01
#define VP8Level_Version1 0x02
#define VP8Level_Version2 0x04
#define VP8Level_Version3 0x08

#define VP8ProfileMain 0x01

#define VP9Profile0    0x01
#define VP9Profile1    0x02
#define VP9Profile2    0x04
#define VP9Profile3    0x08
#define VP9Profile2HDR 0x1000
#define VP9Profile3HDR 0x2000

#define VP9Level1  0x1
#define VP9Level11 0x2
#define VP9Level2  0x4
#define VP9Level21 0x8
#define VP9Level3  0x10
#define VP9Level31 0x20
#define VP9Level4  0x40
#define VP9Level41 0x80
#define VP9Level5  0x100
#define VP9Level51 0x200
#define VP9Level52 0x400
#define VP9Level6  0x800
#define VP9Level61 0x1000
#define VP9Level62 0x2000

#define HEVCProfileMain        0x01
#define HEVCProfileMain10      0x02
#define HEVCProfileMainStill   0x04
#define HEVCProfileMain10HDR10 0x1000

#define HEVCMainTierLevel1  0x1
#define HEVCHighTierLevel1  0x2
#define HEVCMainTierLevel2  0x4
#define HEVCHighTierLevel2  0x8
#define HEVCMainTierLevel21 0x10
#define HEVCHighTierLevel21 0x20
#define HEVCMainTierLevel3  0x40
#define HEVCHighTierLevel3  0x80
#define HEVCMainTierLevel31 0x100
#define HEVCHighTierLevel31 0x200
#define HEVCMainTierLevel4  0x400
#define HEVCHighTierLevel4  0x800
#define HEVCMainTierLevel41 0x1000
#define HEVCHighTierLevel41 0x2000
#define HEVCMainTierLevel5  0x4000
#define HEVCHighTierLevel5  0x8000
#define HEVCMainTierLevel51 0x10000
#define HEVCHighTierLevel51 0x20000
#define HEVCMainTierLevel52 0x40000
#define HEVCHighTierLevel52 0x80000
#define HEVCMainTierLevel6  0x100000
#define HEVCHighTierLevel6  0x200000
#define HEVCMainTierLevel61 0x400000
#define HEVCHighTierLevel61 0x800000
#define HEVCMainTierLevel62 0x1000000
#define HEVCHighTierLevel62 0x2000000

#define DolbyVisionProfileDvavPer 0x1
#define DolbyVisionProfileDvavPen 0x2
#define DolbyVisionProfileDvheDer 0x4
#define DolbyVisionProfileDvheDen 0x8
#define DolbyVisionProfileDvheDtr 0x10
#define DolbyVisionProfileDvheStn 0x20
#define DolbyVisionProfileDvheDth 0x40
#define DolbyVisionProfileDvheDtb 0x80
#define DolbyVisionProfileDvheSt  0x100
#define DolbyVisionProfileDvavSe  0x200

#define DolbyVisionLevelHd24  0x1
#define DolbyVisionLevelHd30  0x2
#define DolbyVisionLevelFhd24 0x4
#define DolbyVisionLevelFhd30 0x8
#define DolbyVisionLevelFhd60 0x10
#define DolbyVisionLevelUhd24 0x20
#define DolbyVisionLevelUhd30 0x40
#define DolbyVisionLevelUhd48 0x80
#define DolbyVisionLevelUhd60 0x100

#define BITRATE_MODE_CQ  0
#define BITRATE_MODE_VBR 1
#define BITRATE_MODE_CBR 2

class CodecOption
{
    public:
        QString name;
        int min;
        int max;
        int step;
        int defaultValue;
};

static const QVector<CodecOption> &codecOptionsVector()
{
    static const QVector<CodecOption> options {
#if __ANDROID_API__ >= 28
        {AMEDIAFORMAT_KEY_BITRATE_MODE, 0, 2, 1, 2},
        {AMEDIAFORMAT_KEY_PROFILE     , 0, 0, 1, 0},
        {AMEDIAFORMAT_KEY_LEVEL       , 0, 0, 1, 0},
#endif
    };

    return options;
}

class OutputFormatsInfo;
using OutputFormatsInfoVector = QVector<OutputFormatsInfo>;
using SupportedCodecsType = QMap<QString, QMap<AkCaps::CapsType, QStringList>>;

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

using CodecOptionValue = QMap<QString, int>;
using CodecOptions = QMap<QString, CodecOptionValue>;

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
        QMap<QString, QVariantMap> m_codecOptions;
        bool m_isRecording {false};
        bool m_muxingStarted {false};

        explicit MediaWriterNDKMediaPrivate(MediaWriterNDKMedia *self);
        QString guessFormat();
        static const QStringList &availableCodecs();
        static const SupportedCodecsType &supportedCodecs();
        static QVariantList parseOptions(const QString &codec);
        static const QVariantList menu(const QString &codec,
                                       const QString &option);
        static const QVector<int> codecDefaults(const QString &codec);
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
    return this->supportedCodecs(format, AkCaps::CapsAny);
}

QStringList MediaWriterNDKMedia::supportedCodecs(const QString &format,
                                                 AkCaps::CapsType type)
{
    QStringList supportedCodecs;

    if (type == AkCaps::CapsAny) {
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
                                          AkCaps::CapsType type)
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

AkCaps::CapsType MediaWriterNDKMedia::codecType(const QString &codec)
{
    auto info = CodecsInfo::byMimeType(codec);

    if (!info)
        return {};

    return info->mimeType.startsWith("audio/")?
                AkCaps::CapsAudio:
                AkCaps::CapsVideo;
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
        static const QVariantList supportedPixelFormats {
            int(AkVideoCaps::Format_yuv420p),
            int(AkVideoCaps::Format_nv12)
        };

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
    outputParams["index"] = streamIndex;
    auto codec = codecParams.value("codec").toString();

    if (codec.isEmpty())
        return {};

    auto supportedCodecs = this->supportedCodecs(outputFormat,
                                                 streamCaps.type());

    if (codec.isEmpty() || !supportedCodecs.contains(codec))
        codec = this->defaultCodec(outputFormat, streamCaps.type());

    outputParams["codec"] = codec;
    outputParams["caps"] = QVariant::fromValue(streamCaps);

    auto defaultCodecParams = this->defaultCodecParams(codec);

    if (streamCaps.type() == AkCaps::CapsAudio
        || streamCaps.type() == AkCaps::CapsVideo) {
        int bitrate = codecParams.value("bitrate").toInt();

        if (bitrate < 1)
            bitrate = defaultCodecParams["defaultBitRate"].toInt();

        outputParams["bitrate"] = bitrate;
    }

    if (streamCaps.type() == AkCaps::CapsVideo) {
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
    auto streamCaps = this->d->m_streamConfigs[index]["caps"].value<AkCaps>();

    if (codecParams.contains("caps")
        && this->d->m_streamConfigs[index]["caps"] != codecParams.value("caps")) {
        this->d->m_streamConfigs[index]["caps"] = codecParams.value("caps");
        streamChanged = true;
    }

    QString codec;

    if (codecParams.contains("codec")) {
        if (this->supportedCodecs(outputFormat, streamCaps.type())
            .contains(codecParams["codec"].toString())) {
            codec = codecParams["codec"].toString();
        } else
            codec = this->defaultCodec(outputFormat, streamCaps.type());

        this->d->m_streamConfigs[index]["codec"] = codec;
        streamChanged = true;
    } else {
        codec = this->d->m_streamConfigs[index]["codec"].toString();
    }

    auto codecDefaults = this->defaultCodecParams(codec);

    if ((streamCaps.type() == AkCaps::CapsAudio
         || streamCaps.type() == AkCaps::CapsVideo)
        && codecParams.contains("bitrate")) {
        int bitRate = codecParams["bitrate"].toInt();
        this->d->m_streamConfigs[index]["bitrate"] =
                bitRate > 0? bitRate: codecDefaults["defaultBitRate"].toInt();
        streamChanged = true;
    } else if (streamCaps.type() == AkCaps::CapsVideo
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
    auto outputFormat = this->d->guessFormat();

    if (outputFormat.isEmpty())
        return {};

    auto codec =
            this->d->m_streamConfigs.value(index).value("codec").toString();

    if (codec.isEmpty())
        return {};

    auto optKey = QString("%1/%2/%3").arg(outputFormat).arg(index).arg(codec);
    auto options = this->d->parseOptions(codec);
    auto globalCodecOptions = this->d->m_codecOptions.value(optKey);
    QVariantList codecOptions;

    for (auto &option: options) {
        auto optionList = option.toList();
        auto key = optionList[0].toString();

        if (globalCodecOptions.contains(key))
            optionList[7] = globalCodecOptions[key];

        codecOptions << QVariant(optionList);
    }

    return codecOptions;
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
    AMediaFormat_setFloat(videoMediaFormat,
                          AMEDIAFORMAT_KEY_FRAME_RATE,
                          30.0f);
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
                    AkCaps::CapsType mimeType = AkCaps::CapsUnknown;

                    if (codec.startsWith("audio/")) {
                        mediaFormat = audioMediaFormat;
                        mimeType = AkCaps::CapsAudio;
                    } else {
                        mediaFormat = videoMediaFormat;
                        mimeType = AkCaps::CapsVideo;
                    }

                    if (mimeType != AkCaps::CapsUnknown) {
                        AMediaFormat_setString(mediaFormat,
                                               AMEDIAFORMAT_KEY_MIME,
                                               codec.toStdString().c_str());

                        if (AMediaMuxer_addTrack(muxer, mediaFormat) >= 0)
                            supportedCodecs[format.mimeType][mimeType] << codec;

                        AMediaMuxer_start(muxer);
                    }

                    AMediaMuxer_delete(muxer);
                }
            }
        }
    }

    AMediaFormat_delete(audioMediaFormat);
    AMediaFormat_delete(videoMediaFormat);

    return supportedCodecs;
}

QVariantList MediaWriterNDKMediaPrivate::parseOptions(const QString &codec)
{
    QVariantList options;

    auto profileLevel = MediaWriterNDKMediaPrivate::codecDefaults(codec);

    for (auto &option: codecOptionsVector()) {
        auto menu = MediaWriterNDKMediaPrivate::menu(codec, option.name);

        if (menu.size() > 1) {
            auto defaultValue = option.defaultValue;

            if (!profileLevel.isEmpty()) {
                if (option.name == "profile")
                    defaultValue = profileLevel[0];

                if (option.name == "level")
                    defaultValue = profileLevel[1];
            }

            options << QVariant(QVariantList {
                option.name,
                option.name,
                "menu",
                option.min,
                menu.size(),
                option.step,
                defaultValue,
                defaultValue,
                menu
            });
        }
    }

    return options;
}

const QVariantList MediaWriterNDKMediaPrivate::menu(const QString &codec,
                                                    const QString &option)
{
    class MenuOption
    {
        public:
            QString name;
            int value;
    };

    using MenuOptions = QVector<MenuOption>;

    static const MenuOptions AVCProfile {
        {"Baseline"            , AVCProfileBaseline           },
        {"Main"                , AVCProfileMain               },
        {"Extended"            , AVCProfileExtended           },
        {"High"                , AVCProfileHigh               },
        {"High 10"             , AVCProfileHigh10             },
        {"High 422"            , AVCProfileHigh422            },
        {"High 444"            , AVCProfileHigh444            },
        {"Constrained Baseline", AVCProfileConstrainedBaseline},
        {"Constrained High"    , AVCProfileConstrainedHigh    },
    };

    static const MenuOptions AVCLevel {
        {"1" , AVCLevel1 },
        {"1b", AVCLevel1b},
        {"11", AVCLevel11},
        {"12", AVCLevel12},
        {"13", AVCLevel13},
        {"2" , AVCLevel2 },
        {"21", AVCLevel21},
        {"22", AVCLevel22},
        {"3" , AVCLevel3 },
        {"31", AVCLevel31},
        {"32", AVCLevel32},
        {"4" , AVCLevel4 },
        {"41", AVCLevel41},
        {"42", AVCLevel42},
        {"5" , AVCLevel5 },
        {"51", AVCLevel51},
        {"52", AVCLevel52},
    };

    static const MenuOptions H263Profile {
        {"Baseline"           , H263ProfileBaseline          },
        {"H320 Coding"        , H263ProfileH320Coding        },
        {"Backward Compatible", H263ProfileBackwardCompatible},
        {"ISWV2"              , H263ProfileISWV2             },
        {"ISWV3"              , H263ProfileISWV3             },
        {"High Compression"   , H263ProfileHighCompression   },
        {"Internet"           , H263ProfileInternet          },
        {"Interlace"          , H263ProfileInterlace         },
        {"High Latency"       , H263ProfileHighLatency       },
    };

    static const MenuOptions H263Level {
        {"10", H263Level10},
        {"20", H263Level20},
        {"30", H263Level30},
        {"40", H263Level40},
        {"45", H263Level45},
        {"50", H263Level50},
        {"60", H263Level60},
        {"70", H263Level70},
    };

    static const MenuOptions MPEG4Profile {
        {"Simple"            , MPEG4ProfileSimple          },
        {"Simple Scalable"   , MPEG4ProfileSimpleScalable  },
        {"Core"              , MPEG4ProfileCore            },
        {"Main"              , MPEG4ProfileMain            },
        {"Nbit"              , MPEG4ProfileNbit            },
        {"Scalable Texture"  , MPEG4ProfileScalableTexture },
        {"Simple Face"       , MPEG4ProfileSimpleFace      },
        {"Simple FBA"        , MPEG4ProfileSimpleFBA       },
        {"Basic Animated"    , MPEG4ProfileBasicAnimated   },
        {"Hybrid"            , MPEG4ProfileHybrid          },
        {"Advanced Real Time", MPEG4ProfileAdvancedRealTime},
        {"Core Scalable"     , MPEG4ProfileCoreScalable    },
        {"Advanced Coding"   , MPEG4ProfileAdvancedCoding  },
        {"Advanced Core"     , MPEG4ProfileAdvancedCore    },
        {"Advanced Scalable" , MPEG4ProfileAdvancedScalable},
        {"Advanced Simple"   , MPEG4ProfileAdvancedSimple  },
    };

    static const MenuOptions MPEG4Level {
        {"0" , MPEG4Level0 },
        {"0b", MPEG4Level0b},
        {"1" , MPEG4Level1 },
        {"2" , MPEG4Level2 },
        {"3" , MPEG4Level3 },
        {"3b", MPEG4Level3b},
        {"4" , MPEG4Level4 },
        {"4a", MPEG4Level4a},
        {"5" , MPEG4Level5 },
        {"6" , MPEG4Level6 },
    };

    static const MenuOptions MPEG2Profile {
        {"Simple" , MPEG2ProfileSimple },
        {"Main"   , MPEG2ProfileMain   },
        {"422"    , MPEG2Profile422    },
        {"SNR"    , MPEG2ProfileSNR    },
        {"Spatial", MPEG2ProfileSpatial},
        {"High"   , MPEG2ProfileHigh   },
    };

    static const MenuOptions MPEG2Level {
        {"LL" , MPEG2LevelLL },
        {"ML" , MPEG2LevelML },
        {"H14", MPEG2LevelH14},
        {"HL" , MPEG2LevelHL },
        {"HP" , MPEG2LevelHP },
    };

    static const MenuOptions VP8Level {
        {"Version 0", VP8Level_Version0},
        {"Version 1", VP8Level_Version1},
        {"Version 2", VP8Level_Version2},
        {"Version 3", VP8Level_Version3},
    };

    static const MenuOptions VP8Profile {
        {"Main", VP8ProfileMain},
    };

    static const MenuOptions VP9Profile {
        {"0"    , VP9Profile0   },
        {"1"    , VP9Profile1   },
        {"2"    , VP9Profile2   },
        {"3"    , VP9Profile3   },
        {"2 HDR", VP9Profile2HDR},
        {"3 HDR", VP9Profile3HDR},
    };

    static const MenuOptions VP9Level {
        {"1" , VP9Level1 },
        {"11", VP9Level11},
        {"2" , VP9Level2 },
        {"21", VP9Level21},
        {"3" , VP9Level3 },
        {"31", VP9Level31},
        {"4" , VP9Level4 },
        {"41", VP9Level41},
        {"5" , VP9Level5 },
        {"51", VP9Level51},
        {"52", VP9Level52},
        {"6" , VP9Level6 },
        {"61", VP9Level61},
        {"62", VP9Level62},
    };

    static const MenuOptions HEVCProfile {
        {"Main"          , HEVCProfileMain       },
        {"Main 10"       , HEVCProfileMain10     },
        {"Main Still"    , HEVCProfileMainStill  },
        {"Main 10 HDR 10", HEVCProfileMain10HDR10},
    };

    static const MenuOptions HEVCLevel {
        {"Main Tier 1" , HEVCMainTierLevel1 },
        {"High Tier 1" , HEVCHighTierLevel1 },
        {"Main Tier 2" , HEVCMainTierLevel2 },
        {"High Tier 2" , HEVCHighTierLevel2 },
        {"Main Tier 21", HEVCMainTierLevel21},
        {"High Tier 21", HEVCHighTierLevel21},
        {"Main Tier 3" , HEVCMainTierLevel3 },
        {"High Tier 3" , HEVCHighTierLevel3 },
        {"Main Tier 31", HEVCMainTierLevel31},
        {"High Tier 31", HEVCHighTierLevel31},
        {"Main Tier 4" , HEVCMainTierLevel4 },
        {"High Tier 4" , HEVCHighTierLevel4 },
        {"Main Tier 41", HEVCMainTierLevel41},
        {"High Tier 41", HEVCHighTierLevel41},
        {"Main Tier 5" , HEVCMainTierLevel5 },
        {"High Tier 5" , HEVCHighTierLevel5 },
        {"Main Tier 51", HEVCMainTierLevel51},
        {"High Tier 51", HEVCHighTierLevel51},
        {"Main Tier 52", HEVCMainTierLevel52},
        {"High Tier 52", HEVCHighTierLevel52},
        {"Main Tier 6" , HEVCMainTierLevel6 },
        {"High Tier 6" , HEVCHighTierLevel6 },
        {"Main Tier 61", HEVCMainTierLevel61},
        {"High Tier 61", HEVCHighTierLevel61},
        {"Main Tier 62", HEVCMainTierLevel62},
        {"High Tier 62", HEVCHighTierLevel62},
    };

    static const MenuOptions DolbyVisionProfile {
        {"DvavPer", DolbyVisionProfileDvavPer},
        {"DvavPen", DolbyVisionProfileDvavPen},
        {"DvheDer", DolbyVisionProfileDvheDer},
        {"DvheDen", DolbyVisionProfileDvheDen},
        {"DvheDtr", DolbyVisionProfileDvheDtr},
        {"DvheStn", DolbyVisionProfileDvheStn},
        {"DvheDth", DolbyVisionProfileDvheDth},
        {"DvheDtb", DolbyVisionProfileDvheDtb},
        {"DvheSt" , DolbyVisionProfileDvheSt },
        {"DvavSe" , DolbyVisionProfileDvavSe },
    };

    static const MenuOptions DolbyVisionLevel {
        {"Hd 24" , DolbyVisionLevelHd24 },
        {"Hd 30" , DolbyVisionLevelHd30 },
        {"Fhd 24", DolbyVisionLevelFhd24},
        {"Fhd 30", DolbyVisionLevelFhd30},
        {"Fhd 60", DolbyVisionLevelFhd60},
        {"Uhd 24", DolbyVisionLevelUhd24},
        {"Uhd 30", DolbyVisionLevelUhd30},
        {"Uhd 48", DolbyVisionLevelUhd48},
        {"Uhd 60", DolbyVisionLevelUhd60},
    };

    static const MenuOptions BitrateMode {
        {"Constant Quality" , BITRATE_MODE_CQ },
        {"Variable Bit Rate", BITRATE_MODE_VBR},
        {"Constant Bit Rate", BITRATE_MODE_CBR},
    };

    using MenuOptionsMap = QMap<QString, const MenuOptions *>;

    static const MenuOptionsMap ProfileOptions {
        {"video/avc"          , &AVCProfile        },
        {"video/3gpp"         , &H263Profile       },
        {"video/mp4v-es"      , &MPEG4Profile      },
        {"video/mpeg2"        , &MPEG2Profile      },
        {"video/x-vnd.on2.vp8", &VP8Profile        },
        {"video/x-vnd.on2.vp9", &VP9Profile        },
        {"video/hevc"         , &HEVCProfile       },
        {"video/dolby-vision" , &DolbyVisionProfile},
    };

    static const MenuOptionsMap LevelOptions {
        {"video/avc"          , &AVCLevel        },
        {"video/3gpp"         , &H263Level       },
        {"video/mp4v-es"      , &MPEG4Level      },
        {"video/mpeg2"        , &MPEG2Level      },
        {"video/x-vnd.on2.vp8", &VP8Level        },
        {"video/x-vnd.on2.vp9", &VP9Level        },
        {"video/hevc"         , &HEVCLevel       },
        {"video/dolby-vision" , &DolbyVisionLevel},
    };

    const MenuOptions *menuOptions {nullptr};

#if __ANDROID_API__ >= 28
    if (option == AMEDIAFORMAT_KEY_BITRATE_MODE)
        menuOptions = &BitrateMode;
    else if (option == AMEDIAFORMAT_KEY_PROFILE
             && ProfileOptions.contains(codec))
        menuOptions = ProfileOptions[codec];
    else if (option == AMEDIAFORMAT_KEY_LEVEL
             && LevelOptions.contains(codec))
        menuOptions = LevelOptions[codec];
#endif

    if (!menuOptions)
        return {};

    QVariantList optionsList;

    for (auto &option: *menuOptions)
        optionsList << QVariant(QVariantList {
            option.name,
            QString(),
            option.value
        });

    return optionsList;
}

const QVector<int> MediaWriterNDKMediaPrivate::codecDefaults(const QString &codec)
{
    class CodecProfileLevel
    {
        public:
            QString codec;
            int profile;
            int level;
    };

    static const QVector<CodecProfileLevel> profileLevel {
        {"video/avc"          , AVCProfileBaseline       , AVCLevel41          },
        {"video/3gpp"         , H263ProfileBaseline      , H263Level45         },
        {"video/mp4v-es"      , MPEG4ProfileSimple       , MPEG4Level3         },
        {"video/mpeg2"        , MPEG2ProfileSimple       , MPEG2LevelHL        },
        {"video/x-vnd.on2.vp8", VP8ProfileMain           , VP8Level_Version0   },
        {"video/x-vnd.on2.vp9", VP9Profile0              , VP9Level41          },
        {"video/hevc"         , HEVCProfileMain          , HEVCMainTierLevel51 },
        {"video/dolby-vision" , DolbyVisionProfileDvavPer, DolbyVisionLevelHd24},
    };

    for (auto &pl: profileLevel)
        if (pl.codec == codec)
            return {pl.profile, pl.level};

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
    auto outputFormat = this->d->guessFormat();

    if (outputFormat.isEmpty())
        return;

    auto codec = this->d->m_streamConfigs.value(index).value("codec").toString();

    if (codec.isEmpty())
        return;

    auto optKey = QString("%1/%2/%3").arg(outputFormat).arg(index).arg(codec);
    bool modified = false;

    for (auto it = codecOptions.begin();
         it != codecOptions.end();
         it++)
        if (it.value() != this->d->m_codecOptions.value(optKey).value(it.key())) {
            this->d->m_codecOptions[optKey][it.key()] = it.value();
            modified = true;
        }

    if (modified)
        emit this->codecOptionsChanged(optKey, this->d->m_codecOptions.value(optKey));
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
    auto outputFormat = this->d->guessFormat();

    if (outputFormat.isEmpty())
        return;

    auto codec = this->d->m_streamConfigs.value(index).value("codec").toString();

    if (codec.isEmpty())
        return;

    auto optKey = QString("%1/%2/%3").arg(outputFormat).arg(index).arg(codec);

    if (this->d->m_codecOptions.value(optKey).isEmpty())
        return;

    this->d->m_codecOptions.remove(optKey);
    emit this->codecOptionsChanged(optKey, QVariantMap());
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

        if (streamCaps.type() == AkCaps::CapsAudio) {
            mediaStream =
                    AbstractStreamPtr(new AudioStream(this->d->m_mediaMuxer,
                                                      uint(i), inputId,
                                                      configs,
                                                      this->d->m_codecOptions,
                                                      this));
        } else if (streamCaps.type() == AkCaps::CapsVideo) {
            mediaStream =
                    AbstractStreamPtr(new VideoStream(this->d->m_mediaMuxer,
                                                      uint(i), inputId,
                                                      configs,
                                                      this->d->m_codecOptions,
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
