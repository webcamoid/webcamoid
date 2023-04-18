/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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
#include <QLibrary>
#include <QMutex>
#include <QSharedPointer>
#include <QSize>
#include <QThreadPool>
#include <QVector>
#include <QtMath>
#include <akaudiocaps.h>
#include <akfrac.h>
#include <akcaps.h>
#include <akpacket.h>
#include <aksubtitlecaps.h>
#include <akvideocaps.h>

#include "mediawriterffmpeg.h"
#include "audiostream.h"
#include "videostream.h"

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/pixdesc.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/opt.h>
    #include <libavutil/parseutils.h>
    #include <libavutil/mathematics.h>
}

using AvMediaAkCapsTypeMap = QMap<AVMediaType, AkCaps::CapsType>;
using VectorVideoCaps = QVector<AkVideoCaps>;
using OptionTypeStrMap = QMap<AVOptionType, QString>;
using SupportedCodecsType = QMap<QString, QMap<AVMediaType, QStringList>>;

class MediaWriterFFmpegGlobal
{
    public:
        AvMediaAkCapsTypeMap m_mediaTypeToStr;
        VectorVideoCaps m_dvSupportedCaps;
        VectorVideoCaps m_dnXhdSupportedCaps;
        QVector<QSize> m_h261SupportedSize;
        QVector<QSize> m_h263SupportedSize;
        QVector<QSize> m_gxfSupportedSize;
        QVector<int> m_swfSupportedSampleRates;
        bool m_hasCudaSupport;
        OptionTypeStrMap m_codecFFOptionTypeToStr;
        SupportedCodecsType m_supportedCodecs;
        QMap<QString, QVariantMap> m_codecDefaults;

        MediaWriterFFmpegGlobal();
        inline const AvMediaAkCapsTypeMap &initAvMediaAkCapsTypeMap();
        inline const VectorVideoCaps &initDVSupportedCaps();
        inline const VectorVideoCaps &initDNxHDSupportedCaps();
        inline const QVector<QSize> &initH261SupportedSize();
        inline const QVector<QSize> &initH263SupportedSize();
        inline const QVector<QSize> &initGXFSupportedSize();
        inline const QVector<int> &initSWFSupportedSampleRates();
        inline bool initHasCudaSupport();
        inline const OptionTypeStrMap &initFFOptionTypeStrMap();
        inline SupportedCodecsType initSupportedCodecs();
        inline QMap<QString, QVariantMap> initCodecDefaults();
};

Q_GLOBAL_STATIC(MediaWriterFFmpegGlobal, mediaWriterFFmpegGlobal)

class MediaWriterFFmpegPrivate
{
    public:
        MediaWriterFFmpeg *self;
        QString m_outputFormat;
        QMap<QString, QVariantMap> m_formatOptions;
        QMap<QString, QVariantMap> m_codecOptions;
        QList<QVariantMap> m_streamConfigs;
        AVFormatContext *m_formatContext {nullptr};
        qint64 m_maxPacketQueueSize {15 * 1024 * 1024};
        QMutex m_packetMutex;
        QMap<int, AbstractStreamPtr> m_streamsMap;
        bool m_isRecording {false};

        explicit MediaWriterFFmpegPrivate(MediaWriterFFmpeg *self);
        QString guessFormat();
        QVariantList parseOptions(const AVClass *avClass) const;
        QVariantMap parseOptionsDefaults(const AVClass *avClass) const;
        AVDictionary *formatContextOptions(AVFormatContext *formatContext,
                                           const QVariantMap &options);
};

MediaWriterFFmpeg::MediaWriterFFmpeg(QObject *parent):
    MediaWriter(parent)
{
    this->d = new MediaWriterFFmpegPrivate(this);

    this->m_codecsBlackList = QStringList {
        // This codec fail.
        "vc2",

        // These codecs are too slow for real time recording.
        "ayuv",
        "cinepak",
        "dpx",
        "jpeg2000",
        "libopenjpeg",
        "libschroedinger",
        "libtheora",
        "libvpx-vp9",
        "msvideo1",
        "prores_ks",
        "r10k",
        "r210",
        "roqvideo",
        "snow",
        "svq1",
        "v210",
        "v308",
        "v408",
    };

    // av_log_set_level(AV_LOG_TRACE);
}

MediaWriterFFmpeg::~MediaWriterFFmpeg()
{
    this->uninit();
    avformat_network_deinit();
    delete this->d;
}

QString MediaWriterFFmpeg::defaultFormat()
{
    if (mediaWriterFFmpegGlobal->m_supportedCodecs.isEmpty())
        return {};

    if (mediaWriterFFmpegGlobal->m_supportedCodecs.contains("webm"))
        return QStringLiteral("webm");

    return mediaWriterFFmpegGlobal->m_supportedCodecs.firstKey();
}

QString MediaWriterFFmpeg::outputFormat() const
{
    return this->d->m_outputFormat;
}

QVariantList MediaWriterFFmpeg::streams() const
{
    QVariantList streams;

    for (auto &stream: this->d->m_streamConfigs)
        streams << stream;

    return streams;
}

qint64 MediaWriterFFmpeg::maxPacketQueueSize() const
{
    return this->d->m_maxPacketQueueSize;
}

QStringList MediaWriterFFmpeg::supportedFormats()
{
    QStringList formats;

    for (auto it = mediaWriterFFmpegGlobal->m_supportedCodecs.begin();
         it != mediaWriterFFmpegGlobal->m_supportedCodecs.end();
         it++)
        if (!this->m_formatsBlackList.contains(it.key()))
            formats << it.key();

    std::sort(formats.begin(), formats.end());

    return formats;
}

QStringList MediaWriterFFmpeg::fileExtensions(const QString &format)
{
    auto outputFormat =
            av_guess_format(format.toStdString().c_str(),
                            nullptr,
                            nullptr);

    if (!outputFormat)
        return {};

    QString extensions(outputFormat->extensions);

    if (extensions.isEmpty())
        return {};

    return extensions.split(",");
}

QString MediaWriterFFmpeg::formatDescription(const QString &format)
{
    auto outputFormat =
            av_guess_format(format.toStdString().c_str(),
                            nullptr,
                            nullptr);

    if (!outputFormat)
        return {};

    return QString(outputFormat->long_name);
}

QVariantList MediaWriterFFmpeg::formatOptions()
{
    auto outFormat = this->d->guessFormat();

    if (outFormat.isEmpty())
        return {};

    auto outputFormat =
            av_guess_format(outFormat.toStdString().c_str(),
                            nullptr,
                            nullptr);

    if (!outputFormat)
        return {};

    auto options = this->d->parseOptions(outputFormat->priv_class);
    auto globalFormatOptions = this->d->m_formatOptions.value(outFormat);
    QVariantList formatOptions;

    for (auto &option: options) {
        auto optionList = option.toList();
        auto key = optionList[0].toString();

        if (globalFormatOptions.contains(key))
            optionList[7] = globalFormatOptions[key];

        formatOptions << QVariant(optionList);
    }

    return formatOptions;
}

