/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
#include <QSize>
#include <QtMath>
#include <akutils.h>

#include "mediasink.h"

#define CODEC_COMPLIANCE FF_COMPLIANCE_VERY_STRICT
//#define CODEC_COMPLIANCE FF_COMPLIANCE_EXPERIMENTAL
#define THREAD_WAIT_LIMIT 500

struct XRGB
{
    quint8 x;
    quint8 r;
    quint8 g;
    quint8 b;
};

struct BGRX
{
    quint8 b;
    quint8 g;
    quint8 r;
    quint8 x;
};

typedef QMap<AVMediaType, QString> AvMediaTypeStrMap;

inline AvMediaTypeStrMap initAvMediaTypeStrMap()
{
    AvMediaTypeStrMap mediaTypeToStr = {
        {AVMEDIA_TYPE_UNKNOWN   , "unknown/x-raw"   },
        {AVMEDIA_TYPE_VIDEO     , "video/x-raw"     },
        {AVMEDIA_TYPE_AUDIO     , "audio/x-raw"     },
        {AVMEDIA_TYPE_DATA      , "data/x-raw"      },
        {AVMEDIA_TYPE_SUBTITLE  , "text/x-raw"      },
        {AVMEDIA_TYPE_ATTACHMENT, "attachment/x-raw"},
        {AVMEDIA_TYPE_NB        , "nb/x-raw"        }
    };

    return mediaTypeToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(AvMediaTypeStrMap, mediaTypeToStr, (initAvMediaTypeStrMap()))

typedef QVector<AkVideoCaps> VectorVideoCaps;

inline VectorVideoCaps initDVSupportedCaps()
{
    QStringList supportedCaps = {
        // Digital Video doesn't support height > 576 yet.
        /*"video/x-raw,format=yuv422p,width=1440,height=1080,fps=25/1",
          "video/x-raw,format=yuv422p,width=1280,height=1080,fps=30000/1001",
          "video/x-raw,format=yuv422p,width=960,height=720,fps=60000/1001",
          "video/x-raw,format=yuv422p,width=960,height=720,fps=50/1",*/
        "video/x-raw,format=yuv422p,width=720,height=576,fps=25/1",
        "video/x-raw,format=yuv420p,width=720,height=576,fps=25/1",
        "video/x-raw,format=yuv411p,width=720,height=576,fps=25/1",
        "video/x-raw,format=yuv422p,width=720,height=480,fps=30000/1001",
        "video/x-raw,format=yuv411p,width=720,height=480,fps=30000/1001"
    };

    VectorVideoCaps dvSupportedCaps(supportedCaps.size());

    for (int i = 0; i < dvSupportedCaps.size(); i++)
        dvSupportedCaps[i] = supportedCaps[i];

    return dvSupportedCaps;
}

Q_GLOBAL_STATIC_WITH_ARGS(VectorVideoCaps, dvSupportedCaps, (initDVSupportedCaps()))

inline VectorVideoCaps initDNxHDSupportedCaps()
{
    QStringList supportedCaps = {
        "video/x-raw,format=yuv422p,width=1920,height=1080,fps=60000/1001,bitrate=440000000",
        "video/x-raw,format=yuv422p,width=1920,height=1080,fps=50/1,bitrate=365000000",
        "video/x-raw,format=yuv422p,width=1920,height=1080,fps=60000/1001,bitrate=290000000",
        "video/x-raw,format=yuv422p,width=1920,height=1080,fps=50/1,bitrate=240000000",
        "video/x-raw,format=yuv422p,width=1920,height=1080,fps=30000/1001,bitrate=220000000",
        "video/x-raw,format=yuv422p,width=1920,height=1080,fps=25/1,bitrate=185000000",
        "video/x-raw,format=yuv422p,width=1920,height=1080,fps=24000/1001,bitrate=175000000",
        "video/x-raw,format=yuv422p,width=1920,height=1080,fps=30000/1001,bitrate=145000000",
        "video/x-raw,format=yuv422p,width=1920,height=1080,fps=25/1,bitrate=120000000",
        "video/x-raw,format=yuv422p,width=1920,height=1080,fps=24000/1001,bitrate=115000000",
        "video/x-raw,format=yuv422p,width=1920,height=1080,fps=60000/1001,bitrate=90000000",
        "video/x-raw,format=yuv422p,width=1920,height=1080,fps=24000/1001,bitrate=36000000",
        "video/x-raw,format=yuv422p,width=1920,height=1080,fps=25/1,bitrate=36000000",
        "video/x-raw,format=yuv422p,width=1920,height=1080,fps=30000/1001,bitrate=45000000",
        "video/x-raw,format=yuv422p,width=1920,height=1080,fps=50/1,bitrate=75000000",
        "video/x-raw,format=yuv422p,width=1440,height=1080,fps=0/0,bitrate=110000000",
        "video/x-raw,format=yuv422p,width=1440,height=1080,fps=0/0,bitrate=100000000",
        "video/x-raw,format=yuv422p,width=1440,height=1080,fps=0/0,bitrate=90000000",
        "video/x-raw,format=yuv422p,width=1440,height=1080,fps=0/0,bitrate=84000000",
        "video/x-raw,format=yuv422p,width=1440,height=1080,fps=0/0,bitrate=80000000",
        "video/x-raw,format=yuv422p,width=1440,height=1080,fps=0/0,bitrate=63000000",
        "video/x-raw,format=yuv422p,width=1280,height=720,fps=60000/1001,bitrate=220000000",
        "video/x-raw,format=yuv422p,width=1280,height=720,fps=50/1,bitrate=180000000",
        "video/x-raw,format=yuv422p,width=1280,height=720,fps=60000/1001,bitrate=145000000",
        "video/x-raw,format=yuv422p,width=1280,height=720,fps=50/1,bitrate=120000000",
        "video/x-raw,format=yuv422p,width=1280,height=720,fps=30000/1001,bitrate=110000000",
        "video/x-raw,format=yuv422p,width=1280,height=720,fps=25/1,bitrate=90000000",
        "video/x-raw,format=yuv422p,width=1280,height=720,fps=24000/1001,bitrate=90000000",
        "video/x-raw,format=yuv422p,width=1280,height=720,fps=30000/1001,bitrate=75000000",
        "video/x-raw,format=yuv422p,width=1280,height=720,fps=25/1,bitrate=60000000",
        "video/x-raw,format=yuv422p,width=1280,height=720,fps=24000/1001,bitrate=60000000",
        "video/x-raw,format=yuv422p,width=960,height=720,fps=0/0,bitrate=115000000",
        "video/x-raw,format=yuv422p,width=960,height=720,fps=0/0,bitrate=75000000",
        "video/x-raw,format=yuv422p,width=960,height=720,fps=0/0,bitrate=60000000",
        "video/x-raw,format=yuv422p,width=960,height=720,fps=0/0,bitrate=42000000"
    };

    VectorVideoCaps dnXhdSupportedCaps(supportedCaps.size());

    for (int i = 0; i < dnXhdSupportedCaps.size(); i++)
        dnXhdSupportedCaps[i] = supportedCaps[i];

    return dnXhdSupportedCaps;
}

Q_GLOBAL_STATIC_WITH_ARGS(VectorVideoCaps, dnXhdSupportedCaps, (initDNxHDSupportedCaps()))

typedef QVector<QSize> VectorSize;

inline VectorSize initH261SupportedSize()
{
    VectorSize supportedSize = {
        QSize(352, 288),
        QSize(176, 144)
    };

    return supportedSize;
}

Q_GLOBAL_STATIC_WITH_ARGS(VectorSize, h261SupportedSize, (initH261SupportedSize()))

inline VectorSize initH263SupportedSize()
{
    VectorSize supportedSize = {
        QSize(1408, 1152),
        QSize(704, 576),
        QSize(352, 288),
        QSize(176, 144),
        QSize(128, 96)
    };

    return supportedSize;
}

Q_GLOBAL_STATIC_WITH_ARGS(VectorSize, h263SupportedSize, (initH263SupportedSize()))

inline VectorSize initGXFSupportedSize()
{
    VectorSize supportedSize = {
        QSize(768, 576), // PAL
        QSize(640, 480)  // NTSC
    };

    return supportedSize;
}

Q_GLOBAL_STATIC_WITH_ARGS(VectorSize, gxfSupportedSize, (initGXFSupportedSize()))

typedef QVector<int> VectorInt;

inline VectorInt initSWFSupportedSampleRates()
{
    QVector<int> supportedSampleRates = {
        44100,
        22050,
        11025
    };

    return supportedSampleRates;
}

Q_GLOBAL_STATIC_WITH_ARGS(VectorInt, swfSupportedSampleRates, (initSWFSupportedSampleRates()))

MediaSink::MediaSink(QObject *parent): QObject(parent)
{
    av_register_all();
    avcodec_register_all();
    avformat_network_init();

    this->m_formatContext = NULL;
    this->m_packetQueueSize = 0;
    this->m_maxPacketQueueSize = 15 * 1024 * 1024;
    this->m_runAudioLoop = false;
    this->m_runVideoLoop = false;
    this->m_runSubtitleLoop = false;
    this->m_isRecording = false;

    QObject::connect(this,
                     &MediaSink::outputFormatChanged,
                     this,
                     &MediaSink::updateStreams);
}

MediaSink::~MediaSink()
{
    this->uninit();
    avformat_network_deinit();
}

QString MediaSink::location() const
{
    return this->m_location;
}

QString MediaSink::outputFormat() const
{
    return this->m_outputFormat;
}

QVariantMap MediaSink::formatOptions() const
{
    return this->m_formatOptions;
}

QVariantList MediaSink::streams() const
{
    QVariantList streams;

    foreach (QVariantMap stream, this->m_streamConfigs)
        streams << stream;

    return streams;
}

qint64 MediaSink::maxPacketQueueSize() const
{
    return this->m_maxPacketQueueSize;
}

QStringList MediaSink::supportedFormats()
{
    QStringList formats;
    AVOutputFormat *outputFormat = NULL;

    while ((outputFormat = av_oformat_next(outputFormat))) {
        QString format(outputFormat->name);

        if (!formats.contains(format))
            formats << format;
    }

    return formats;
}

QStringList MediaSink::fileExtensions(const QString &format)
{
    AVOutputFormat *outputFormat = av_guess_format(format.toStdString().c_str(),
                                                   NULL,
                                                   NULL);

    if (!outputFormat)
        return QStringList();

    QString extensions(outputFormat->extensions);

    if (extensions.isEmpty())
        return QStringList();

    return extensions.split(",");
}

QString MediaSink::formatDescription(const QString &format)
{
    AVOutputFormat *outputFormat = av_guess_format(format.toStdString().c_str(),
                                                   NULL,
                                                   NULL);

    if (!outputFormat)
        return QString();

    return QString(outputFormat->long_name);
}

QStringList MediaSink::supportedCodecs(const QString &format,
                                       const QString &type)
{
    AVOutputFormat *outputFormat = av_guess_format(format.toStdString().c_str(),
                                                   NULL,
                                                   NULL);

    if (!outputFormat)
        return QStringList();

    QStringList codecs;
    AVCodec *codec = NULL;

    while ((codec = av_codec_next(codec))) {
        if (codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL
            && CODEC_COMPLIANCE > FF_COMPLIANCE_EXPERIMENTAL)
            continue;

        // Real Video codecs are not supported by Matroska.
        if (!strcmp(outputFormat->name, "matroska"))
            if (codec->id == AV_CODEC_ID_RV10
                || codec->id == AV_CODEC_ID_RV20)
                continue;

        if ((type.isEmpty() || mediaTypeToStr->value(codec->type) == type)
            && av_codec_is_encoder(codec)
            && avformat_query_codec(outputFormat,
                                    codec->id,
                                    CODEC_COMPLIANCE) > 0) {
            if (codec->type == AVMEDIA_TYPE_VIDEO) {
                // Skip Codecs with pixel formats that can't be encoded to.
                int unsupported = 0;
                int i = 0;

                if (codec->pix_fmts)
                    forever {
                        AVPixelFormat sampleFormat = codec->pix_fmts[i];

                        if (sampleFormat == AV_PIX_FMT_NONE)
                            break;

                        if (!sws_isSupportedOutput(sampleFormat))
                            unsupported++;

                        i++;
                    }

                // Keep all codecs that have at least one supported pixel
                // format.
                if (unsupported == i)
                    continue;
            }

            codecs << QString(codec->name);
        }
    }

    return codecs;
}

QString MediaSink::defaultCodec(const QString &format, const QString &type)
{
    AVOutputFormat *outputFormat = av_guess_format(format.toStdString().c_str(),
                                                   NULL,
                                                   NULL);

    if (!outputFormat)
        return QString();

    AVCodecID codecId = type == "audio/x-raw"?
                            outputFormat->audio_codec:
                        type == "video/x-raw"?
                            outputFormat->video_codec:
                        type == "text/x-raw"?
                            outputFormat->subtitle_codec:
                            AV_CODEC_ID_NONE;

    if (codecId == AV_CODEC_ID_NONE)
        return QString();

    AVCodec *codec = avcodec_find_encoder(codecId);
    QString codecName(codec->name);

    QStringList supportedCodecs = this->supportedCodecs(format, type);

    if (supportedCodecs.isEmpty())
        return QString();

    if (!supportedCodecs.contains(codecName))
        codecName = supportedCodecs.first();

    return codecName;
}

QString MediaSink::codecDescription(const QString &codec)
{
    AVCodec *avCodec = avcodec_find_encoder_by_name(codec.toStdString().c_str());

    if (!avCodec)
        return QString();

    return QString(avCodec->long_name);
}

QString MediaSink::codecType(const QString &codec)
{
    AVCodec *avCodec = avcodec_find_encoder_by_name(codec.toStdString().c_str());

    if (!avCodec)
        return QString();

    switch (avCodec->type) {
    case AVMEDIA_TYPE_AUDIO:
        return QString("audio/x-raw");
    case AVMEDIA_TYPE_VIDEO:
        return QString("video/x-raw");
    case AVMEDIA_TYPE_SUBTITLE:
        return QString("text/x-raw");
    default:
        break;
    }

    return QString();
}

QVariantMap MediaSink::defaultCodecParams(const QString &codec)
{
    AVCodec *avCodec = avcodec_find_encoder_by_name(codec.toStdString().c_str());

    if (!avCodec)
        return QVariantMap();

    QVariantMap codecParams;
    AVCodecContext *codecContext = avcodec_alloc_context3(avCodec);

    if (avCodec->type == AVMEDIA_TYPE_AUDIO) {
        QVariantList supportedSampleRates;

        if (avCodec->supported_samplerates)
            for (int i = 0; int sampleRate = avCodec->supported_samplerates[i]; i++)
                supportedSampleRates << sampleRate;

        if (supportedSampleRates.isEmpty())
            switch (avCodec->id) {
            case AV_CODEC_ID_G723_1:
            case AV_CODEC_ID_ADPCM_G726:
            case AV_CODEC_ID_GSM_MS:
            case AV_CODEC_ID_AMR_NB:
                supportedSampleRates << 8000;
                break;
            case AV_CODEC_ID_ROQ_DPCM:
                supportedSampleRates << 22050;
                break;
            case AV_CODEC_ID_ADPCM_SWF:
                supportedSampleRates << 44100
                                     << 22050
                                     << 11025;
                break;
            case AV_CODEC_ID_NELLYMOSER:
                supportedSampleRates << 8000
                                     << 11025
                                     << 16000
                                     << 22050
                                     << 44100;
                break;
            default:
                break;
            }

        QStringList supportedSampleFormats;

        if (avCodec->sample_fmts)
            for (int i = 0; ; i++) {
                AVSampleFormat sampleFormat = avCodec->sample_fmts[i];

                if (sampleFormat == AV_SAMPLE_FMT_NONE)
                    break;

                supportedSampleFormats << QString(av_get_sample_fmt_name(sampleFormat));
            }

        QStringList supportedChannelLayouts;
        char layout[1024];

        if (avCodec->channel_layouts)
            for (int i = 0; uint64_t channelLayout = avCodec->channel_layouts[i]; i++) {
                int channels = av_get_channel_layout_nb_channels(channelLayout);
                av_get_channel_layout_string(layout, 1024, channels, channelLayout);
                supportedChannelLayouts << QString(layout);
            }

        if (supportedChannelLayouts.isEmpty())
            switch (avCodec->id) {
            case AV_CODEC_ID_AMR_NB:
            case AV_CODEC_ID_ADPCM_G722:
            case AV_CODEC_ID_ADPCM_G726:
            case AV_CODEC_ID_G723_1:
            case AV_CODEC_ID_GSM_MS:
            case AV_CODEC_ID_NELLYMOSER: {
                uint64_t channelLayout = AV_CH_LAYOUT_MONO;
                int channels = av_get_channel_layout_nb_channels(channelLayout);
                av_get_channel_layout_string(layout, 1024, channels, channelLayout);
                supportedChannelLayouts << QString(layout);
            }
                break;
            default:
                break;
            }

        switch (avCodec->id) {
        case AV_CODEC_ID_G723_1:
            codecContext->bit_rate = 6300;
            break;
        case AV_CODEC_ID_GSM_MS:
            codecContext->bit_rate = 13000;
            break;
        default:
            break;
        };

        codecParams["supportedSampleRates"] = supportedSampleRates;
        codecParams["supportedSampleFormats"] = supportedSampleFormats;
        codecParams["supportedChannelLayouts"] = supportedChannelLayouts;
        codecParams["defaultSampleFormat"] = codecContext->sample_fmt != AV_SAMPLE_FMT_NONE?
                                                QString(av_get_sample_fmt_name(codecContext->sample_fmt)):
                                                supportedSampleFormats.value(0, "s16");
        codecParams["defaultBitRate"] = codecContext->bit_rate?
                                            qint64(codecContext->bit_rate): 128000;
        codecParams["defaultSampleRate"] = codecContext->sample_rate?
                                               codecContext->sample_rate:
                                               supportedSampleRates.value(0, 44100);

        int channels = av_get_channel_layout_nb_channels(codecContext->channel_layout);
        av_get_channel_layout_string(layout, 1024, channels, codecContext->channel_layout);

        QString channelLayout = codecContext->channel_layout?
                                    QString(layout):
                                    supportedChannelLayouts.value(0, "mono");

        codecParams["defaultChannelLayout"] = channelLayout;

        int channelsCount = av_get_channel_layout_nb_channels(av_get_channel_layout(channelLayout.toStdString().c_str()));

        codecParams["defaultChannels"] = codecContext->channels?
                                             codecContext->channels:
                                             channelsCount;
    } else if (avCodec->type == AVMEDIA_TYPE_VIDEO) {
        QVariantList supportedFrameRates;

        if (avCodec->supported_framerates)
            for (int i = 0; ; i++) {
                AVRational frameRate = avCodec->supported_framerates[i];

                if (frameRate.num == 0 && frameRate.den == 0)
                    break;

                supportedFrameRates << QVariant::fromValue(AkFrac(frameRate.num, frameRate.den));
            }

        switch (avCodec->id) {
        case AV_CODEC_ID_ROQ:
            supportedFrameRates << QVariant::fromValue(AkFrac(30, 1));
            break;
        default:
            break;
        }

        codecParams["supportedFrameRates"] = supportedFrameRates;

        QStringList supportedPixelFormats;

        if (avCodec->pix_fmts)
            for (int i = 0; ; i++) {
                AVPixelFormat pixelFormat = avCodec->pix_fmts[i];

                if (pixelFormat == AV_PIX_FMT_NONE)
                    break;

                supportedPixelFormats << QString(av_get_pix_fmt_name(pixelFormat));
            }

        codecParams["supportedPixelFormats"] = supportedPixelFormats;
        codecParams["defaultGOP"] = codecContext->gop_size > 0?
                                        codecContext->gop_size: 12;
        codecParams["defaultBitRate"] = codecContext->bit_rate?
                                            qint64(codecContext->bit_rate): 200000;
        codecParams["defaultPixelFormat"] = codecContext->pix_fmt != AV_PIX_FMT_NONE?
                                            QString(av_get_pix_fmt_name(codecContext->pix_fmt)):
                                            supportedPixelFormats.value(0, "yuv420p");
    }

    av_free(codecContext);

    return codecParams;
}

QVariantMap MediaSink::addStream(int streamIndex,
                                 const AkCaps &streamCaps,
                                 const QVariantMap &codecParams)
{
    QString outputFormat;

    if (this->supportedFormats().contains(this->m_outputFormat))
        outputFormat = this->m_outputFormat;
    else {
        AVOutputFormat *format =
                av_guess_format(NULL,
                                this->m_location.toStdString().c_str(),
                                NULL);

        if (format)
            outputFormat = QString(format->name);
    }

    if (outputFormat.isEmpty())
        return QVariantMap();

    QVariantMap outputParams;

    if (codecParams.contains("label"))
        outputParams["label"] = codecParams["label"];

    outputParams["index"] = streamIndex;
    QString codec;

    if (codecParams.contains("codec")) {
        if (this->supportedCodecs(outputFormat, streamCaps.mimeType())
            .contains(codecParams["codec"].toString())) {
            codec = codecParams["codec"].toString();
        } else
            codec = this->defaultCodec(outputFormat, streamCaps.mimeType());
    } else
        codec = this->defaultCodec(outputFormat, streamCaps.mimeType());

    outputParams["codec"] = codec;

    QVariantMap codecDefaults = this->defaultCodecParams(codec);

    outputParams["codecOptions"] = codecParams.value("codecOptions", QVariantMap());

    if (streamCaps.mimeType() == "audio/x-raw") {
        int bitRate = codecParams.value("bitrate",
                                        codecDefaults["defaultBitRate"]).toInt();
        outputParams["bitrate"] = bitRate > 0?
                                      bitRate:
                                      codecDefaults["defaultBitRate"].toInt();

        AkAudioCaps audioCaps(streamCaps);
        QString sampleFormat = AkAudioCaps::sampleFormatToString(audioCaps.format());
        QStringList supportedSampleFormats = codecDefaults["supportedSampleFormats"].toStringList();

        if (!supportedSampleFormats.isEmpty() && !supportedSampleFormats.contains(sampleFormat)) {
            QString defaultSampleFormat = codecDefaults["defaultSampleFormat"].toString();
            audioCaps.format() = AkAudioCaps::sampleFormatFromString(defaultSampleFormat);
            audioCaps.bps() = 8 * av_get_bytes_per_sample(av_get_sample_fmt(defaultSampleFormat.toStdString().c_str()));
        }

        QVariantList supportedSampleRates = codecDefaults["supportedSampleRates"].toList();

        if (!supportedSampleRates.isEmpty()) {
            int sampleRate = 0;
            int maxDiff = std::numeric_limits<int>::max();

            foreach (QVariant rate, supportedSampleRates) {
                int diff = qAbs(audioCaps.rate() - rate.toInt());

                if (diff < maxDiff) {
                    sampleRate = rate.toInt();

                    if (!diff)
                        break;

                    maxDiff = diff;
                }
            }

            audioCaps.rate() = sampleRate;
        }

        QString channelLayout = AkAudioCaps::channelLayoutToString(audioCaps.layout());
        QStringList supportedChannelLayouts = codecDefaults["supportedChannelLayouts"].toStringList();

        if (!supportedChannelLayouts.isEmpty() && !supportedChannelLayouts.contains(channelLayout)) {
            QString defaultChannelLayout = codecDefaults["defaultChannelLayout"].toString();
            audioCaps.layout() = AkAudioCaps::channelLayoutFromString(defaultChannelLayout);
            audioCaps.channels() = av_get_channel_layout_nb_channels(av_get_channel_layout(defaultChannelLayout.toStdString().c_str()));
        }

        if (outputFormat == "gxf") {
            audioCaps.rate() = 48000;
            audioCaps.layout() = AkAudioCaps::Layout_mono;
            audioCaps.channels() = 1;
        } else if (outputFormat == "mxf") {
            audioCaps.rate() = 48000;
        } else if (outputFormat == "swf") {
            audioCaps = this->nearestSWFCaps(audioCaps);
        }

        outputParams["caps"] = QVariant::fromValue(audioCaps.toCaps());
        outputParams["timeBase"] = QVariant::fromValue(AkFrac(1, audioCaps.rate()));
    } else if (streamCaps.mimeType() == "video/x-raw") {
        int bitRate = codecParams.value("bitrate",
                                        codecDefaults["defaultBitRate"]).toInt();
        outputParams["bitrate"] = bitRate > 0?
                                      bitRate:
                                      codecDefaults["defaultBitRate"].toInt();
        int gop = codecParams.value("gop",
                                    codecDefaults["defaultGOP"]).toInt();
        outputParams["gop"] = gop > 0?
                                  gop:
                                  codecDefaults["defaultGOP"].toInt();

        AkVideoCaps videoCaps(streamCaps);
        QString pixelFormat = AkVideoCaps::pixelFormatToString(videoCaps.format());
        QStringList supportedPixelFormats = codecDefaults["supportedPixelFormats"].toStringList();

        if (!supportedPixelFormats.isEmpty() && !supportedPixelFormats.contains(pixelFormat)) {
            QString defaultPixelFormat = codecDefaults["defaultPixelFormat"].toString();
            videoCaps.format() = AkVideoCaps::pixelFormatFromString(defaultPixelFormat);
            videoCaps.bpp() = AkVideoCaps::bitsPerPixel(videoCaps.format());
        }

        QVariantList supportedFrameRates = codecDefaults["supportedFrameRates"].toList();

        if (!supportedFrameRates.isEmpty()) {
            AkFrac frameRate;
            qreal maxDiff = std::numeric_limits<qreal>::max();

            foreach (QVariant rate, supportedFrameRates) {
                qreal diff = qAbs(videoCaps.fps().value() - rate.value<AkFrac>().value());

                if (diff < maxDiff) {
                    frameRate = rate.value<AkFrac>();

                    if (qIsNull(diff))
                        break;

                    maxDiff = diff;
                }
            }

            videoCaps.fps() = frameRate;
        }

        AVCodec *avCodec = avcodec_find_encoder_by_name(codec.toStdString().c_str());

        switch (avCodec->id) {
        case AV_CODEC_ID_H261:
            videoCaps = this->nearestH261Caps(videoCaps);
            break;
        case AV_CODEC_ID_H263:
            videoCaps = this->nearestH263Caps(videoCaps);
            break;
        case AV_CODEC_ID_DVVIDEO:
            videoCaps = this->nearestDVCaps(videoCaps);
            break;
        case AV_CODEC_ID_DNXHD:
            videoCaps.setProperty("bitrate", outputParams["bitrate"]);
            videoCaps = this->nearestDNxHDCaps(videoCaps);
            outputParams["bitrate"] = videoCaps.property("bitrate");
            videoCaps.setProperty("bitrate", QVariant());
            break;
        case AV_CODEC_ID_ROQ:
            videoCaps.width() = int(qPow(2, qRound(qLn(videoCaps.width()) / qLn(2))));
            videoCaps.height() = int(qPow(2, qRound(qLn(videoCaps.height()) / qLn(2))));
            videoCaps.fps() = AkFrac(qRound(videoCaps.fps().value()), 1);
            break;
        case AV_CODEC_ID_RV10:
            videoCaps.width() = 16 * qRound(videoCaps.width() / 16.);
        case AV_CODEC_ID_AMV:
            videoCaps.height() = 16 * qRound(videoCaps.height() / 16.);
            break;
        case AV_CODEC_ID_XFACE:
            videoCaps.width() = 48;
            videoCaps.height() = 48;
            break;
        default:
            break;
        }

        if (outputFormat == "gxf")
            videoCaps = this->nearestGXFCaps(videoCaps);

        outputParams["caps"] = QVariant::fromValue(videoCaps.toCaps());
        outputParams["timeBase"] = QVariant::fromValue(videoCaps.fps().invert());
    } else if (streamCaps.mimeType() == "text/x-raw") {
        outputParams["caps"] = QVariant::fromValue(streamCaps);
    }

    this->m_streamConfigs << outputParams;
    this->streamsChanged(this->streams());

    return outputParams;
}

QVariantMap MediaSink::updateStream(int index, const QVariantMap &codecParams)
{
    QString outputFormat;

    if (this->supportedFormats().contains(this->m_outputFormat))
        outputFormat = this->m_outputFormat;
    else {
        AVOutputFormat *format =
                av_guess_format(NULL,
                                this->m_location.toStdString().c_str(),
                                NULL);

        if (format)
            outputFormat = QString(format->name);
    }

    if (outputFormat.isEmpty())
        return QVariantMap();

    if (codecParams.contains("label"))
        this->m_streamConfigs[index]["label"] = codecParams["label"];

    AkCaps streamCaps = this->m_streamConfigs[index]["caps"].value<AkCaps>();
    QString codec;
    bool streamChanged = false;

    if (codecParams.contains("codec")) {
        if (this->supportedCodecs(outputFormat, streamCaps.mimeType())
            .contains(codecParams["codec"].toString())) {
            codec = codecParams["codec"].toString();
        } else
            codec = this->defaultCodec(outputFormat, streamCaps.mimeType());

        this->m_streamConfigs[index]["codec"] = codec;
        streamChanged |= true;

        // Update sample format.
        QVariantMap codecDefaults = this->defaultCodecParams(codec);

        if (streamCaps.mimeType() == "audio/x-raw") {
            AkAudioCaps audioCaps(streamCaps);
            QString sampleFormat = AkAudioCaps::sampleFormatToString(audioCaps.format());
            QStringList supportedSampleFormats = codecDefaults["supportedSampleFormats"].toStringList();

            if (!supportedSampleFormats.isEmpty()
                && !supportedSampleFormats.contains(sampleFormat)) {
                QString defaultSampleFormat = codecDefaults["defaultSampleFormat"].toString();
                audioCaps.format() = AkAudioCaps::sampleFormatFromString(defaultSampleFormat);
                audioCaps.bps() = 8 * av_get_bytes_per_sample(av_get_sample_fmt(defaultSampleFormat.toStdString().c_str()));
            }

            QVariantList supportedSampleRates = codecDefaults["supportedSampleRates"].toList();

            if (!supportedSampleRates.isEmpty()) {
                int sampleRate = 0;
                int maxDiff = std::numeric_limits<int>::max();

                foreach (QVariant rate, supportedSampleRates) {
                    int diff = qAbs(audioCaps.rate() - rate.toInt());

                    if (diff < maxDiff) {
                        sampleRate = rate.toInt();

                        if (!diff)
                            break;

                        maxDiff = diff;
                    }
                }

                audioCaps.rate() = sampleRate;
            }

            QString channelLayout = AkAudioCaps::channelLayoutToString(audioCaps.layout());
            QStringList supportedChannelLayouts = codecDefaults["supportedChannelLayouts"].toStringList();

            if (!supportedChannelLayouts.isEmpty() && !supportedChannelLayouts.contains(channelLayout)) {
                QString defaultChannelLayout = codecDefaults["defaultChannelLayout"].toString();
                audioCaps.layout() = AkAudioCaps::channelLayoutFromString(defaultChannelLayout);
                audioCaps.channels() = av_get_channel_layout_nb_channels(av_get_channel_layout(defaultChannelLayout.toStdString().c_str()));
            }

            if (outputFormat == "gxf") {
                audioCaps.rate() = 48000;
                audioCaps.layout() = AkAudioCaps::Layout_mono;
                audioCaps.channels() = 1;
            } else if (outputFormat == "mxf") {
                audioCaps.rate() = 48000;
            } else if (outputFormat == "swf") {
                audioCaps = this->nearestSWFCaps(audioCaps);
            }

            streamCaps = audioCaps.toCaps();
            this->m_streamConfigs[index]["timeBase"] = QVariant::fromValue(AkFrac(1, audioCaps.rate()));
        } else if (streamCaps.mimeType() == "video/x-raw") {
            AkVideoCaps videoCaps(streamCaps);
            QString pixelFormat = AkVideoCaps::pixelFormatToString(videoCaps.format());
            QStringList supportedPixelFormats = codecDefaults["supportedPixelFormats"].toStringList();

            if (!supportedPixelFormats.isEmpty()
                && !supportedPixelFormats.contains(pixelFormat)) {
                QString defaultPixelFormat = codecDefaults["defaultPixelFormat"].toString();
                videoCaps.format() = AkVideoCaps::pixelFormatFromString(defaultPixelFormat);
                videoCaps.bpp() = AkVideoCaps::bitsPerPixel(videoCaps.format());
            }

            QVariantList supportedFrameRates = codecDefaults["supportedFrameRates"].toList();

            if (!supportedFrameRates.isEmpty()) {
                AkFrac frameRate;
                qreal maxDiff = std::numeric_limits<qreal>::max();

                foreach (QVariant rate, supportedFrameRates) {
                    qreal diff = qAbs(videoCaps.fps().value() - rate.value<AkFrac>().value());

                    if (diff < maxDiff) {
                        frameRate = rate.value<AkFrac>();

                        if (qIsNull(diff))
                            break;

                        maxDiff = diff;
                    }
                }

                videoCaps.fps() = frameRate;
            }

            AVCodec *avCodec = avcodec_find_encoder_by_name(codec.toStdString().c_str());

            switch (avCodec->id) {
            case AV_CODEC_ID_H261:
                videoCaps = this->nearestH261Caps(videoCaps);
                break;
            case AV_CODEC_ID_H263:
                videoCaps = this->nearestH263Caps(videoCaps);
                break;
            case AV_CODEC_ID_DVVIDEO:
                videoCaps = this->nearestDVCaps(videoCaps);
                break;
            case AV_CODEC_ID_DNXHD:
                videoCaps.setProperty("bitrate", this->m_streamConfigs[index]["bitrate"]);
                videoCaps = this->nearestDNxHDCaps(videoCaps);
                this->m_streamConfigs[index]["bitrate"] = videoCaps.property("bitrate");
                videoCaps.setProperty("bitrate", QVariant());
                break;
            case AV_CODEC_ID_ROQ:
                videoCaps.width() = int(qPow(2, qRound(qLn(videoCaps.width()) / qLn(2))));
                videoCaps.height() = int(qPow(2, qRound(qLn(videoCaps.height()) / qLn(2))));
                videoCaps.fps() = AkFrac(qRound(videoCaps.fps().value()), 1);
                break;
            case AV_CODEC_ID_RV10:
                videoCaps.width() = 16 * qRound(videoCaps.width() / 16.);
            case AV_CODEC_ID_AMV:
                videoCaps.height() = 16 * qRound(videoCaps.height() / 16.);
                break;
            case AV_CODEC_ID_XFACE:
                videoCaps.width() = 48;
                videoCaps.height() = 48;
                break;
            default:
                break;
            }

            if (outputFormat == "gxf")
                videoCaps = this->nearestGXFCaps(videoCaps);

            streamCaps = videoCaps.toCaps();
            this->m_streamConfigs[index]["timeBase"] = QVariant::fromValue(videoCaps.fps().invert());
        }

        this->m_streamConfigs[index]["caps"] = QVariant::fromValue(streamCaps);
    } else
        codec = this->m_streamConfigs[index]["codec"].toString();

    QVariantMap codecDefaults = this->defaultCodecParams(codec);

    if ((streamCaps.mimeType() == "audio/x-raw"
         || streamCaps.mimeType() == "video/x-raw")
        && codecParams.contains("bitrate")) {
        int bitRate = codecParams["bitrate"].toInt();
        this->m_streamConfigs[index]["bitrate"] = bitRate > 0?
                                                      bitRate:
                                                      codecDefaults["defaultBitRate"].toInt();
        streamChanged |= true;
    }

    if (streamCaps.mimeType() == "video/x-raw"
        && codecParams.contains("gop")) {
        int gop = codecParams["gop"].toInt();
        this->m_streamConfigs[index]["gop"] = gop > 0?
                                                  gop:
                                                  codecDefaults["defaultGOP"].toInt();
        streamChanged |= true;
    }

    if (codecParams.contains("codecOptions")) {
        this->m_streamConfigs[index]["codecOptions"] = codecParams["codecOptions"];
        streamChanged |= true;
    }

    if (streamChanged)
        this->streamUpdated(index);

    return this->m_streamConfigs[index];
}

void MediaSink::flushStreams()
{
    for (uint i = 0; i < this->m_formatContext->nb_streams; i++) {
        AVStream *stream = this->m_formatContext->streams[i];
        AVMediaType mediaType = stream->codec->codec_type;

        if (mediaType == AVMEDIA_TYPE_AUDIO) {
            if (stream->codec->frame_size <= 1)
                continue;

            qint64 pts = this->m_streamParams[int(i)].audioPts();
            int ptsDiff = stream->codec->codec->capabilities
                          & AV_CODEC_CAP_VARIABLE_FRAME_SIZE?
                              1:
                              stream->codec->frame_size;

            forever {
                AVPacket pkt;
                av_init_packet(&pkt);
                pkt.data = NULL;
                pkt.size = 0;

                int gotPacket;

                if (avcodec_encode_audio2(stream->codec,
                                          &pkt,
                                          NULL,
                                          &gotPacket) < 0)
                    break;

                if (!gotPacket)
                    break;

                pkt.pts = pkt.dts = pts;
                pts += ptsDiff;
                av_packet_rescale_ts(&pkt, stream->codec->time_base, stream->time_base);
                pkt.stream_index = int(i);
                av_interleaved_write_frame(this->m_formatContext, &pkt);
                av_packet_unref(&pkt);
            }
        } else if (mediaType == AVMEDIA_TYPE_VIDEO) {
            if (this->m_formatContext->oformat->flags & AVFMT_RAWPICTURE
                && stream->codec->codec->id == AV_CODEC_ID_RAWVIDEO)
                continue;

            forever {
                AVPacket pkt;
                av_init_packet(&pkt);
                pkt.data = NULL;
                pkt.size = 0;

                int gotPacket;

                if (avcodec_encode_video2(stream->codec,
                                          &pkt,
                                          NULL,
                                          &gotPacket) < 0)
                    break;

                if (!gotPacket)
                    break;

                pkt.pts = pkt.dts = this->m_streamParams[int(i)].nextPts(0, 0);
                av_packet_rescale_ts(&pkt, stream->codec->time_base, stream->time_base);
                pkt.stream_index = int(i);
                av_interleaved_write_frame(this->m_formatContext, &pkt);
                av_packet_unref(&pkt);
            }
        }
    }
}

QImage MediaSink::swapChannels(const QImage &image) const
{
    QImage swapped(image.size(), image.format());

    for (int y = 0; y < image.height(); y++) {
        const XRGB *src = reinterpret_cast<const XRGB *>(image.constScanLine(y));
        BGRX *dst = reinterpret_cast<BGRX *>(swapped.scanLine(y));

        for (int x = 0; x < image.width(); x++) {
            dst[x].x = src[x].x;
            dst[x].r = src[x].r;
            dst[x].g = src[x].g;
            dst[x].b = src[x].b;
        }
    }

    return swapped;
}

AkVideoCaps MediaSink::nearestDVCaps(const AkVideoCaps &caps) const
{
    AkVideoCaps nearestCaps;
    qreal q = std::numeric_limits<qreal>::max();

    foreach (AkVideoCaps sCaps, *dvSupportedCaps) {
        qreal dw = sCaps.width() - caps.width();
        qreal dh = sCaps.height() - caps.height();
        qreal df = sCaps.fps().value() - caps.fps().value();
        qreal k = dw * dw + dh * dh + df * df;

        if (k < q) {
            nearestCaps = sCaps;
            q = k;
        } else if (qFuzzyCompare(k, q) && sCaps.format() == caps.format())
            nearestCaps = sCaps;
    }

    return nearestCaps;
}

AkVideoCaps MediaSink::nearestDNxHDCaps(const AkVideoCaps &caps) const
{
    AkVideoCaps nearestCaps;
    qreal q = std::numeric_limits<qreal>::max();

    foreach (AkVideoCaps sCaps, *dnXhdSupportedCaps) {
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
        } else if (qFuzzyCompare(k, q) && sCaps.format() == caps.format())
            nearestCaps = sCaps;
    }

    return nearestCaps;
}