QStringList MediaWriterFFmpeg::supportedCodecs(const QString &format)
{
    return this->supportedCodecs(format, AkCaps::CapsUnknown);
}

QStringList MediaWriterFFmpeg::supportedCodecs(const QString &format,
                                               AkCaps::CapsType type)
{
    QStringList supportedCodecs;

    if (type == AkCaps::CapsUnknown) {
        for (auto &codecs: mediaWriterFFmpegGlobal->m_supportedCodecs.value(format)) {
            for (auto &codec: codecs)
                if (!this->m_codecsBlackList.contains(codec))
                    supportedCodecs << codec;
        }
    } else {
        auto codecType =
                mediaWriterFFmpegGlobal->m_mediaTypeToStr.key(type,
                                                              AVMEDIA_TYPE_UNKNOWN);
        auto codecs =
                mediaWriterFFmpegGlobal->m_supportedCodecs.value(format).value(codecType);

        for (auto &codec: codecs)
            if (!this->m_codecsBlackList.contains(codec))
                supportedCodecs << codec;
    }

    std::sort(supportedCodecs.begin(), supportedCodecs.end());

    return supportedCodecs;
}

QString MediaWriterFFmpeg::defaultCodec(const QString &format,
                                        AkCaps::CapsType type)
{
    auto outputFormat =
            av_guess_format(format.toStdString().c_str(),
                            nullptr,
                            nullptr);

    if (!outputFormat)
        return QString();

    AVCodecID codecId = type == AkCaps::CapsAudio?
                            outputFormat->audio_codec:
                        type == AkCaps::CapsVideo?
                            outputFormat->video_codec:
                        type == AkCaps::CapsSubtitle?
                            outputFormat->subtitle_codec:
                            AV_CODEC_ID_NONE;

    if (codecId == AV_CODEC_ID_NONE)
        return {};

    if (codecId == AV_CODEC_ID_VP9)
        codecId = AV_CODEC_ID_VP8;

    auto codec = avcodec_find_encoder(codecId);
    QString codecName;

    if (codec)
        codecName = QString(codec->name);

    auto supportedCodecs = this->supportedCodecs(format, type);

    if (supportedCodecs.isEmpty())
        return {};

    if (codecName.isEmpty() || !supportedCodecs.contains(codecName))
        codecName = supportedCodecs.first();

    return codecName;
}

QString MediaWriterFFmpeg::codecDescription(const QString &codec)
{
    auto avCodec =
            avcodec_find_encoder_by_name(codec.toStdString().c_str());

    if (!avCodec)
        return {};

    return QString(avCodec->long_name);
}

AkCaps::CapsType MediaWriterFFmpeg::codecType(const QString &codec)
{
    auto avCodec =
            avcodec_find_encoder_by_name(codec.toStdString().c_str());

    if (!avCodec)
        return {};

    return mediaWriterFFmpegGlobal->m_mediaTypeToStr.value(avCodec->type);
}

QVariantMap MediaWriterFFmpeg::defaultCodecParams(const QString &codec)
{
    return mediaWriterFFmpegGlobal->m_codecDefaults.value(codec);
}

QVariantMap MediaWriterFFmpeg::addStream(int streamIndex,
                                         const AkCaps &streamCaps)
{
    return this->addStream(streamIndex, streamCaps, {});
}