AkVideoCaps MediaSink::nearestH261Caps(const AkVideoCaps &caps) const
{
    QSize nearestSize;
    qreal q = std::numeric_limits<qreal>::max();

    foreach (QSize size, *h261SupportedSize) {
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
    nearestCaps.width() = nearestSize.width();
    nearestCaps.height() = nearestSize.height();

    return nearestCaps;
}

AkVideoCaps MediaSink::nearestH263Caps(const AkVideoCaps &caps) const
{
    QSize nearestSize;
    qreal q = std::numeric_limits<qreal>::max();

    foreach (QSize size, *h263SupportedSize) {
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
    nearestCaps.width() = nearestSize.width();
    nearestCaps.height() = nearestSize.height();

    return nearestCaps;
}

AkVideoCaps MediaSink::nearestGXFCaps(const AkVideoCaps &caps) const
{
    QSize nearestSize;
    qreal q = std::numeric_limits<qreal>::max();

    foreach (QSize size, *gxfSupportedSize) {
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
    nearestCaps.width() = nearestSize.width();
    nearestCaps.height() = nearestSize.height();

    return nearestCaps;
}

AkAudioCaps MediaSink::nearestSWFCaps(const AkAudioCaps &caps) const
{
    int nearestSampleRate = 0;
    int q = std::numeric_limits<int>::max();

    foreach (int sampleRate, *swfSupportedSampleRates) {
        int k = qAbs(sampleRate - caps.rate());

        if (k < q) {
            nearestSampleRate = sampleRate;
            q = k;

            if (k == 0)
                break;
        }
    }

    AkAudioCaps nearestCaps(caps);
    nearestCaps.rate() = nearestSampleRate;

    return nearestCaps;
}

void MediaSink::writeAudioLoop(MediaSink *self)
{
    while (self->m_runAudioLoop) {
        self->m_audioMutex.lock();
        bool gotPacket = true;

        if (self->m_audioPackets.isEmpty())
            gotPacket = self->m_audioQueueNotEmpty.wait(&self->m_audioMutex,
                                                        THREAD_WAIT_LIMIT);

        AkAudioPacket packet;

        if (gotPacket) {
            packet = self->m_audioPackets.dequeue();
            self->decreasePacketQueue(packet.buffer().size());
        }

        self->m_audioMutex.unlock();

        if (gotPacket)
            self->writeAudioPacket(packet);
    }
}

void MediaSink::writeVideoLoop(MediaSink *self)
{
    while (self->m_runVideoLoop) {
        self->m_videoMutex.lock();
        bool gotPacket = true;

        if (self->m_videoPackets.isEmpty())
            gotPacket = self->m_videoQueueNotEmpty.wait(&self->m_videoMutex,
                                                        THREAD_WAIT_LIMIT);

        AkVideoPacket packet;

        if (gotPacket) {
            packet = self->m_videoPackets.dequeue();
            self->decreasePacketQueue(packet.buffer().size());
        }

        self->m_videoMutex.unlock();

        if (gotPacket)
            self->writeVideoPacket(packet);
    }
}

void MediaSink::writeSubtitleLoop(MediaSink *self)
{
    while (self->m_runSubtitleLoop) {
        self->m_subtitleMutex.lock();
        bool gotPacket = true;

        if (self->m_subtitlePackets.isEmpty())
            gotPacket = self->m_subtitleQueueNotEmpty.wait(&self->m_subtitleMutex,
                                                           THREAD_WAIT_LIMIT);

        AkPacket packet;

        if (gotPacket) {
            packet = self->m_subtitlePackets.dequeue();
            self->decreasePacketQueue(packet.buffer().size());
        }

        self->m_subtitleMutex.unlock();

        if (gotPacket)
            self->writeSubtitlePacket(packet);
    }
}

void MediaSink::decreasePacketQueue(int packetSize)
{
    this->m_packetMutex.lock();
    this->m_packetQueueSize -= packetSize;

    if (this->m_packetQueueSize <= this->m_maxPacketQueueSize)
        this->m_packetQueueNotFull.wakeAll();

    this->m_packetMutex.unlock();
}

void MediaSink::setLocation(const QString &location)
{
    if (this->m_location == location)
        return;

    this->m_location = location;
    emit this->locationChanged(location);
}

void MediaSink::setOutputFormat(const QString &outputFormat)
{
    if (this->m_outputFormat == outputFormat)
        return;

    this->m_outputFormat = outputFormat;
    emit this->outputFormatChanged(outputFormat);
}

void MediaSink::setFormatOptions(const QVariantMap &formatOptions)
{
    if (this->m_formatOptions == formatOptions)
        return;

    this->m_formatOptions = formatOptions;
    emit this->formatOptionsChanged(formatOptions);
}

void MediaSink::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    if (this->m_maxPacketQueueSize == maxPacketQueueSize)
        return;

    this->m_maxPacketQueueSize = maxPacketQueueSize;
    emit this->maxPacketQueueSizeChanged(maxPacketQueueSize);
}

void MediaSink::resetLocation()
{
    this->setLocation("");
}

void MediaSink::resetOutputFormat()
{
    this->setOutputFormat("");
}

void MediaSink::resetFormatOptions()
{
    this->setFormatOptions(QVariantMap());
}

void MediaSink::resetMaxPacketQueueSize()
{
    this->setMaxPacketQueueSize(15 * 1024 * 1024);
}

void MediaSink::enqueuePacket(const AkPacket &packet)
{
    forever {
        if (!this->m_isRecording)
            return;

        this->m_packetMutex.lock();
        bool canEnqueue = true;

        if (this->m_packetQueueSize >= this->m_maxPacketQueueSize)
            canEnqueue = this->m_packetQueueNotFull.wait(&this->m_packetMutex, THREAD_WAIT_LIMIT);

        if (canEnqueue) {
            if (packet.caps().mimeType() == "audio/x-raw") {
                this->m_audioMutex.lock();
                this->m_audioPackets.enqueue(AkAudioPacket(packet));
                this->m_audioMutex.unlock();
            } else if (packet.caps().mimeType() == "video/x-raw") {
                this->m_videoMutex.lock();
                this->m_videoPackets.enqueue(AkVideoPacket(packet));
                this->m_videoMutex.unlock();
            } else if (packet.caps().mimeType() == "text/x-raw") {
                this->m_subtitleMutex.lock();
                this->m_subtitlePackets.enqueue(packet);
                this->m_subtitleMutex.unlock();
            }

            this->m_packetQueueSize += packet.buffer().size();
            this->m_packetMutex.unlock();

            break;
        }

        this->m_packetMutex.unlock();
    }
}

void MediaSink::clearStreams()
{
    this->m_streamConfigs.clear();
    this->streamsChanged(this->streams());
}

bool MediaSink::init()
{
    if (avformat_alloc_output_context2(&this->m_formatContext,
                                       NULL,
                                       this->m_outputFormat.isEmpty()?
                                            NULL: this->m_outputFormat.toStdString().c_str(),
                                       this->m_location.toStdString().c_str()) < 0)
        return false;

    QVector<QVariantMap> streamConfigs = this->m_streamConfigs.toVector();

    if (!strcmp(this->m_formatContext->oformat->name, "mxf_opatom")) {
        QList<QVariantMap> mxfConfigs;

        foreach (QVariantMap configs, streamConfigs) {
            AkCaps streamCaps = configs["caps"].value<AkCaps>();

            if (streamCaps.mimeType() == "video/x-raw") {
                mxfConfigs << configs;

                break;
            }
        }

        if (mxfConfigs.isEmpty())
            foreach (QVariantMap configs, streamConfigs) {
                AkCaps streamCaps = configs["caps"].value<AkCaps>();

                if (streamCaps.mimeType() == "audio/x-raw") {
                    mxfConfigs << configs;

                    break;
                }
            }

        streamConfigs = mxfConfigs.toVector();
    }

    for (int i = 0; i < streamConfigs.count(); i++) {
        QVariantMap configs = streamConfigs[i];
        QString codecName = configs["codec"].toString();

        AVCodec *codec = avcodec_find_encoder_by_name(codecName.toStdString().c_str());
        AVStream *stream = avformat_new_stream(this->m_formatContext, codec);

        stream->id = i;

        // Some formats want stream headers to be separate.
        if (this->m_formatContext->oformat->flags & AVFMT_GLOBALHEADER)
            stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

        // Use experimental codecs by default
        stream->codec->strict_std_compliance = CODEC_COMPLIANCE;

        // Confihure streams parameters.
        AkCaps streamCaps = configs["caps"].value<AkCaps>();

        if (streamCaps.mimeType() == "audio/x-raw") {
            stream->codec->bit_rate = configs["bitrate"].toInt();

            switch (codec->id) {
            case AV_CODEC_ID_G723_1:
                stream->codec->bit_rate = 6300;
                break;
            case AV_CODEC_ID_GSM_MS:
                stream->codec->bit_rate = 13000;
                break;
            default:
                break;
            }

            AkAudioCaps audioCaps(streamCaps);

            if (!strcmp(this->m_formatContext->oformat->name, "gxf")) {
                audioCaps.rate() = 48000;
                audioCaps.layout() = AkAudioCaps::Layout_mono;
                audioCaps.channels() = 1;
            } else if (!strcmp(this->m_formatContext->oformat->name, "mxf")) {
                audioCaps.rate() = 48000;
            } else if (!strcmp(this->m_formatContext->oformat->name, "swf")) {
                audioCaps = this->nearestSWFCaps(audioCaps);
            }

            QString sampleFormat = AkAudioCaps::sampleFormatToString(audioCaps.format());
            stream->codec->sample_fmt = av_get_sample_fmt(sampleFormat.toStdString().c_str());
            stream->codec->sample_rate = audioCaps.rate();
            QString layout = AkAudioCaps::channelLayoutToString(audioCaps.layout());
            stream->codec->channel_layout = av_get_channel_layout(layout.toStdString().c_str());
            stream->codec->channels = audioCaps.channels();

            AkFrac timeBase(configs["timeBase"].value<AkFrac>());

            stream->time_base.num = int(timeBase.num());
            stream->time_base.den = int(timeBase.den());
        } else if (streamCaps.mimeType() == "video/x-raw") {
            AkVideoCaps videoCaps(streamCaps);

            switch (codec->id) {
            case AV_CODEC_ID_H261:
                videoCaps = this->nearestH261Caps(videoCaps);
                break;
            case AV_CODEC_ID_H263:
                videoCaps = this->nearestH263Caps(videoCaps);
                break;
            case AV_CODEC_ID_DVVIDEO:
                videoCaps = this->nearestDVCaps(videoCaps);
                break;
            case AV_CODEC_ID_DNXHD:
                videoCaps.setProperty("bitrate", configs["bitrate"]);
                videoCaps = this->nearestDNxHDCaps(videoCaps);
                configs["bitrate"] = videoCaps.property("bitrate");
                videoCaps.setProperty("bitrate", QVariant());
                break;
            case AV_CODEC_ID_ROQ:
                videoCaps.width() = int(qPow(2, qRound(qLn(videoCaps.width()) / qLn(2))));
                videoCaps.height() = int(qPow(2, qRound(qLn(videoCaps.height()) / qLn(2))));
                videoCaps.fps() = AkFrac(qRound(videoCaps.fps().value()), 1);
                break;
            case AV_CODEC_ID_RV10:
                videoCaps.width() = 16 * qRound(videoCaps.width() / 16.);
            case AV_CODEC_ID_AMV:
                videoCaps.height() = 16 * qRound(videoCaps.height() / 16.);
                break;
            case AV_CODEC_ID_XFACE:
                videoCaps.width() = 48;
                videoCaps.height() = 48;
                break;
            default:
                break;
            }

            if (!strcmp(this->m_formatContext->oformat->name, "gxf"))
                videoCaps = this->nearestGXFCaps(videoCaps);

            stream->codec->bit_rate = configs["bitrate"].toInt();

            QString pixelFormat = AkVideoCaps::pixelFormatToString(videoCaps.format());
            stream->codec->pix_fmt = av_get_pix_fmt(pixelFormat.toStdString().c_str());
            stream->codec->width = videoCaps.width();
            stream->codec->height = videoCaps.height();

            AkFrac timeBase(configs["timeBase"].value<AkFrac>());
            stream->time_base.num = int(timeBase.num());
            stream->time_base.den = int(timeBase.den());
            stream->codec->time_base = stream->time_base;

            stream->codec->gop_size = configs["gop"].toInt();
        } else if (streamCaps.mimeType() == "text/x-raw") {
        }

        // Set codec options.
        AVDictionary *options = NULL;
        QVariantMap codecOptions = configs.value("codecOptions").toMap();

        foreach (QString key, codecOptions.keys()) {
            QString value = codecOptions[key].toString();

            av_dict_set(&options,
                        key.toStdString().c_str(),
                        value.toStdString().c_str(),
                        0);
        }

        // Open stream.
        int error = avcodec_open2(stream->codec, codec, &options);
        av_dict_free(&options);

        if (error < 0) {
            char errorStr[1024];
            av_strerror(AVERROR(error), errorStr, 1024);
            qDebug() << "Can't open codec " << codec->name << ": " << errorStr;
            avformat_free_context(this->m_formatContext);
            this->m_formatContext = NULL;

            return false;
        }

        this->m_streamParams << OutputParams(configs["index"].toInt());
    }

    // Print recording information.
    av_dump_format(this->m_formatContext,
                   0,
                   this->m_location.toStdString().c_str(),
                   1);

    // Open file.
    if (!(this->m_formatContext->oformat->flags & AVFMT_NOFILE)) {
        int error = avio_open(&this->m_formatContext->pb,
                              this->m_location.toStdString().c_str(),
                              AVIO_FLAG_READ_WRITE);

        if (error < 0) {
            char errorStr[1024];
            av_strerror(AVERROR(error), errorStr, 1024);
            qDebug() << "Can't open output file: " << errorStr;

            for (uint i = 0; i < this->m_formatContext->nb_streams; i++)
                avcodec_close(this->m_formatContext->streams[i]->codec);

            avformat_free_context(this->m_formatContext);
            this->m_formatContext = NULL;

            return false;
        }
    }


    // Set format options.
    AVDictionary *formatOptions = NULL;

    foreach (QString key, this->m_formatOptions.keys()) {
        QString value = this->m_formatOptions[key].toString();

        av_dict_set(&formatOptions,
                    key.toStdString().c_str(),
                    value.toStdString().c_str(),
                    0);
    }

    // Write file header.
    int error = avformat_write_header(this->m_formatContext, &formatOptions);
    av_dict_free(&formatOptions);

    if (error < 0) {
        char errorStr[1024];
        av_strerror(AVERROR(error), errorStr, 1024);
        qDebug() << "Can't write header: " << errorStr;

        if (!(this->m_formatContext->oformat->flags & AVFMT_NOFILE))
            // Close the output file.
            avio_close(this->m_formatContext->pb);

        for (uint i = 0; i < this->m_formatContext->nb_streams; i++)
            avcodec_close(this->m_formatContext->streams[i]->codec);

        avformat_free_context(this->m_formatContext);
        this->m_formatContext = NULL;

        return false;
    }

    this->m_audioPackets.clear();
    this->m_videoPackets.clear();
    this->m_subtitlePackets.clear();

    this->m_runAudioLoop = true;
    this->m_audioLoopResult = QtConcurrent::run(&this->m_threadPool, this->writeAudioLoop, this);
    this->m_runVideoLoop = true;
    this->m_videoLoopResult = QtConcurrent::run(&this->m_threadPool, this->writeVideoLoop, this);
    this->m_runSubtitleLoop = true;
    this->m_subtitleLoopResult = QtConcurrent::run(&this->m_threadPool, this->writeSubtitleLoop, this);
    this->m_isRecording = true;

    return true;
}

void MediaSink::uninit()
{
    if (!this->m_formatContext)
        return;

    this->m_isRecording = false;

    this->m_runAudioLoop = false;
    this->m_audioLoopResult.waitForFinished();
    this->m_runVideoLoop = false;
    this->m_videoLoopResult.waitForFinished();
    this->m_runSubtitleLoop = false;
    this->m_subtitleLoopResult.waitForFinished();

    this->m_audioPackets.clear();
    this->m_videoPackets.clear();
    this->m_subtitlePackets.clear();

    // Write remaining frames in the file.
    this->flushStreams();
    this->m_streamParams.clear();

    // Write the trailer, if any. The trailer must be written before you
    // close the CodecContexts open when you wrote the header; otherwise
    // av_write_trailer() may try to use memory that was freed on
    // av_codec_close().
    av_write_trailer(this->m_formatContext);

    if (!(this->m_formatContext->oformat->flags & AVFMT_NOFILE))
        // Close the output file.
        avio_close(this->m_formatContext->pb);

    for (uint i = 0; i < this->m_formatContext->nb_streams; i++)
        avcodec_close(this->m_formatContext->streams[i]->codec);

    avformat_free_context(this->m_formatContext);
    this->m_formatContext = NULL;
}

void MediaSink::writeAudioPacket(const AkAudioPacket &packet)
{
    if (!this->m_formatContext)
        return;

    int streamIndex = -1;

    for (int i = 0; i < this->m_streamParams.size(); i++)
        if (this->m_streamParams[i].inputIndex() == packet.index()) {
            streamIndex = i;

            break;
        }

    if (streamIndex < 0)
        return;

    AVStream *stream = this->m_formatContext->streams[streamIndex];
    AVCodecContext *codecContext = stream->codec;

    AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));
    iFrame.format = codecContext->sample_fmt;
    iFrame.channels = codecContext->channels;
    iFrame.channel_layout = codecContext->channel_layout;
    iFrame.sample_rate = codecContext->sample_rate;

    if (!this->m_streamParams[streamIndex].convert(packet, &iFrame)) {
        av_frame_unref(&iFrame);

        return;
    }

    AkFrac outTimeBase(codecContext->time_base.num,
                       codecContext->time_base.den);
    qint64 pts = qRound64(packet.pts()
                        * packet.timeBase().value()
                        / outTimeBase.value());
    iFrame.pts = iFrame.pkt_pts = pts;
    this->m_streamParams[streamIndex].addAudioSamples(&iFrame, packet.id());

    int outSamples = codecContext->codec->capabilities
                     & AV_CODEC_CAP_VARIABLE_FRAME_SIZE?
                        iFrame.nb_samples:
                        codecContext->frame_size;

    av_frame_unref(&iFrame);

    forever {
        pts = this->m_streamParams[streamIndex].audioPts();
        uint8_t *buffer = NULL;
        int bufferSize = this->m_streamParams[streamIndex].readAudioSamples(outSamples, &buffer);

        if (bufferSize < 1)
            break;

        AVFrame oFrame;
        memset(&oFrame, 0, sizeof(AVFrame));
        oFrame.format = codecContext->sample_fmt;
        oFrame.channels = codecContext->channels;
        oFrame.channel_layout = codecContext->channel_layout;
        oFrame.sample_rate = codecContext->sample_rate;
        oFrame.nb_samples = outSamples;
        oFrame.pts = oFrame.pkt_pts = pts;

        if (avcodec_fill_audio_frame(&oFrame,
                                     codecContext->channels,
                                     codecContext->sample_fmt,
                                     buffer,
                                     bufferSize,
                                     1) < 0) {
            delete [] buffer;

            continue;
        }

        // Initialize audio packet.
        AVPacket pkt;
        memset(&pkt, 0, sizeof(AVPacket));
        av_init_packet(&pkt);

        // Compress audio packet.
        int gotPacket;
        int result = avcodec_encode_audio2(codecContext,
                                           &pkt,
                                           &oFrame,
                                           &gotPacket);

        if (result < 0) {
            char error[1024];
            av_strerror(result, error, 1024);
            qDebug() << "Error: " << error;
            delete [] buffer;

            break;
        }

        if (!gotPacket) {
            delete [] buffer;

            continue;
        }

        pkt.stream_index = streamIndex;
        av_packet_rescale_ts(&pkt, codecContext->time_base, stream->time_base);

        // Write audio packet.
        this->m_writeMutex.lock();
        av_interleaved_write_frame(this->m_formatContext, &pkt);
        this->m_writeMutex.unlock();

        delete [] buffer;
    }
}

void MediaSink::writeVideoPacket(const AkVideoPacket &packet)
{
    if (!this->m_formatContext)
        return;

    int streamIndex = -1;

    for (int i = 0; i < this->m_streamParams.size(); i++)
        if (this->m_streamParams[i].inputIndex() == packet.index()) {
            streamIndex = i;

            break;
        }

    if (streamIndex < 0)
        return;

    AVStream *stream = this->m_formatContext->streams[streamIndex];
    AVCodecContext *codecContext = stream->codec;

    AVFrame oFrame;
    memset(&oFrame, 0, sizeof(AVFrame));
    oFrame.format = codecContext->pix_fmt;
    oFrame.width = codecContext->width;
    oFrame.height = codecContext->height;

    AkPacket videoPacket = packet.toPacket();
    QImage image = AkUtils::packetToImage(videoPacket);
    image = image.convertToFormat(QImage::Format_RGB32);
    image = this->swapChannels(image);
    videoPacket = AkUtils::imageToPacket(image, videoPacket);

    if (!this->m_streamParams[streamIndex].convert(videoPacket, &oFrame)) {
        av_frame_unref(&oFrame);

        return;
    }

    AkFrac outTimeBase(codecContext->time_base.num,
                       codecContext->time_base.den);

    qint64 pts = qRound64(packet.pts()
                        * packet.timeBase().value()
                        / outTimeBase.value());

    oFrame.pts = oFrame.pkt_pts =
            this->m_streamParams[streamIndex].nextPts(pts, packet.id());

    if (oFrame.pts < 0) {
        av_frame_unref(&oFrame);

        return;
    }

    AVPacket pkt;
    av_init_packet(&pkt);

    if (this->m_formatContext->oformat->flags & AVFMT_RAWPICTURE) {
        // Raw video case - directly store the picture in the packet
        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.data = oFrame.data[0];
        pkt.size = sizeof(AVPicture);
        pkt.pts = oFrame.pts;
        pkt.stream_index = streamIndex;

        av_packet_rescale_ts(&pkt, codecContext->time_base, stream->time_base);

        this->m_writeMutex.lock();
        av_interleaved_write_frame(this->m_formatContext, &pkt);
        this->m_writeMutex.unlock();
    } else {
        // encode the image
        pkt.data = NULL; // packet data will be allocated by the encoder
        pkt.size = 0;

        int gotPacket;

        if (avcodec_encode_video2(stream->codec,
                                  &pkt,
                                  &oFrame,
                                  &gotPacket) < 0) {
            av_frame_unref(&oFrame);

            return;
        }

        // If size is zero, it means the image was buffered.
        if (gotPacket) {
            pkt.stream_index = streamIndex;
            av_packet_rescale_ts(&pkt, codecContext->time_base, stream->time_base);

            // Write the compressed frame to the media file.
            this->m_writeMutex.lock();
            av_interleaved_write_frame(this->m_formatContext, &pkt);
            this->m_writeMutex.unlock();
        }
    }

    av_frame_unref(&oFrame);
}

void MediaSink::writeSubtitlePacket(const AkPacket &packet)
{
    Q_UNUSED(packet)
    // TODO: Implement this.
}

void MediaSink::updateStreams()
{
    QList<QVariantMap> streamConfigs = this->m_streamConfigs;
    this->clearStreams();

    foreach (QVariantMap configs, streamConfigs) {
        AkCaps caps = configs["caps"].value<AkCaps>();
        int index = configs["index"].toInt();
        this->addStream(index, caps, configs);
    }
}