QVariantMap MediaWriterFFmpeg::addStream(int streamIndex,
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

    auto supportedCodecs = this->supportedCodecs(outputFormat, streamCaps.type());

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

QVariantMap MediaWriterFFmpeg::updateStream(int index)
{
    return this->updateStream(index, {});
}

QVariantMap MediaWriterFFmpeg::updateStream(int index,
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
    } else
        codec = this->d->m_streamConfigs[index]["codec"].toString();

    auto codecDefaults = this->defaultCodecParams(codec);

    if ((streamCaps.type() == AkCaps::CapsAudio
         || streamCaps.type() == AkCaps::CapsVideo)
        && codecParams.contains("bitrate")) {
        int bitRate = codecParams["bitrate"].toInt();
        this->d->m_streamConfigs[index]["bitrate"] =
                bitRate > 0? bitRate: codecDefaults["defaultBitRate"].toInt();
        streamChanged = true;
    }

    if (streamCaps.type() == AkCaps::CapsVideo
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

QVariantList MediaWriterFFmpeg::codecOptions(int index)
{
    auto outputFormat = this->d->guessFormat();

    if (outputFormat.isEmpty())
        return {};

    auto codec =
            this->d->m_streamConfigs.value(index).value("codec").toString();

    if (codec.isEmpty())
        return {};

    auto avCodec = avcodec_find_encoder_by_name(codec.toStdString().c_str());

    if (!avCodec)
        return {};

    auto optKey = QString("%1/%2/%3").arg(outputFormat).arg(index).arg(codec);
    auto options = this->d->parseOptions(avCodec->priv_class);
    auto globalCodecOptions = this->d->m_codecOptions.value(optKey);
    QVariantList codecOptions;

    if (codec == "libvpx") {
        quint8 r = 0;

        for (auto &opt: options) {
            auto option = opt.toList();

            if (option[0] == "deadline") {
                option[6] = option[7] = "realtime";
                opt = option;
                r |= 0x1;
            } else if (option[0] == "quality") {
                option[6] = option[7] = "realtime";
                opt = option;
                r |= 0x2;
            }

            if (r > 2)
                break;
        }
    } else if (codec == "libx265") {
        for (auto &opt: options) {
            auto option = opt.toList();

            if (option[0] == "preset") {
                option[6] = option[7] = "ultrafast";
                opt = option;

                break;
            }
        }
    }

    for (auto &option: options) {
        auto optionList = option.toList();
        auto key = optionList[0].toString();

        if (globalCodecOptions.contains(key))
            optionList[7] = globalCodecOptions[key];

        codecOptions << QVariant(optionList);
    }

    return codecOptions;
}

MediaWriterFFmpegPrivate::MediaWriterFFmpegPrivate(MediaWriterFFmpeg *self):
    self(self)
{
}

QString MediaWriterFFmpegPrivate::guessFormat()
{
    QString outputFormat;

    if (self->supportedFormats().contains(this->m_outputFormat))
        outputFormat = this->m_outputFormat;
    else {
        auto format =
                av_guess_format(nullptr,
                                self->location().toStdString().c_str(),
                                nullptr);

        if (format)
            outputFormat = QString(format->name);
    }

    return outputFormat;
}

QVariantList MediaWriterFFmpegPrivate::parseOptions(const AVClass *avClass) const
{
    if (!avClass)
        return QVariantList();

    QList<QVariantList> avOptions;
    QMap<QString, QVariantList> menu;

    for (auto option = avClass->option;
         option;
         option = av_opt_next(&avClass, option)) {
        QVariant value;
        qreal step = 0;

        switch (option->type) {
            case AV_OPT_TYPE_FLAGS:
            case AV_OPT_TYPE_INT:
            case AV_OPT_TYPE_INT64:
            case AV_OPT_TYPE_CONST:
            case AV_OPT_TYPE_PIXEL_FMT:
            case AV_OPT_TYPE_SAMPLE_FMT:
            case AV_OPT_TYPE_DURATION:
            case AV_OPT_TYPE_CHANNEL_LAYOUT:
            case AV_OPT_TYPE_BOOL:
                value = qint64(option->default_val.i64);
                step = 1;
                break;
            case AV_OPT_TYPE_DOUBLE:
            case AV_OPT_TYPE_FLOAT:
                value = option->default_val.dbl;
                step = 0.01;
                break;
            case AV_OPT_TYPE_STRING:
                value = option->default_val.str;
                break;
            case AV_OPT_TYPE_IMAGE_SIZE: {
                int width = 0;
                int height = 0;

                if (av_parse_video_size(&width, &height, option->default_val.str) < 0)
                    value = QSize();
                else
                    value = QSize(width, height);

                break;
            }
            case AV_OPT_TYPE_VIDEO_RATE: {
                AVRational rate;

                if (av_parse_video_rate(&rate, option->default_val.str) < 0)
                    value = QVariant::fromValue(AkFrac());
                else
                    value = QVariant::fromValue(AkFrac(rate.num, rate.den));

                break;
            }
            case AV_OPT_TYPE_COLOR: {
                uint8_t color[4];

                if (av_parse_color(color,
                                   option->default_val.str,
                                   -1,
                                   nullptr) < 0) {
                    value = qRgba(0, 0, 0, 0);
                } else {
                    value = qRgba(color[0], color[1], color[2], color[3]);
                }

                break;
            }
            case AV_OPT_TYPE_RATIONAL:
                value = AkFrac(option->default_val.q.num,
                               option->default_val.q.den).toString();
                break;
            default:
                continue;
        }

        if (option->type == AV_OPT_TYPE_CONST) {
            QVariantList menuOption = {option->name, option->help, value};

            if (menu.contains(option->unit))
                menu[option->unit] << QVariant(menuOption);
            else
                menu[option->unit] = QVariantList {QVariant(menuOption)};
        } else {
            avOptions << QVariantList {
                option->name,
                option->help,
                mediaWriterFFmpegGlobal->m_codecFFOptionTypeToStr.value(option->type),
                option->min,
                option->max,
                step,
                value,
                value,
                option->unit?
                    QVariantList {option->unit}:
                    QVariantList()
            };
        }

        /*
        // I've not idea what's the point of range values since these are the
        // same as AVOption.min AVOption.max, but I will leave the code here
        // just in case it could be useful in the future.
        AVOptionRanges *optionRanges = NULL;
        int nranges = av_opt_query_ranges(&optionRanges,
                                          &avCodec->priv_class,
                                          option->name,
                                          0);

        for (int i = 0; i < nranges; i++) {
            auto ors = optionRanges + i;

            for (int j = 0; j < ors->nb_components; j++) {
                auto range = ors->range + ors->nb_ranges * j;

                for (int k = 0; k < ors->nb_ranges; k++) {
                    qreal min =
                            range[k]->is_range?
                                range[k]->value_min: range[k]->component_min;
                    qreal max =
                            range[k]->is_range?
                                range[k]->value_max: range[k]->component_max;
                    qDebug() << range[k]->str << min << max;
                }
            }
        }

        if (optionRanges)
            av_opt_freep_ranges(&optionRanges);
        */
    }

    QVariantList options;

    for (auto &option: avOptions) {
        auto unitMap = option.last().toList();

        if (!unitMap.isEmpty()) {
            auto optionMenu = menu[unitMap.first().toString()];

            if (option[2].toString() == "flags") {
                QStringList defaultValues;
                QStringList values;

                for (auto &opt: optionMenu) {
                    auto optList = opt.toList();
                    auto flag = optList[2].toUInt();

                    if ((option[6].toUInt() & flag) == flag)
                        defaultValues << optList[0].toString();

                    if ((option[7].toUInt() & flag) == flag)
                        values << optList[0].toString();
                }

                option[6] = defaultValues;
                option[7] = values;
            } else if (!optionMenu.isEmpty()) {
                option[2] = "menu";
                bool defaultFound = false;
                bool valueFound = false;
                QVariant defaultValue;

                for (auto &opt: optionMenu) {
                    auto optList = opt.toList();

                    if (!defaultValue.isValid())
                        defaultValue = optList[0];

                    if (optList[2] == option[6]) {
                        option[6] = optList[0];
                        defaultFound = true;
                    }

                    if (optList[2] == option[7]) {
                        option[7] = optList[0];
                        valueFound = true;
                    }
                }

                if (!defaultFound)
                    option[6] = defaultValue;

                if (!valueFound)
                    option[7] = option[6];
            }

            option[8] = optionMenu;
        }

        options << QVariant(option);
    }

    return options;
}

QVariantMap MediaWriterFFmpegPrivate::parseOptionsDefaults(const AVClass *avClass) const
{
    QVariantMap optionsDefaults;

    for (auto &option: this->parseOptions(avClass)) {
        auto opt = option.toList();
        optionsDefaults[opt[0].toString()] = opt[6].toString();
    }

    return optionsDefaults;
}

AVDictionary *MediaWriterFFmpegPrivate::formatContextOptions(AVFormatContext *formatContext,
                                                             const QVariantMap &options)
{
    auto avClass = formatContext->oformat->priv_class;
    auto currentOptions =
            this->parseOptionsDefaults(formatContext->oformat->priv_class);

    QStringList flagType;

    if (avClass)
        for (auto option = avClass->option;
             option;
             option = av_opt_next(&avClass, option)) {
            if (option->type == AV_OPT_TYPE_FLAGS)
                flagType << option->name;
        }

    AVDictionary *formatOptions = nullptr;

    for (auto it = options.begin();
         it != options.end();
         it++) {
        if (currentOptions.contains(it.key())
            && currentOptions[it.key()] == it.value()) {
            continue;
        }

        QString value;

        if (flagType.contains(it.key())) {
            auto flags = it.value().toStringList();
            value = flags.join('+');

            if (value.isEmpty())
                value = "0";
        } else {
            value = it.value().toString();
        }

        av_dict_set(&formatOptions,
                    it.key().toStdString().c_str(),
                    value.toStdString().c_str(),
                    0);
    }

    return formatOptions;
}

AkVideoCaps MediaWriterFFmpeg::nearestDVCaps(const AkVideoCaps &caps) const
{
    AkVideoCaps nearestCaps;
    auto q = std::numeric_limits<qreal>::max();

    for (auto &sCaps: mediaWriterFFmpegGlobal->m_dvSupportedCaps) {
        qreal dw = sCaps.width() - caps.width();
        qreal dh = sCaps.height() - caps.height();
        qreal df = sCaps.fps().value() - caps.fps().value();
        qreal k = dw * dw + dh * dh + df * df;

        if (k < q) {
            nearestCaps = sCaps;
            q = k;
        } else if (qFuzzyCompare(k, q) && sCaps.format() == caps.format()) {
            nearestCaps = sCaps;
        }
    }

    return nearestCaps;
}

AkVideoCaps MediaWriterFFmpeg::nearestDNxHDCaps(const AkVideoCaps &caps) const
{
    AkVideoCaps nearestCaps;
    auto q = std::numeric_limits<qreal>::max();

    for (auto &sCaps: mediaWriterFFmpegGlobal->m_dnXhdSupportedCaps) {
        qreal dw = sCaps.width() - caps.width();
        qreal dh = sCaps.height() - caps.height();
        AkFrac fps = sCaps.fps().isValid()? sCaps.fps(): caps.fps();
        qreal df = fps.value() - caps.fps().value();
        qreal db = sCaps.property("bitrate").toReal() - caps.property("bitrate").toReal();
        qreal k = dw * dw + dh * dh + df * df + db * db;

        if (k < q) {
            nearestCaps = sCaps;
            nearestCaps.fps() = fps;
            q = k;
        } else if (qFuzzyCompare(k, q) && sCaps.format() == caps.format()) {
            nearestCaps = sCaps;
        }
    }

    return nearestCaps;
}

AkVideoCaps MediaWriterFFmpeg::nearestH261Caps(const AkVideoCaps &caps) const
{
    QSize nearestSize;
    auto q = std::numeric_limits<qreal>::max();

    for (auto &size: mediaWriterFFmpegGlobal->m_h261SupportedSize) {
        qreal dw = size.width() - caps.width();
        qreal dh = size.height() - caps.height();
        qreal k = dw * dw + dh * dh;

        if (k < q) {
            nearestSize = size;
            q = k;

            if (k == 0.)
                break;
        }
    }

    AkVideoCaps nearestCaps(caps);
    nearestCaps.setWidth(nearestSize.width());
    nearestCaps.setHeight(nearestSize.height());

    return nearestCaps;
}

AkVideoCaps MediaWriterFFmpeg::nearestH263Caps(const AkVideoCaps &caps) const
{
    QSize nearestSize;
    auto q = std::numeric_limits<qreal>::max();

    for (auto &size: mediaWriterFFmpegGlobal->m_h263SupportedSize) {
        qreal dw = size.width() - caps.width();
        qreal dh = size.height() - caps.height();
        qreal k = dw * dw + dh * dh;

        if (k < q) {
            nearestSize = size;
            q = k;

            if (k == 0.)
                break;
        }
    }

    AkVideoCaps nearestCaps(caps);
    nearestCaps.setWidth(nearestSize.width());
    nearestCaps.setHeight(nearestSize.height());

    return nearestCaps;
}

AkVideoCaps MediaWriterFFmpeg::nearestGXFCaps(const AkVideoCaps &caps) const
{
    QSize nearestSize;
    auto q = std::numeric_limits<qreal>::max();

    for (auto &size: mediaWriterFFmpegGlobal->m_gxfSupportedSize) {
        qreal dw = size.width() - caps.width();
        qreal dh = size.height() - caps.height();
        qreal k = dw * dw + dh * dh;

        if (k < q) {
            nearestSize = size;
            q = k;

            if (k == 0.)
                break;
        }
    }

    AkVideoCaps nearestCaps(caps);
    nearestCaps.setWidth(nearestSize.width());
    nearestCaps.setHeight(nearestSize.height());

    return nearestCaps;
}

AkAudioCaps MediaWriterFFmpeg::nearestSWFCaps(const AkAudioCaps &caps) const
{
    int nearestSampleRate = 0;
    int q = std::numeric_limits<int>::max();

    for (auto &sampleRate: mediaWriterFFmpegGlobal->m_swfSupportedSampleRates) {
        int k = qAbs(sampleRate - caps.rate());

        if (k < q) {
            nearestSampleRate = sampleRate;
            q = k;

            if (k == 0)
                break;
        }
    }

    AkAudioCaps nearestCaps(caps);
    nearestCaps.setRate(nearestSampleRate);

    return nearestCaps;
}

void MediaWriterFFmpeg::setOutputFormat(const QString &outputFormat)
{
    if (this->d->m_outputFormat == outputFormat)
        return;

    this->d->m_outputFormat = outputFormat;
    emit this->outputFormatChanged(outputFormat);
}

void MediaWriterFFmpeg::setFormatOptions(const QVariantMap &formatOptions)
{
    auto outputFormat = this->d->guessFormat();
    bool modified = false;

    for (auto it = formatOptions.begin();
         it != formatOptions.end();
         it++)
        if (it.value() != this->d->m_formatOptions.value(outputFormat).value(it.key())) {
            this->d->m_formatOptions[outputFormat][it.key()] = it.value();
            modified = true;
        }

    if (modified)
        emit this->formatOptionsChanged(this->d->m_formatOptions.value(outputFormat));
}

void MediaWriterFFmpeg::setCodecOptions(int index,
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

void MediaWriterFFmpeg::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    if (this->d->m_maxPacketQueueSize == maxPacketQueueSize)
        return;

    this->d->m_maxPacketQueueSize = maxPacketQueueSize;
    emit this->maxPacketQueueSizeChanged(maxPacketQueueSize);
}

void MediaWriterFFmpeg::resetOutputFormat()
{
    this->setOutputFormat("");
}

void MediaWriterFFmpeg::resetFormatOptions()
{
    auto outputFormat = this->d->guessFormat();

    if (this->d->m_formatOptions.value(outputFormat).isEmpty())
        return;

    this->d->m_formatOptions.remove(outputFormat);
    emit this->formatOptionsChanged(QVariantMap());
}

void MediaWriterFFmpeg::resetCodecOptions(int index)
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

void MediaWriterFFmpeg::resetMaxPacketQueueSize()
{
    this->setMaxPacketQueueSize(15 * 1024 * 1024);
}

void MediaWriterFFmpeg::enqueuePacket(const AkPacket &packet)
{
    if (this->d->m_isRecording && this->d->m_streamsMap.contains(packet.index()))
        this->d->m_streamsMap[packet.index()]->packetEnqueue(packet);
}

void MediaWriterFFmpeg::clearStreams()
{
    this->d->m_streamConfigs.clear();
    emit this->streamsChanged(this->streams());
}

bool MediaWriterFFmpeg::init()
{
    auto outputFormat = this->d->guessFormat();
    int result =
            avformat_alloc_output_context2(&this->d->m_formatContext,
                                           nullptr,
                                           this->d->m_outputFormat.isEmpty()?
                                                nullptr: this->d->m_outputFormat.toStdString().c_str(),
                                           this->m_location.toStdString().c_str());

    if (result < 0) {
        char error[1024];
        av_strerror(result, error, 1024);
        qDebug() << "Error allocating output context: " << error;

        return false;
    }

    // Initialize and run streams loops.
    auto streamConfigs = this->d->m_streamConfigs.toVector();

    if (!strcmp(this->d->m_formatContext->oformat->name, "mxf_opatom")) {
        QList<QVariantMap> mxfConfigs;

        for (auto &configs: streamConfigs) {
            auto streamCaps = configs["caps"].value<AkCaps>();

            if (streamCaps.type() == AkCaps::CapsVideo) {
                mxfConfigs << configs;

                break;
            }
        }

        if (mxfConfigs.isEmpty())
            for (auto &configs: streamConfigs) {
                auto streamCaps = configs["caps"].value<AkCaps>();

                if (streamCaps.type() == AkCaps::CapsAudio) {
                    mxfConfigs << configs;

                    break;
                }
            }

        streamConfigs = mxfConfigs.toVector();
    }

    for (int i = 0; i < streamConfigs.count(); i++) {
        auto configs = streamConfigs[i];
        auto stream = avformat_new_stream(this->d->m_formatContext, nullptr);
        stream->id = i;

        // Confihure streams parameters.
        AkCaps streamCaps = configs["caps"].value<AkCaps>();
        AbstractStreamPtr mediaStream;
        int inputId = configs["index"].toInt();

        if (streamCaps.type() == AkCaps::CapsAudio) {
            mediaStream =
                    AbstractStreamPtr(new AudioStream(this->d->m_formatContext,
                                                      uint(i), inputId,
                                                      configs,
                                                      this->d->m_codecOptions,
                                                      this));
        } else if (streamCaps.type() == AkCaps::CapsVideo) {
            mediaStream =
                    AbstractStreamPtr(new VideoStream(this->d->m_formatContext,
                                                      uint(i), inputId,
                                                      configs,
                                                      this->d->m_codecOptions,
                                                      this));
        } else {
        }

        if (mediaStream) {
            this->d->m_streamsMap[inputId] = mediaStream;

            QObject::connect(mediaStream.data(),
                             SIGNAL(packetReady(AVPacket*)),
                             this,
                             SLOT(writePacket(AVPacket*)),
                             Qt::DirectConnection);

            mediaStream->init();
        }
    }

    // Print recording information.
    av_dump_format(this->d->m_formatContext,
                   0,
                   this->m_location.toStdString().c_str(),
                   1);

    // Open file.
    if (!(this->d->m_formatContext->oformat->flags & AVFMT_NOFILE)) {
        int error = avio_open(&this->d->m_formatContext->pb,
                              this->m_location.toStdString().c_str(),
                              AVIO_FLAG_READ_WRITE);

        if (error < 0) {
            char errorStr[1024];
            av_strerror(AVERROR(error), errorStr, 1024);
            qDebug() << "Can't open output file: " << errorStr;

            this->d->m_streamsMap.clear();
            avformat_free_context(this->d->m_formatContext);
            this->d->m_formatContext = nullptr;

            return false;
        }
    }

    // Set format options.
    auto formatOptions =
            this->d->formatContextOptions(this->d->m_formatContext,
                                          this->d->m_formatOptions.value(outputFormat));

    // Write file header.
    result = avformat_write_header(this->d->m_formatContext, &formatOptions);
    av_dict_free(&formatOptions);

    if (result < 0) {
        char errorStr[1024];
        av_strerror(AVERROR(result), errorStr, 1024);
        qDebug() << "Can't write header: " << errorStr;

        if (!(this->d->m_formatContext->oformat->flags & AVFMT_NOFILE))
            // Close the output file.
            avio_close(this->d->m_formatContext->pb);

        this->d->m_streamsMap.clear();
        avformat_free_context(this->d->m_formatContext);
        this->d->m_formatContext = nullptr;

        return false;
    }

    this->d->m_isRecording = true;

    return true;
}

void MediaWriterFFmpeg::uninit()
{
    if (!this->d->m_formatContext)
        return;

    this->d->m_isRecording = false;
    this->d->m_streamsMap.clear();

    // Write the trailer, if any. The trailer must be written before you
    // close the CodecContexts open when you wrote the header; otherwise
    // av_write_trailer() may try to use memory that was freed on
    // av_codec_close().
    av_write_trailer(this->d->m_formatContext);

    if (!(this->d->m_formatContext->oformat->flags & AVFMT_NOFILE))
        // Close the output file.
        avio_close(this->d->m_formatContext->pb);

    avformat_free_context(this->d->m_formatContext);
    this->d->m_formatContext = nullptr;
}

void MediaWriterFFmpeg::writePacket(AVPacket *packet)
{
    this->d->m_packetMutex.lock();
    av_interleaved_write_frame(this->d->m_formatContext, packet);
    this->d->m_packetMutex.unlock();
}

MediaWriterFFmpegGlobal::MediaWriterFFmpegGlobal()
{
    avformat_network_init();

#ifndef QT_DEBUG
    av_log_set_level(AV_LOG_QUIET);
#endif

    this->m_mediaTypeToStr = this->initAvMediaAkCapsTypeMap();
    this->m_dvSupportedCaps = this->initDVSupportedCaps();
    this->m_dnXhdSupportedCaps = this->initDNxHDSupportedCaps();
    this->m_h261SupportedSize = this->initH261SupportedSize();
    this->m_h263SupportedSize = this->initH263SupportedSize();
    this->m_gxfSupportedSize = this->initGXFSupportedSize();
    this->m_swfSupportedSampleRates = this->initSWFSupportedSampleRates();
    this->m_hasCudaSupport = this->initHasCudaSupport();
    this->m_codecFFOptionTypeToStr = this->initFFOptionTypeStrMap();
    this->m_supportedCodecs = this->initSupportedCodecs();
    this->m_codecDefaults = this->initCodecDefaults();
}

const AvMediaAkCapsTypeMap &MediaWriterFFmpegGlobal::initAvMediaAkCapsTypeMap()
{
    static const AvMediaAkCapsTypeMap mediaTypeToCapsType = {
        {AVMEDIA_TYPE_UNKNOWN   , AkCaps::CapsUnknown },
        {AVMEDIA_TYPE_VIDEO     , AkCaps::CapsVideo   },
        {AVMEDIA_TYPE_AUDIO     , AkCaps::CapsAudio   },
        {AVMEDIA_TYPE_SUBTITLE  , AkCaps::CapsSubtitle},
    };

    return mediaTypeToCapsType;
}

const VectorVideoCaps &MediaWriterFFmpegGlobal::initDVSupportedCaps()
{
    static const VectorVideoCaps dvSupportedCaps {
        // Digital Video doesn't support height > 576 yet.
        /*
        {AkVideoCaps::Format_yuv422p, 1440, 1080, {25, 1}      },
        {AkVideoCaps::Format_yuv422p, 1280, 1080, {30000, 1001}},
        {AkVideoCaps::Format_yuv422p, 960 , 720 , {60000, 1001}},
        {AkVideoCaps::Format_yuv422p, 960 , 720 , {50, 1}      },*/
        {AkVideoCaps::Format_yuv422p, 720, 576, {25, 1}      },
        {AkVideoCaps::Format_yuv420p, 720, 576, {25, 1}      },
        {AkVideoCaps::Format_yuv411p, 720, 576, {25, 1}      },
        {AkVideoCaps::Format_yuv422p, 720, 480, {30000, 1001}},
        {AkVideoCaps::Format_yuv411p, 720, 480, {30000, 1001}},
    };

    return dvSupportedCaps;
}

const VectorVideoCaps &MediaWriterFFmpegGlobal::initDNxHDSupportedCaps()
{
    static VectorVideoCaps dnXhdSupportedCaps;

    if (!dnXhdSupportedCaps.isEmpty())
        return dnXhdSupportedCaps;

    struct CapsEx
    {
        AkVideoCaps caps;
        quint64 bitrate;
    };

    const QVector<CapsEx> supportedCaps {
        {{AkVideoCaps::Format_yuv422p, 1920, 1080, {60000, 1001}}, 440000000},
        {{AkVideoCaps::Format_yuv422p, 1920, 1080, {50, 1}      }, 365000000},
        {{AkVideoCaps::Format_yuv422p, 1920, 1080, {60000, 1001}}, 290000000},
        {{AkVideoCaps::Format_yuv422p, 1920, 1080, {50, 1}      }, 240000000},
        {{AkVideoCaps::Format_yuv422p, 1920, 1080, {30000, 1001}}, 220000000},
        {{AkVideoCaps::Format_yuv422p, 1920, 1080, {25, 1}      }, 185000000},
        {{AkVideoCaps::Format_yuv422p, 1920, 1080, {24000, 1001}}, 175000000},
        {{AkVideoCaps::Format_yuv422p, 1920, 1080, {30000, 1001}}, 145000000},
        {{AkVideoCaps::Format_yuv422p, 1920, 1080, {25, 1}      }, 120000000},
        {{AkVideoCaps::Format_yuv422p, 1920, 1080, {24000, 1001}}, 115000000},
        {{AkVideoCaps::Format_yuv422p, 1920, 1080, {60000, 1001}}, 90000000 },
        {{AkVideoCaps::Format_yuv422p, 1920, 1080, {24000, 1001}}, 36000000 },
        {{AkVideoCaps::Format_yuv422p, 1920, 1080, {25, 1}      }, 36000000 },
        {{AkVideoCaps::Format_yuv422p, 1920, 1080, {30000, 1001}}, 45000000 },
        {{AkVideoCaps::Format_yuv422p, 1920, 1080, {50, 1}      }, 75000000 },
        {{AkVideoCaps::Format_yuv422p, 1440, 1080, {0, 0}       }, 110000000},
        {{AkVideoCaps::Format_yuv422p, 1440, 1080, {0, 0}       }, 100000000},
        {{AkVideoCaps::Format_yuv422p, 1440, 1080, {0, 0}       }, 90000000 },
        {{AkVideoCaps::Format_yuv422p, 1440, 1080, {0, 0}       }, 84000000 },
        {{AkVideoCaps::Format_yuv422p, 1440, 1080, {0, 0}       }, 80000000 },
        {{AkVideoCaps::Format_yuv422p, 1440, 1080, {0, 0}       }, 63000000 },
        {{AkVideoCaps::Format_yuv422p, 1280, 720 , {60000, 1001}}, 220000000},
        {{AkVideoCaps::Format_yuv422p, 1280, 720 , {50, 1}      }, 180000000},
        {{AkVideoCaps::Format_yuv422p, 1280, 720 , {60000, 1001}}, 145000000},
        {{AkVideoCaps::Format_yuv422p, 1280, 720 , {50, 1}      }, 120000000},
        {{AkVideoCaps::Format_yuv422p, 1280, 720 , {30000, 1001}}, 110000000},
        {{AkVideoCaps::Format_yuv422p, 1280, 720 , {25, 1}      }, 90000000 },
        {{AkVideoCaps::Format_yuv422p, 1280, 720 , {24000, 1001}}, 90000000 },
        {{AkVideoCaps::Format_yuv422p, 1280, 720 , {30000, 1001}}, 75000000 },
        {{AkVideoCaps::Format_yuv422p, 1280, 720 , {25, 1}      }, 60000000 },
        {{AkVideoCaps::Format_yuv422p, 1280, 720 , {24000, 1001}}, 60000000 },
        {{AkVideoCaps::Format_yuv422p, 960 , 720 , {0, 0}       }, 115000000},
        {{AkVideoCaps::Format_yuv422p, 960 , 720 , {0, 0}       }, 75000000 },
        {{AkVideoCaps::Format_yuv422p, 960 , 720 , {0, 0}       }, 60000000 },
        {{AkVideoCaps::Format_yuv422p, 960 , 720 , {0, 0}       }, 42000000 },
    };

    for (auto &capsEx: supportedCaps) {
        dnXhdSupportedCaps << capsEx.caps;
        dnXhdSupportedCaps.last().setProperty("bitrate", capsEx.bitrate);
    }

    return dnXhdSupportedCaps;
}

const QVector<QSize> &MediaWriterFFmpegGlobal::initH261SupportedSize()
{
    static const QVector<QSize> supportedSize = {
        QSize(352, 288),
        QSize(176, 144)
    };

    return supportedSize;
}

const QVector<QSize> &MediaWriterFFmpegGlobal::initH263SupportedSize()
{
    static const QVector<QSize> supportedSize = {
        QSize(1408, 1152),
        QSize(704, 576),
        QSize(352, 288),
        QSize(176, 144),
        QSize(128, 96)
    };

    return supportedSize;
}

const QVector<QSize> &MediaWriterFFmpegGlobal::initGXFSupportedSize()
{
    static const QVector<QSize> supportedSize = {
        QSize(768, 576), // PAL
        QSize(640, 480)  // NTSC
    };

    return supportedSize;
}

const QVector<int> &MediaWriterFFmpegGlobal::initSWFSupportedSampleRates()
{
    static const QVector<int> supportedSampleRates = {
        44100,
        22050,
        11025
    };

    return supportedSampleRates;
}

bool MediaWriterFFmpegGlobal::initHasCudaSupport()
{
    for (auto &libName: QStringList {"cuda", "nvcuda"}) {
        QLibrary lib(libName);

        if (lib.load()) {
            lib.unload();

            return true;
        }
    }

    return false;
}

const OptionTypeStrMap &MediaWriterFFmpegGlobal::initFFOptionTypeStrMap()
{
    static const OptionTypeStrMap optionTypeStrMap = {
        {AV_OPT_TYPE_FLAGS         , "flags"         },
        {AV_OPT_TYPE_INT           , "number"        },
        {AV_OPT_TYPE_INT64         , "number"        },
        {AV_OPT_TYPE_DOUBLE        , "number"        },
        {AV_OPT_TYPE_FLOAT         , "number"        },
        {AV_OPT_TYPE_STRING        , "string"        },
        {AV_OPT_TYPE_RATIONAL      , "frac"          },
        {AV_OPT_TYPE_BINARY        , "binary"        },
        {AV_OPT_TYPE_CONST         , "const"         },
        {AV_OPT_TYPE_DICT          , "dict"          },
        {AV_OPT_TYPE_IMAGE_SIZE    , "image_size"    },
        {AV_OPT_TYPE_PIXEL_FMT     , "pixel_fmt"     },
        {AV_OPT_TYPE_SAMPLE_FMT    , "sample_fmt"    },
        {AV_OPT_TYPE_VIDEO_RATE    , "video_rate"    },
        {AV_OPT_TYPE_DURATION      , "duration"      },
        {AV_OPT_TYPE_COLOR         , "color"         },
        {AV_OPT_TYPE_CHANNEL_LAYOUT, "channel_layout"},
        {AV_OPT_TYPE_BOOL          , "boolean"       },
    };

    return optionTypeStrMap;
}

SupportedCodecsType MediaWriterFFmpegGlobal::initSupportedCodecs()
{
    SupportedCodecsType supportedCodecs;
    void *opaqueFmt = nullptr;

    while (auto outputFormat = av_muxer_iterate(&opaqueFmt)) {
        void *opaqueCdc = nullptr;

        while (auto codec = av_codec_iterate(&opaqueCdc)) {
            if (codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL
                && CODEC_COMPLIANCE > FF_COMPLIANCE_EXPERIMENTAL)
                continue;

            QString codecName(codec->name);

            if ((codecName.contains("nvenc") && !this->m_hasCudaSupport))
                continue;

            bool codecSupported = avformat_query_codec(outputFormat,
                                                       codec->id,
                                                       CODEC_COMPLIANCE) > 0;

            // Fix codecs that are not properly recognized by
            // avformat_query_codec.
            if (!strcmp(outputFormat->name, "matroska")) {
                switch (codec->id) {
                case AV_CODEC_ID_RV10:
                case AV_CODEC_ID_RV20:
                    codecSupported = false;
                    break;
                default:
                    break;
                }
            } else if (!strcmp(outputFormat->name, "mp4")) {
                if (codec->id == AV_CODEC_ID_VP9)
                    codecSupported = false;
            } else if (!strcmp(outputFormat->name, "ogg")
                       || !strcmp(outputFormat->name, "ogv")) {
                switch (codec->id) {
                case AV_CODEC_ID_SPEEX:
                case AV_CODEC_ID_FLAC:
                case AV_CODEC_ID_OPUS:
                case AV_CODEC_ID_VP8:
                    codecSupported = true;
                    break;
                default:
                    break;
                }
            } else if (!strcmp(outputFormat->name, "webm")) {
                switch (codec->id) {
                case AV_CODEC_ID_VORBIS:
                case AV_CODEC_ID_VP8:
                    codecSupported = true;
                    break;
                default:
                    break;
                }
            }

            if (av_codec_is_encoder(codec) && codecSupported) {
                if (codec->type == AVMEDIA_TYPE_VIDEO) {
                    if (!codec->pix_fmts)
                        continue;

                    // Skip Codecs with pixel formats that can't be encoded to.
                    bool isValid = false;

                    for (auto sampleFormat = codec->pix_fmts;
                         *sampleFormat != AV_PIX_FMT_NONE;
                         sampleFormat++) {
                        if (sws_isSupportedOutput(*sampleFormat)) {
                            /* Keep all codecs that have at least one supported
                               pixel format. */
                            isValid = true;

                            break;
                        }
                    }

                    if (!isValid)
                        continue;
                }

                supportedCodecs[outputFormat->name][codec->type] << codecName;
            }
        }
    }

    return supportedCodecs;
}

QMap<QString, QVariantMap> MediaWriterFFmpegGlobal::initCodecDefaults()
{
    QMap<QString, QVariantMap> codecDefaults;
    void *opaqueCdc = nullptr;

    while (auto codec = av_codec_iterate(&opaqueCdc)) {
        if (!av_codec_is_encoder(codec))
            continue;

        auto codecContext = avcodec_alloc_context3(codec);

        if (!codecContext)
            continue;

        QVariantMap codecParams;

        if (codec->type == AVMEDIA_TYPE_AUDIO) {
            QVariantList supportedSampleRates;

            if (codec->supported_samplerates)
                for (int i = 0;
                     int sampleRate = codec->supported_samplerates[i];
                     i++)
                    supportedSampleRates << sampleRate;

            if (supportedSampleRates.isEmpty())
                switch (codec->id) {
                case AV_CODEC_ID_G723_1:
                case AV_CODEC_ID_ADPCM_G726:
                case AV_CODEC_ID_GSM_MS:
                case AV_CODEC_ID_AMR_NB:
                    supportedSampleRates = {8000};
                    break;
                case AV_CODEC_ID_ROQ_DPCM:
                    supportedSampleRates = {22050};
                    break;
                case AV_CODEC_ID_ADPCM_SWF:
                    supportedSampleRates = {
                        44100,
                        22050,
                        11025
                    };

                    break;
                case AV_CODEC_ID_NELLYMOSER:
                    supportedSampleRates = {
                        8000,
                        11025,
                        16000,
                        22050,
                        44100
                    };

                    break;
                default:
                    break;
                }

            QVariantList supportedSampleFormats;

            if (codec->sample_fmts)
                for (int i = 0; ; i++) {
                    auto sampleFormat = codec->sample_fmts[i];

                    if (sampleFormat == AV_SAMPLE_FMT_NONE)
                        break;

                    QPair<AkAudioCaps::SampleFormat, bool> fmt(AudioStream::sampleFormat(sampleFormat),
                                                               av_sample_fmt_is_planar(sampleFormat));
                    supportedSampleFormats << QVariant::fromValue(fmt);
                }

            QVariant defaultSampleFormat;

            if (codecContext->sample_fmt == AV_SAMPLE_FMT_NONE) {
                QPair<AkAudioCaps::SampleFormat, bool> fmt(AkAudioCaps::SampleFormat_s16, false);
                defaultSampleFormat = supportedSampleFormats.value(0, QVariant::fromValue(fmt));
            } else {
                QPair<AkAudioCaps::SampleFormat, bool> fmt(AudioStream::sampleFormat(codecContext->sample_fmt),
                                                           av_sample_fmt_is_planar(codecContext->sample_fmt));
                defaultSampleFormat = QVariant::fromValue(fmt);
            }

            QVariantList supportedChannelLayouts;
            static const size_t layoutStrSize = 1024;
            char layout[layoutStrSize];
            memset(&layout, 0, layoutStrSize);

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
            if (codec->ch_layouts)
                for (int i = 0; codec->ch_layouts[i].nb_channels > 0; i++) {
                    auto &channelLayout = codec->ch_layouts[i];
                    supportedChannelLayouts << AudioStream::channelLayout(channelLayout.u.mask);
                }
#else
            if (codec->channel_layouts)
                for (int i = 0; auto channelLayout = codec->channel_layouts[i]; i++)
                    supportedChannelLayouts << AudioStream::channelLayout(channelLayout);
#endif

            if (supportedChannelLayouts.isEmpty())
                switch (codec->id) {
                case AV_CODEC_ID_AMR_NB:
                case AV_CODEC_ID_ADPCM_G722:
                case AV_CODEC_ID_ADPCM_G726:
                case AV_CODEC_ID_G723_1:
                case AV_CODEC_ID_GSM_MS:
                case AV_CODEC_ID_NELLYMOSER:
                    supportedChannelLayouts << AudioStream::channelLayout(AV_CH_LAYOUT_MONO);
                    break;
                default:
                    break;
                }

            switch (codec->id) {
            case AV_CODEC_ID_G723_1:
                codecContext->bit_rate = 6300;
                break;
            case AV_CODEC_ID_GSM_MS:
                codecContext->bit_rate = 13000;
                break;
            default:
                break;
            }

            codecParams["supportedSampleRates"] = supportedSampleRates;
            codecParams["supportedSampleFormats"] = supportedSampleFormats;
            codecParams["supportedChannelLayouts"] = supportedChannelLayouts;
            codecParams["defaultSampleFormat"] = defaultSampleFormat;
            codecParams["defaultBitRate"] =
                    codecContext->bit_rate?
                        qint64(codecContext->bit_rate): 128000;
            codecParams["defaultSampleRate"] =
                    codecContext->sample_rate?
                        codecContext->sample_rate:
                        supportedSampleRates.value(0, 44100);

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
            AkAudioCaps::ChannelLayout defaultChannelLayout =
                    codecContext->ch_layout.nb_channels > 0?
                        AudioStream::channelLayout(codecContext->ch_layout.u.mask):
                        AkAudioCaps::ChannelLayout(supportedChannelLayouts.value(0, AkAudioCaps::Layout_mono).toInt());
            codecParams["defaultChannelLayout"] = defaultChannelLayout;

            int codecChannelsCount = codecContext->ch_layout.nb_channels;
            int streamChannelsCount = AkAudioCaps::channelCount(defaultChannelLayout);
#else
            AkAudioCaps::ChannelLayout defaultChannelLayout =
                    codecContext->channel_layout?
                        AudioStream::channelLayout(codecContext->channel_layout):
                        AkAudioCaps::ChannelLayout(supportedChannelLayouts.value(0, AkAudioCaps::Layout_mono).toInt());
            codecParams["defaultChannelLayout"] = defaultChannelLayout;

            int codecChannelsCount = codecContext->channels;
            int streamChannelsCount = AkAudioCaps::channelCount(defaultChannelLayout);
#endif
            codecParams["defaultChannels"] = codecChannelsCount?
                                                 codecChannelsCount:
                                                 streamChannelsCount;
        } else if (codec->type == AVMEDIA_TYPE_VIDEO) {
            QVariantList supportedFrameRates;

            if (codec->supported_framerates)
                for (int i = 0; ; i++) {
                    auto frameRate = codec->supported_framerates[i];

                    if (frameRate.num == 0 && frameRate.den == 0)
                        break;

                    supportedFrameRates << QVariant::fromValue(AkFrac(frameRate.num, frameRate.den));
                }

            switch (codec->id) {
            case AV_CODEC_ID_ROQ:
                supportedFrameRates << QVariant::fromValue(AkFrac(30, 1));
                break;
            default:
                break;
            }

            codecParams["supportedFrameRates"] = supportedFrameRates;

            QVariantList supportedPixelFormats;

            if (codec->pix_fmts)
                for (int i = 0; ; i++) {
                    auto pixelFormat = codec->pix_fmts[i];

                    if (pixelFormat == AV_PIX_FMT_NONE)
                        break;

                    auto akFormat = VideoStream::ffToAkFormat(pixelFormat);

                    if (akFormat != AkVideoCaps::Format_none)
                        supportedPixelFormats << int(akFormat);
                }

            codecParams["supportedPixelFormats"] = supportedPixelFormats;
            codecParams["defaultGOP"] = codecContext->gop_size > 0?
                                            codecContext->gop_size: 12;
            codecParams["defaultBitRate"] = qMax<qint64>(codecContext->bit_rate,
                                                         1500000);
            auto akFormat = VideoStream::ffToAkFormat(codecContext->pix_fmt);
            codecParams["defaultPixelFormat"] =
                    codecContext->pix_fmt != AV_PIX_FMT_NONE && akFormat != AkVideoCaps::Format_none?
                                                 int(akFormat):
                                                 supportedPixelFormats.value(0, int(AkVideoCaps::Format_yuv420p)).toInt();
        }

        codecDefaults[codec->name] = codecParams;
        avcodec_free_context(&codecContext);
    }

    return codecDefaults;
}

#include "moc_mediawriterffmpeg.cpp"
