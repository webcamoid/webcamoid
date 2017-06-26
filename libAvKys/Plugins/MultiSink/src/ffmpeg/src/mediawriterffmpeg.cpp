/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include "mediawriterffmpeg.h"

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
typedef QVector<AkVideoCaps> VectorVideoCaps;
typedef QMap<AVOptionType, QString> OptionTypeStrMap;
typedef QMap<QString, QMap<AVMediaType, QStringList>> SupportedCodecsType;

class MediaWriterFFmpegGlobal
{
    public:
        AvMediaTypeStrMap m_mediaTypeToStr;
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

        MediaWriterFFmpegGlobal()
        {
            av_register_all();
            avcodec_register_all();
            avformat_network_init();

#ifndef QT_DEBUG
            av_log_set_level(AV_LOG_QUIET);
#endif

            this->m_mediaTypeToStr = this->initAvMediaTypeStrMap();
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

        inline AvMediaTypeStrMap initAvMediaTypeStrMap()
        {
            const AvMediaTypeStrMap mediaTypeToStr = {
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

        inline VectorVideoCaps initDVSupportedCaps()
        {
            const QStringList supportedCaps = {
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

        inline VectorVideoCaps initDNxHDSupportedCaps()
        {
            const QStringList supportedCaps = {
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

        inline QVector<QSize> initH261SupportedSize()
        {
            const QVector<QSize> supportedSize = {
                QSize(352, 288),
                QSize(176, 144)
            };

            return supportedSize;
        }

        inline QVector<QSize> initH263SupportedSize()
        {
            const QVector<QSize> supportedSize = {
                QSize(1408, 1152),
                QSize(704, 576),
                QSize(352, 288),
                QSize(176, 144),
                QSize(128, 96)
            };

            return supportedSize;
        }

        inline QVector<QSize> initGXFSupportedSize()
        {
            const QVector<QSize> supportedSize = {
                QSize(768, 576), // PAL
                QSize(640, 480)  // NTSC
            };

            return supportedSize;
        }

        inline QVector<int> initSWFSupportedSampleRates()
        {
            const QVector<int> supportedSampleRates = {
                44100,
                22050,
                11025
            };

            return supportedSampleRates;
        }

        inline bool initHasCudaSupport()
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

        inline OptionTypeStrMap initFFOptionTypeStrMap()
        {
            const OptionTypeStrMap optionTypeStrMap = {
                {AV_OPT_TYPE_FLAGS         , "flags"         },
                {AV_OPT_TYPE_INT           , "number"        },
                {AV_OPT_TYPE_INT64         , "number"        },
                {AV_OPT_TYPE_DOUBLE        , "number"        },
                {AV_OPT_TYPE_FLOAT         , "number"        },
                {AV_OPT_TYPE_STRING        , "string"        },
                {AV_OPT_TYPE_RATIONAL      , "frac"          },
                {AV_OPT_TYPE_BINARY        , "binary"        },
                {AV_OPT_TYPE_CONST         , "const"         },
#ifdef HAVE_EXTRAOPTIONS
                {AV_OPT_TYPE_DICT          , "dict"          },
                {AV_OPT_TYPE_IMAGE_SIZE    , "image_size"    },
                {AV_OPT_TYPE_PIXEL_FMT     , "pixel_fmt"     },
                {AV_OPT_TYPE_SAMPLE_FMT    , "sample_fmt"    },
                {AV_OPT_TYPE_VIDEO_RATE    , "video_rate"    },
                {AV_OPT_TYPE_DURATION      , "duration"      },
                {AV_OPT_TYPE_COLOR         , "color"         },
                {AV_OPT_TYPE_CHANNEL_LAYOUT, "channel_layout"},
                {AV_OPT_TYPE_BOOL          , "boolean"       },
#endif
            };

            return optionTypeStrMap;
        }

        inline SupportedCodecsType initSupportedCodecs()
        {
            SupportedCodecsType supportedCodecs;
            AVOutputFormat *outputFormat = NULL;

            while ((outputFormat = av_oformat_next(outputFormat))) {
                AVCodec *codec = NULL;

                while ((codec = av_codec_next(codec))) {
                    if (codec->capabilities & CODEC_CAP_EXPERIMENTAL
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
#ifdef HAVE_EXTRACODECFORMATS
                    } else if (!strcmp(outputFormat->name, "mp4")) {
                        if (codec->id == AV_CODEC_ID_VP9)
                            codecSupported = false;
#endif
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

                        supportedCodecs[outputFormat->name][codec->type] << codecName;
                    }
                }
            }

            return supportedCodecs;
        }

        inline QMap<QString, QVariantMap> initCodecDefaults()
        {
            QMap<QString, QVariantMap> codecDefaults;

            for (auto codec = av_codec_next(NULL);
                 codec;
                 codec = av_codec_next(codec)) {
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

                    QStringList supportedSampleFormats;

                    if (codec->sample_fmts)
                        for (int i = 0; ; i++) {
                            AVSampleFormat sampleFormat = codec->sample_fmts[i];

                            if (sampleFormat == AV_SAMPLE_FMT_NONE)
                                break;

                            supportedSampleFormats << QString(av_get_sample_fmt_name(sampleFormat));
                        }

                    QStringList supportedChannelLayouts;
                    char layout[1024];

                    if (codec->channel_layouts)
                        for (int i = 0; uint64_t channelLayout = codec->channel_layouts[i]; i++) {
                            int channels = av_get_channel_layout_nb_channels(channelLayout);
                            av_get_channel_layout_string(layout, 1024, channels, channelLayout);
                            supportedChannelLayouts << QString(layout);
                        }

                    if (supportedChannelLayouts.isEmpty())
                        switch (codec->id) {
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

                    switch (codec->id) {
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
                    codecParams["defaultSampleFormat"] =
                            codecContext->sample_fmt != AV_SAMPLE_FMT_NONE?
                                QString(av_get_sample_fmt_name(codecContext->sample_fmt)):
                                supportedSampleFormats.value(0, "s16");
                    codecParams["defaultBitRate"] =
                            codecContext->bit_rate?
                                qint64(codecContext->bit_rate): 128000;
                    codecParams["defaultSampleRate"] =
                            codecContext->sample_rate?
                                codecContext->sample_rate:
                                supportedSampleRates.value(0, 44100);

                    int channels =
                            av_get_channel_layout_nb_channels(codecContext->channel_layout);
                    av_get_channel_layout_string(layout,
                                                 1024,
                                                 channels,
                                                 codecContext->channel_layout);

                    QString channelLayout = codecContext->channel_layout?
                                                QString(layout):
                                                supportedChannelLayouts.value(0, "mono");

                    codecParams["defaultChannelLayout"] = channelLayout;

                    int channelsCount =
                            av_get_channel_layout_nb_channels(av_get_channel_layout(channelLayout.toStdString().c_str()));

                    codecParams["defaultChannels"] = codecContext->channels?
                                                         codecContext->channels:
                                                         channelsCount;
                } else if (codec->type == AVMEDIA_TYPE_VIDEO) {
                    QVariantList supportedFrameRates;

                    if (codec->supported_framerates)
                        for (int i = 0; ; i++) {
                            AVRational frameRate = codec->supported_framerates[i];

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

                    QStringList supportedPixelFormats;

                    if (codec->pix_fmts)
                        for (int i = 0; ; i++) {
                            AVPixelFormat pixelFormat = codec->pix_fmts[i];

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

                codecDefaults[codec->name] = codecParams;

                av_free(codecContext);
            }

            return codecDefaults;
        }
};

Q_GLOBAL_STATIC(MediaWriterFFmpegGlobal, mediaWriterFFmpegGlobal)

MediaWriterFFmpeg::MediaWriterFFmpeg(QObject *parent):
    MediaWriter(parent)
{
    this->m_formatContext = NULL;
    this->m_packetQueueSize = 0;
    this->m_maxPacketQueueSize = 15 * 1024 * 1024;
    this->m_runAudioLoop = false;
    this->m_runVideoLoop = false;
    this->m_runSubtitleLoop = false;
    this->m_isRecording = false;

    this->m_codecsBlackList = QStringList {
        // This codec fail.
        "vc2",

        // The codecs are too slow for real time recording.
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
}

MediaWriterFFmpeg::~MediaWriterFFmpeg()
{
    this->uninit();
    avformat_network_deinit();
}

QString MediaWriterFFmpeg::outputFormat() const
{
    return this->m_outputFormat;
}

QVariantList MediaWriterFFmpeg::streams() const
{
    QVariantList streams;

    for (const QVariantMap &stream: this->m_streamConfigs)
        streams << stream;

    return streams;
}

qint64 MediaWriterFFmpeg::maxPacketQueueSize() const
{
    return this->m_maxPacketQueueSize;
}

QStringList MediaWriterFFmpeg::supportedFormats()
{
    QStringList formats;

    for (auto &format: mediaWriterFFmpegGlobal->m_supportedCodecs.keys())
        if (!this->m_formatsBlackList.contains(format))
            formats << format;

    return formats;
}

QStringList MediaWriterFFmpeg::fileExtensions(const QString &format)
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

QString MediaWriterFFmpeg::formatDescription(const QString &format)
{
    AVOutputFormat *outputFormat = av_guess_format(format.toStdString().c_str(),
                                                   NULL,
                                                   NULL);

    if (!outputFormat)
        return QString();

    return QString(outputFormat->long_name);
}

QVariantList MediaWriterFFmpeg::formatOptions()
{
    auto outFormat = this->guessFormat();

    if (outFormat.isEmpty())
        return QVariantList();

    AVOutputFormat *outputFormat = av_guess_format(outFormat.toStdString().c_str(),
                                                   NULL,
                                                   NULL);

    if (!outputFormat)
        return QVariantList();

    auto options = this->parseOptions(outputFormat->priv_class);
    auto globalFormatOptions = this->m_formatOptions.value(outFormat);
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
    return this->supportedCodecs(format, "");
}

QStringList MediaWriterFFmpeg::supportedCodecs(const QString &format,
                                               const QString &type)
{
    QStringList supportedCodecs;

    if (type.isEmpty()) {
        for (auto &codecs: mediaWriterFFmpegGlobal->m_supportedCodecs.value(format)) {
            for (auto &codec: codecs)
                if (!this->m_codecsBlackList.contains(codec))
                    supportedCodecs << codec;
        }
    } else {
        auto codecType = mediaWriterFFmpegGlobal->m_mediaTypeToStr.key(type, AVMEDIA_TYPE_UNKNOWN);
        auto codecs = mediaWriterFFmpegGlobal->m_supportedCodecs.value(format).value(codecType);

        for (auto &codec: codecs)
            if (!this->m_codecsBlackList.contains(codec))
                supportedCodecs << codec;
    }

    return supportedCodecs;
}

QString MediaWriterFFmpeg::defaultCodec(const QString &format,
                                        const QString &type)
{
    auto outputFormat = av_guess_format(format.toStdString().c_str(),
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
#ifdef HAVE_EXTRACODECFORMATS
    if (codecId == AV_CODEC_ID_VP9)
        codecId = AV_CODEC_ID_VP8;
#endif
    AVCodec *codec = avcodec_find_encoder(codecId);
    QString codecName(codec->name);

    QStringList supportedCodecs = this->supportedCodecs(format, type);

    if (supportedCodecs.isEmpty())
        return QString();

    if (!supportedCodecs.contains(codecName))
        codecName = supportedCodecs.first();

    return codecName;
}

QString MediaWriterFFmpeg::codecDescription(const QString &codec)
{
    auto avCodec = avcodec_find_encoder_by_name(codec.toStdString().c_str());

    if (!avCodec)
        return QString();

    return QString(avCodec->long_name);
}

QString MediaWriterFFmpeg::codecType(const QString &codec)
{
    auto avCodec = avcodec_find_encoder_by_name(codec.toStdString().c_str());

    if (!avCodec)
        return QString();

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
    QString outputFormat = this->guessFormat();

    if (outputFormat.isEmpty())
        return QVariantMap();

    QVariantMap outputParams;

    if (codecParams.contains("label"))
        outputParams["label"] = codecParams["label"];

    outputParams["index"] = streamIndex;
    auto codec = codecParams.value("codec").toString();

    if (codec.isEmpty())
        return QVariantMap();

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

    this->m_streamConfigs << outputParams;
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
    QString outputFormat = this->guessFormat();

    if (outputFormat.isEmpty())
        return QVariantMap();

    bool streamChanged = false;

    if (codecParams.contains("label")
        && this->m_streamConfigs[index]["label"] != codecParams.value("label")) {
        this->m_streamConfigs[index]["label"] = codecParams.value("label");
        streamChanged |= true;
    }

    AkCaps streamCaps = this->m_streamConfigs[index]["caps"].value<AkCaps>();

    if (codecParams.contains("caps")
        && this->m_streamConfigs[index]["caps"] != codecParams.value("caps")) {
        this->m_streamConfigs[index]["caps"] = codecParams.value("caps");
        streamChanged |= true;
    }

    QString codec;

    if (codecParams.contains("codec")) {
        if (this->supportedCodecs(outputFormat, streamCaps.mimeType())
            .contains(codecParams["codec"].toString())) {
            codec = codecParams["codec"].toString();
        } else
            codec = this->defaultCodec(outputFormat, streamCaps.mimeType());

        this->m_streamConfigs[index]["codec"] = codec;
        streamChanged |= true;
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

    if (streamChanged)
        emit this->streamsChanged(this->streams());

    return this->m_streamConfigs[index];
}

QVariantList MediaWriterFFmpeg::codecOptions(int index)
{
    auto outputFormat = this->guessFormat();

    if (outputFormat.isEmpty())
        return QVariantList();

    auto codec = this->m_streamConfigs.value(index).value("codec").toString();

    if (codec.isEmpty())
        return QVariantList();

    auto avCodec = avcodec_find_encoder_by_name(codec.toStdString().c_str());

    if (!avCodec)
        return QVariantList();

    auto optKey = QString("%1/%2/%3").arg(outputFormat).arg(index).arg(codec);
    auto options = this->parseOptions(avCodec->priv_class);
    auto globalCodecOptions = this->m_codecOptions.value(optKey);
    QVariantList codecOptions;

    if (codec == "libvpx") {
        quint8 r = 0;

        for (int i = 0; i < options.size(); i++) {
            auto option = options[i].toList();

            if (option[0] == "deadline") {
                option[6] = option[7] = "realtime";
                options[i] = option;
                r |= 0x1;
            } else if (option[0] == "quality") {
                option[6] = option[7] = "realtime";
                options[i] = option;
                r |= 0x2;
            }

            if (r > 2)
                break;
        }
    } else if (codec == "libx265") {
        for (int i = 0; i < options.size(); i++) {
            auto option = options[i].toList();

            if (option[0] == "preset") {
                option[6] = option[7] = "ultrafast";
                options[i] = option;

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

void MediaWriterFFmpeg::flushStreams()
{
    for (uint i = 0; i < this->m_formatContext->nb_streams; i++) {
        AVStream *stream = this->m_formatContext->streams[i];
        auto codecContext = this->m_streamParams[int(i)].codecContext();
        AVMediaType mediaType = codecContext->codec_type;

        if (mediaType == AVMEDIA_TYPE_AUDIO) {
            if (codecContext->frame_size <= 1)
                continue;

            qint64 pts = this->m_streamParams[int(i)].audioPts();
            int ptsDiff = codecContext->codec->capabilities
                          & CODEC_CAP_VARIABLE_FRAME_SIZE?
                              1:
#ifdef HAVE_CODECPAR
                              stream->codecpar->frame_size;
#else
                              stream->codec->frame_size;
#endif

#ifdef HAVE_SENDRECV
            if (avcodec_send_frame(codecContext.data(), NULL) < 0)
                continue;
#endif

            forever {
                AVPacket pkt;
                av_init_packet(&pkt);
                pkt.data = NULL;
                pkt.size = 0;

#ifdef HAVE_SENDRECV
                auto error = avcodec_receive_packet(codecContext.data(), &pkt);

                if (error < 0) {
                    if (error != AVERROR_EOF) {
                        char errorStr[1024];
                        av_strerror(AVERROR(error), errorStr, 1024);
                        qDebug() << "Error encoding packets: " << errorStr;
                    }

                    break;
                }
#else
                int gotPacket;

                if (avcodec_encode_audio2(codecContext.data(),
                                          &pkt,
                                          NULL,
                                          &gotPacket) < 0)
                    break;

                if (!gotPacket)
                    break;
#endif
                pkt.pts = pkt.dts = pts;
                pts += ptsDiff;
                this->rescaleTS(&pkt,
                                codecContext->time_base,
                                stream->time_base);
                pkt.stream_index = int(i);
                av_interleaved_write_frame(this->m_formatContext, &pkt);
#ifdef HAVE_PACKETREF
                av_packet_unref(&pkt);
#else
                av_destruct_packet(&pkt);
#endif
                auto eventDispatcher = QThread::currentThread()->eventDispatcher();

                if (eventDispatcher)
                    eventDispatcher->processEvents(QEventLoop::AllEvents);
            }
        } else if (mediaType == AVMEDIA_TYPE_VIDEO) {
            if (this->m_formatContext->oformat->flags & AVFMT_RAWPICTURE
                && codecContext->codec->id == AV_CODEC_ID_RAWVIDEO)
                continue;

#ifdef HAVE_SENDRECV
            if (avcodec_send_frame(codecContext.data(), NULL) < 0)
                continue;
#endif

            forever {
                AVPacket pkt;
                av_init_packet(&pkt);
                pkt.data = NULL;
                pkt.size = 0;

#ifdef HAVE_SENDRECV
                auto error = avcodec_receive_packet(codecContext.data(), &pkt);

                if (error < 0) {
                    if (error != AVERROR_EOF) {
                        char errorStr[1024];
                        av_strerror(AVERROR(error), errorStr, 1024);
                        qDebug() << "Error encoding packets: " << errorStr;
                    }

                    break;
                }
#else
                int gotPacket;

                if (avcodec_encode_video2(codecContext.data(),
                                          &pkt,
                                          NULL,
                                          &gotPacket) < 0)
                    break;

                if (!gotPacket)
                    break;
#endif

                pkt.pts = pkt.dts = this->m_streamParams[int(i)].nextPts(0, 0);
                this->rescaleTS(&pkt,
                                codecContext->time_base,
                                stream->time_base);
                pkt.stream_index = int(i);
                av_interleaved_write_frame(this->m_formatContext, &pkt);
#ifdef HAVE_PACKETREF
                av_packet_unref(&pkt);
#else
                av_destruct_packet(&pkt);
#endif
                auto eventDispatcher = QThread::currentThread()->eventDispatcher();

                if (eventDispatcher)
                    eventDispatcher->processEvents(QEventLoop::AllEvents);
            }
        }
    }
}

QImage MediaWriterFFmpeg::swapChannels(const QImage &image) const
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

QString MediaWriterFFmpeg::guessFormat()
{
    QString outputFormat;

    if (this->supportedFormats().contains(this->m_outputFormat))
        outputFormat = this->m_outputFormat;
    else {
        auto format =
                av_guess_format(NULL,
                                this->m_location.toStdString().c_str(),
                                NULL);

        if (format)
            outputFormat = QString(format->name);
    }

    return outputFormat;
}

QVariantList MediaWriterFFmpeg::parseOptions(const AVClass *avClass) const
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
#ifdef HAVE_EXTRAOPTIONS
            case AV_OPT_TYPE_PIXEL_FMT:
            case AV_OPT_TYPE_SAMPLE_FMT:
            case AV_OPT_TYPE_DURATION:
            case AV_OPT_TYPE_CHANNEL_LAYOUT:
            case AV_OPT_TYPE_BOOL:
#endif
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
#ifdef HAVE_EXTRAOPTIONS
            case AV_OPT_TYPE_IMAGE_SIZE: {
                int width = 0;
                int height = 0;

                if (av_parse_video_size(&width, &height, option->default_val.str) < 0)
                    value = QSize();

                value = QSize(width, height);
                break;
            }
            case AV_OPT_TYPE_VIDEO_RATE: {
                AVRational rate;

                if (av_parse_video_rate(&rate, option->default_val.str) < 0)
                    value = QVariant::fromValue(AkFrac());

                value = QVariant::fromValue(AkFrac(rate.num, rate.den));
                break;
            }
            case AV_OPT_TYPE_COLOR: {
                uint8_t color[4];

                if (av_parse_color(color, option->default_val.str, -1, NULL) < 0)
                    value = qRgba(0, 0, 0, 0);

                value = qRgba(color[0], color[1], color[2], color[3]);
                break;
            }
#endif
            case AV_OPT_TYPE_RATIONAL:
                value = AkFrac(option->default_val.q.num,
                               option->default_val.q.den).toString();
                break;
            default:
                continue;
        }

        if (option->type == AV_OPT_TYPE_CONST) {
            QVariantList menuOption = {option->name, option->help, value};

            if (menu.contains(option->unit)) {
                menu[option->unit] << QVariant(menuOption);
            } else
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

    for (auto option: avOptions) {
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

AVDictionary *MediaWriterFFmpeg::formatContextOptions(AVFormatContext *formatContext,
                                                      const QVariantMap &options)
{
    auto avClass = formatContext->oformat->priv_class;
    QStringList flagType;

    if (avClass)
        for (auto option = avClass->option;
             option;
             option = av_opt_next(&avClass, option)) {
            if (option->type == AV_OPT_TYPE_FLAGS)
                flagType << option->name;
        }

    AVDictionary *formatOptions = NULL;

    for (const QString &key: options.keys()) {
        QString value;

        if (flagType.contains(key)) {
            auto flags = options[key].toStringList();
            value = flags.join('+');
        } else {
            value = options[key].toString();
        }

        av_dict_set(&formatOptions,
                    key.toStdString().c_str(),
                    value.toStdString().c_str(),
                    0);
    }

    return formatOptions;
}

AkVideoCaps MediaWriterFFmpeg::nearestDVCaps(const AkVideoCaps &caps) const
{
    AkVideoCaps nearestCaps;
    qreal q = std::numeric_limits<qreal>::max();

    for (const AkVideoCaps &sCaps: mediaWriterFFmpegGlobal->m_dvSupportedCaps) {
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

AkVideoCaps MediaWriterFFmpeg::nearestDNxHDCaps(const AkVideoCaps &caps) const
{
    AkVideoCaps nearestCaps;
    qreal q = std::numeric_limits<qreal>::max();

    for (const AkVideoCaps &sCaps: mediaWriterFFmpegGlobal->m_dnXhdSupportedCaps) {
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

AkVideoCaps MediaWriterFFmpeg::nearestH261Caps(const AkVideoCaps &caps) const
{
    QSize nearestSize;
    qreal q = std::numeric_limits<qreal>::max();

    for (const QSize &size: mediaWriterFFmpegGlobal->m_h261SupportedSize) {
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

AkVideoCaps MediaWriterFFmpeg::nearestH263Caps(const AkVideoCaps &caps) const
{
    QSize nearestSize;
    qreal q = std::numeric_limits<qreal>::max();

    for (const QSize &size: mediaWriterFFmpegGlobal->m_h263SupportedSize) {
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

AkVideoCaps MediaWriterFFmpeg::nearestGXFCaps(const AkVideoCaps &caps) const
{
    QSize nearestSize;
    qreal q = std::numeric_limits<qreal>::max();

    for (const QSize &size: mediaWriterFFmpegGlobal->m_gxfSupportedSize) {
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

AkAudioCaps MediaWriterFFmpeg::nearestSWFCaps(const AkAudioCaps &caps) const
{
    int nearestSampleRate = 0;
    int q = std::numeric_limits<int>::max();

    for (const int &sampleRate: mediaWriterFFmpegGlobal->m_swfSupportedSampleRates) {
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

void MediaWriterFFmpeg::writeAudioLoop(MediaWriterFFmpeg *self)
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

void MediaWriterFFmpeg::writeVideoLoop(MediaWriterFFmpeg *self)
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

void MediaWriterFFmpeg::writeSubtitleLoop(MediaWriterFFmpeg *self)
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

void MediaWriterFFmpeg::decreasePacketQueue(int packetSize)
{
    this->m_packetMutex.lock();
    this->m_packetQueueSize -= packetSize;

    if (this->m_packetQueueSize <= this->m_maxPacketQueueSize)
        this->m_packetQueueNotFull.wakeAll();

    this->m_packetMutex.unlock();
}

void MediaWriterFFmpeg::deleteFrame(AVFrame *frame)
{
    av_freep(&frame->data[0]);
    frame->data[0] = NULL;

#ifdef HAVE_FRAMEALLOC
    av_frame_unref(frame);
#endif
}

void MediaWriterFFmpeg::rescaleTS(AVPacket *pkt,
                                  AVRational src,
                                  AVRational dst)
{
#ifdef HAVE_RESCALETS
    av_packet_rescale_ts(pkt, src, dst);
#else
    if (pkt->pts != AV_NOPTS_VALUE)
        pkt->pts = av_rescale_q(pkt->pts, src, dst);

    if (pkt->dts != AV_NOPTS_VALUE)
        pkt->dts = av_rescale_q(pkt->dts, src, dst);

    if (pkt->duration > 0)
        pkt->duration = av_rescale_q(pkt->duration, src, dst);
#endif
}

void MediaWriterFFmpeg::setOutputFormat(const QString &outputFormat)
{
    if (this->m_outputFormat == outputFormat)
        return;

    this->m_outputFormat = outputFormat;
    emit this->outputFormatChanged(outputFormat);
}

void MediaWriterFFmpeg::setFormatOptions(const QVariantMap &formatOptions)
{
    auto outputFormat = this->guessFormat();
    bool modified = false;

    for (auto &key: formatOptions.keys())
        if (formatOptions[key] != this->m_formatOptions.value(outputFormat).value(key)) {
            this->m_formatOptions[outputFormat][key] = formatOptions[key];
            modified = true;
        }

    if (modified)
        emit this->formatOptionsChanged(this->m_formatOptions.value(outputFormat));
}

void MediaWriterFFmpeg::setCodecOptions(int index,
                                        const QVariantMap &codecOptions)
{
    auto outputFormat = this->guessFormat();

    if (outputFormat.isEmpty())
        return;

    auto codec = this->m_streamConfigs.value(index).value("codec").toString();

    if (codec.isEmpty())
        return;

    auto optKey = QString("%1/%2/%3").arg(outputFormat).arg(index).arg(codec);
    bool modified = false;

    for (auto &key: codecOptions.keys())
        if (codecOptions[key] != this->m_codecOptions.value(optKey).value(key)) {
            this->m_codecOptions[optKey][key] = codecOptions[key];
            modified = true;
        }

    if (modified)
        emit this->codecOptionsChanged(optKey, this->m_formatOptions.value(optKey));
}

void MediaWriterFFmpeg::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    if (this->m_maxPacketQueueSize == maxPacketQueueSize)
        return;

    this->m_maxPacketQueueSize = maxPacketQueueSize;
    emit this->maxPacketQueueSizeChanged(maxPacketQueueSize);
}

void MediaWriterFFmpeg::resetOutputFormat()
{
    this->setOutputFormat("");
}

void MediaWriterFFmpeg::resetFormatOptions()
{
    auto outputFormat = this->guessFormat();

    if (this->m_formatOptions.value(outputFormat).isEmpty())
        return;

    this->m_formatOptions.remove(outputFormat);
    emit this->formatOptionsChanged(QVariantMap());
}

void MediaWriterFFmpeg::resetCodecOptions(int index)
{
    auto outputFormat = this->guessFormat();

    if (outputFormat.isEmpty())
        return;

    auto codec = this->m_streamConfigs.value(index).value("codec").toString();

    if (codec.isEmpty())
        return;

    auto optKey = QString("%1/%2/%3").arg(outputFormat).arg(index).arg(codec);

    if (this->m_codecOptions.value(optKey).isEmpty())
        return;

    this->m_codecOptions.remove(optKey);
    emit this->codecOptionsChanged(optKey, QVariantMap());
}

void MediaWriterFFmpeg::resetMaxPacketQueueSize()
{
    this->setMaxPacketQueueSize(15 * 1024 * 1024);
}

void MediaWriterFFmpeg::enqueuePacket(const AkPacket &packet)
{
    forever {
        if (!this->m_isRecording)
            return;

        this->m_packetMutex.lock();
        bool canEnqueue = true;

        if (this->m_packetQueueSize >= this->m_maxPacketQueueSize)
            canEnqueue =
                this->m_packetQueueNotFull.wait(&this->m_packetMutex,
                                                THREAD_WAIT_LIMIT);

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

void MediaWriterFFmpeg::clearStreams()
{
    this->m_streamConfigs.clear();
    emit this->streamsChanged(this->streams());
}

bool MediaWriterFFmpeg::init()
{
    auto outputFormat = this->guessFormat();

#ifdef HAVE_ALLOCOUTPUTCONTEXT
    if (avformat_alloc_output_context2(&this->m_formatContext,
                                       NULL,
                                       this->m_outputFormat.isEmpty()?
                                            NULL: this->m_outputFormat.toStdString().c_str(),
                                       this->m_location.toStdString().c_str()) < 0)
        return false;
#else
    this->m_formatContext = avformat_alloc_context();

    if (!this->m_formatContext)
        return false;

    this->m_formatContext->oformat =
            av_guess_format(this->m_outputFormat.isEmpty()?
                                NULL: this->m_outputFormat.toStdString().c_str(),
                            this->m_location.toStdString().c_str(),
                            NULL);

    if (!this->m_formatContext->oformat) {
        avformat_free_context(this->m_formatContext);
        this->m_formatContext = NULL;

        return false;
    }

    memset(this->m_formatContext->filename, 0, 1024);
    memcpy(this->m_formatContext->filename,
           this->m_location.toStdString().c_str(),
           size_t(this->m_location.size()));
#endif

    auto streamConfigs = this->m_streamConfigs.toVector();

    if (!strcmp(this->m_formatContext->oformat->name, "mxf_opatom")) {
        QList<QVariantMap> mxfConfigs;

        for (const QVariantMap &configs: streamConfigs) {
            AkCaps streamCaps = configs["caps"].value<AkCaps>();

            if (streamCaps.mimeType() == "video/x-raw") {
                mxfConfigs << configs;

                break;
            }
        }

        if (mxfConfigs.isEmpty())
            for (const QVariantMap &configs: streamConfigs) {
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
        auto defaultCodecParams = this->defaultCodecParams(codecName);

        AVCodec *codec = avcodec_find_encoder_by_name(codecName.toStdString().c_str());
        AVStream *stream = avformat_new_stream(this->m_formatContext, NULL);

        stream->id = i;
        auto codecContext = avcodec_alloc_context3(codec);

        // Some formats want stream headers to be separate.
        if (this->m_formatContext->oformat->flags & AVFMT_GLOBALHEADER)
            codecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;

        codecContext->strict_std_compliance = CODEC_COMPLIANCE;

        // Confihure streams parameters.
        AkCaps streamCaps = configs["caps"].value<AkCaps>();

        if (streamCaps.mimeType() == "audio/x-raw") {
            codecContext->bit_rate = configs["bitrate"].toInt();

            if (codecContext->bit_rate < 1)
                codecContext->bit_rate = defaultCodecParams["defaultBitRate"].toInt();

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

            AkAudioCaps audioCaps(streamCaps);

            QString sampleFormat = AkAudioCaps::sampleFormatToString(audioCaps.format());
            QStringList supportedSampleFormats = defaultCodecParams["supportedSampleFormats"].toStringList();

            if (!supportedSampleFormats.isEmpty() && !supportedSampleFormats.contains(sampleFormat)) {
                QString defaultSampleFormat = defaultCodecParams["defaultSampleFormat"].toString();
                audioCaps.format() = AkAudioCaps::sampleFormatFromString(defaultSampleFormat);
                audioCaps.bps() = 8 * av_get_bytes_per_sample(av_get_sample_fmt(defaultSampleFormat.toStdString().c_str()));
            }

            QVariantList supportedSampleRates = defaultCodecParams["supportedSampleRates"].toList();

            if (!supportedSampleRates.isEmpty()) {
                int sampleRate = 0;
                int maxDiff = std::numeric_limits<int>::max();

                for (const QVariant &rate: supportedSampleRates) {
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
            QStringList supportedChannelLayouts = defaultCodecParams["supportedChannelLayouts"].toStringList();

            if (!supportedChannelLayouts.isEmpty() && !supportedChannelLayouts.contains(channelLayout)) {
                QString defaultChannelLayout = defaultCodecParams["defaultChannelLayout"].toString();
                audioCaps.layout() = AkAudioCaps::channelLayoutFromString(defaultChannelLayout);
                audioCaps.channels() = av_get_channel_layout_nb_channels(av_get_channel_layout(defaultChannelLayout.toStdString().c_str()));
            }

            if (!strcmp(this->m_formatContext->oformat->name, "gxf")) {
                audioCaps.rate() = 48000;
                audioCaps.layout() = AkAudioCaps::Layout_mono;
                audioCaps.channels() = 1;
            } else if (!strcmp(this->m_formatContext->oformat->name, "mxf")) {
                audioCaps.rate() = 48000;
            } else if (!strcmp(this->m_formatContext->oformat->name, "swf")) {
                audioCaps = this->nearestSWFCaps(audioCaps);
            }

            QString sampleFormatStr = AkAudioCaps::sampleFormatToString(audioCaps.format());
            codecContext->sample_fmt = av_get_sample_fmt(sampleFormatStr.toStdString().c_str());
            codecContext->sample_rate = audioCaps.rate();
            QString layout = AkAudioCaps::channelLayoutToString(audioCaps.layout());
            codecContext->channel_layout = av_get_channel_layout(layout.toStdString().c_str());
            codecContext->channels = audioCaps.channels();

            stream->time_base.num = 1;
            stream->time_base.den = audioCaps.rate();
            codecContext->time_base = stream->time_base;
        } else if (streamCaps.mimeType() == "video/x-raw") {
            codecContext->bit_rate = configs["bitrate"].toInt();

            if (codecContext->bit_rate < 1)
                codecContext->bit_rate = defaultCodecParams["defaultBitRate"].toInt();

            AkVideoCaps videoCaps(streamCaps);

            QString pixelFormat = AkVideoCaps::pixelFormatToString(videoCaps.format());
            QStringList supportedPixelFormats = defaultCodecParams["supportedPixelFormats"].toStringList();

            if (!supportedPixelFormats.isEmpty() && !supportedPixelFormats.contains(pixelFormat)) {
                QString defaultPixelFormat = defaultCodecParams["defaultPixelFormat"].toString();
                videoCaps.format() = AkVideoCaps::pixelFormatFromString(defaultPixelFormat);
                videoCaps.bpp() = AkVideoCaps::bitsPerPixel(videoCaps.format());
            }

            QVariantList supportedFrameRates = defaultCodecParams["supportedFrameRates"].toList();

            if (!supportedFrameRates.isEmpty()) {
                AkFrac frameRate;
                qreal maxDiff = std::numeric_limits<qreal>::max();

                for (const QVariant &rate: supportedFrameRates) {
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
                codecContext->bit_rate = videoCaps.property("bitrate").toInt();
                videoCaps.setProperty("bitrate", QVariant());
                break;
            case AV_CODEC_ID_ROQ:
                videoCaps.width() = int(qPow(2, qRound(qLn(videoCaps.width()) / qLn(2))));
                videoCaps.height() = int(qPow(2, qRound(qLn(videoCaps.height()) / qLn(2))));
                videoCaps.fps() = AkFrac(qRound(videoCaps.fps().value()), 1);
                break;
            case AV_CODEC_ID_RV10:
                videoCaps.width() = 16 * qRound(videoCaps.width() / 16.);
                videoCaps.height() = 16 * qRound(videoCaps.height() / 16.);
                break;
            case AV_CODEC_ID_AMV:
                videoCaps.height() = 16 * qRound(videoCaps.height() / 16.);
                break;
#ifdef HAVE_EXTRACODECFORMATS
            case AV_CODEC_ID_XFACE:
                videoCaps.width() = 48;
                videoCaps.height() = 48;
                break;
#endif
            default:
                break;
            }

            if (!strcmp(this->m_formatContext->oformat->name, "gxf"))
                videoCaps = this->nearestGXFCaps(videoCaps);

            QString pixelFormatStr = AkVideoCaps::pixelFormatToString(videoCaps.format());
            codecContext->pix_fmt = av_get_pix_fmt(pixelFormatStr.toStdString().c_str());
            codecContext->width = videoCaps.width();
            codecContext->height = videoCaps.height();

            AkFrac timeBase = videoCaps.fps().invert();
            stream->time_base.num = int(timeBase.num());
            stream->time_base.den = int(timeBase.den());
            codecContext->time_base = stream->time_base;
            codecContext->gop_size = configs["gop"].toInt();

            if (codecContext->gop_size < 1)
                codecContext->gop_size = defaultCodecParams["defaultGOP"].toInt();
        } else if (streamCaps.mimeType() == "text/x-raw") {
        }

        // Set codec options.
        AVDictionary *options = NULL;
        auto optKey = QString("%1/%2/%3").arg(outputFormat).arg(i).arg(codecName);
        QVariantMap codecOptions = this->m_codecOptions.value(optKey);

        if (codecName == "libvpx") {
            if (!codecOptions.contains("deadline"))
                codecOptions["deadline"] = "realtime";

            if (!codecOptions.contains("quality"))
                codecOptions["quality"] = "realtime";
        } else if (codecName == "libx265") {
            if (!codecOptions.contains("preset"))
                codecOptions["preset"] = "ultrafast";
        }

        for (const QString &key: codecOptions.keys()) {
            QString value = codecOptions[key].toString();

            av_dict_set(&options,
                        key.toStdString().c_str(),
                        value.toStdString().c_str(),
                        0);
        }

        // Open stream.
        auto error = avcodec_open2(codecContext, codec, &options);
        av_dict_free(&options);

        if (error < 0) {
            char errorStr[1024];
            av_strerror(AVERROR(error), errorStr, 1024);
            qDebug() << "Can't open codec " << codec->name << ": " << errorStr;
#ifdef HAVE_FREECONTEXT
            avcodec_free_context(&codecContext);
#else
            avcodec_close(codecContext);
            av_free(codecContext);
#endif
            avformat_free_context(this->m_formatContext);
            this->m_formatContext = NULL;

            return false;
        }

#ifdef HAVE_CODECPAR
        avcodec_parameters_from_context(stream->codecpar, codecContext);
#else
        avcodec_copy_context(stream->codec, codecContext);
#endif
        this->m_streamParams << OutputParams(configs["index"].toInt(), codecContext);
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
                avcodec_close(this->m_streamParams[int(i)].codecContext().data());

            this->m_streamParams.clear();
            avformat_free_context(this->m_formatContext);
            this->m_formatContext = NULL;

            return false;
        }
    }

    // Set format options.
    auto formatOptions =
            this->formatContextOptions(this->m_formatContext,
                                       this->m_formatOptions.value(outputFormat));

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
            avcodec_close(this->m_streamParams[int(i)].codecContext().data());

        this->m_streamParams.clear();
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

void MediaWriterFFmpeg::uninit()
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

    // Write the trailer, if any. The trailer must be written before you
    // close the CodecContexts open when you wrote the header; otherwise
    // av_write_trailer() may try to use memory that was freed on
    // av_codec_close().
    av_write_trailer(this->m_formatContext);

    if (!(this->m_formatContext->oformat->flags & AVFMT_NOFILE))
        // Close the output file.
        avio_close(this->m_formatContext->pb);

    for (uint i = 0; i < this->m_formatContext->nb_streams; i++)
        avcodec_close(this->m_streamParams[int(i)].codecContext().data());

    this->m_streamParams.clear();
    avformat_free_context(this->m_formatContext);
    this->m_formatContext = NULL;
}

void MediaWriterFFmpeg::writeAudioPacket(const AkAudioPacket &packet)
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
    auto codecContext = this->m_streamParams[streamIndex].codecContext();

    AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));
    iFrame.format = codecContext->sample_fmt;
    iFrame.channel_layout = codecContext->channel_layout;
    iFrame.sample_rate = codecContext->sample_rate;

    if (!this->m_streamParams[streamIndex].convert(packet, &iFrame)) {
        this->deleteFrame(&iFrame);

        return;
    }

    AkFrac outTimeBase(codecContext->time_base.num,
                       codecContext->time_base.den);
    qint64 pts = qRound64(packet.pts()
                        * packet.timeBase().value()
                        / outTimeBase.value());
    iFrame.pts = pts;
    this->m_streamParams[streamIndex].addAudioSamples(&iFrame, packet.id());

    int outSamples = codecContext->codec->capabilities
                     & CODEC_CAP_VARIABLE_FRAME_SIZE?
                        iFrame.nb_samples:
                        codecContext->frame_size;

    this->deleteFrame(&iFrame);

    forever {
        pts = this->m_streamParams[streamIndex].audioPts();
        uint8_t *buffer = NULL;
        int bufferSize =
                this->m_streamParams[streamIndex].readAudioSamples(outSamples,
                                                                   &buffer);

        if (bufferSize < 1)
            break;

        AVFrame oFrame;
        memset(&oFrame, 0, sizeof(AVFrame));
        oFrame.format = codecContext->sample_fmt;
        oFrame.channel_layout = codecContext->channel_layout;
        oFrame.sample_rate = codecContext->sample_rate;
        oFrame.nb_samples = outSamples;
        oFrame.pts = pts;

        if (avcodec_fill_audio_frame(&oFrame,
                                     codecContext->channels,
                                     codecContext->sample_fmt,
                                     buffer,
                                     bufferSize,
                                     1) < 0) {
            delete [] buffer;

            continue;
        }

        // Compress audio packet.
#ifdef HAVE_SENDRECV
        int result = avcodec_send_frame(codecContext.data(), &oFrame);

        if (result < 0) {
            char error[1024];
            av_strerror(result, error, 1024);
            qDebug() << "Error: " << error;
            delete [] buffer;

            break;
        }

        forever {
            // Initialize audio packet.
            AVPacket pkt;
            memset(&pkt, 0, sizeof(AVPacket));
            av_init_packet(&pkt);

            if (avcodec_receive_packet(codecContext.data(), &pkt) < 0)
                break;

            pkt.stream_index = streamIndex;
            this->rescaleTS(&pkt, codecContext->time_base, stream->time_base);

            // Write audio packet.
            this->m_writeMutex.lock();
            av_interleaved_write_frame(this->m_formatContext, &pkt);
            this->m_writeMutex.unlock();
        }
#else
        // Initialize audio packet.
        AVPacket pkt;
        memset(&pkt, 0, sizeof(AVPacket));
        av_init_packet(&pkt);

        int gotPacket;
        int result = avcodec_encode_audio2(codecContext.data(),
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
        this->rescaleTS(&pkt, codecContext->time_base, stream->time_base);

        // Write audio packet.
        this->m_writeMutex.lock();
        av_interleaved_write_frame(this->m_formatContext, &pkt);
        this->m_writeMutex.unlock();
#endif

        delete [] buffer;
    }
}

void MediaWriterFFmpeg::writeVideoPacket(const AkVideoPacket &packet)
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
    auto codecContext = this->m_streamParams[streamIndex].codecContext();

    AVFrame oFrame;
    memset(&oFrame, 0, sizeof(AVFrame));
    oFrame.format = codecContext->pix_fmt;
    oFrame.width = codecContext->width;
    oFrame.height = codecContext->height;

    AkPacket videoPacket = packet.toPacket();
    QImage image = AkUtils::packetToImage(videoPacket);
    image = image.convertToFormat(QImage::Format_ARGB32);
    image = this->swapChannels(image);
    videoPacket = AkUtils::imageToPacket(image, videoPacket);

    if (!this->m_streamParams[streamIndex].convert(videoPacket, &oFrame)) {
        this->deleteFrame(&oFrame);

        return;
    }

    AkFrac outTimeBase(codecContext->time_base.num,
                       codecContext->time_base.den);

    qint64 pts = qRound64(packet.pts()
                          * packet.timeBase().value()
                          / outTimeBase.value());

    oFrame.pts =
            this->m_streamParams[streamIndex].nextPts(pts, packet.id());

    if (oFrame.pts < 0) {
        this->deleteFrame(&oFrame);

        return;
    }

    if (this->m_formatContext->oformat->flags & AVFMT_RAWPICTURE) {
        // Raw video case - directly store the picture in the packet
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.data = oFrame.data[0];
        pkt.size = sizeof(AVPicture);
        pkt.pts = oFrame.pts;
        pkt.stream_index = streamIndex;

        this->rescaleTS(&pkt, codecContext->time_base, stream->time_base);

        this->m_writeMutex.lock();
        av_interleaved_write_frame(this->m_formatContext, &pkt);
        this->m_writeMutex.unlock();
    } else {
        // encode the image
#ifdef HAVE_SENDRECV
        auto error = avcodec_send_frame(codecContext.data(), &oFrame);

        if (error < 0) {
            char errorStr[1024];
            av_strerror(AVERROR(error), errorStr, 1024);
            qDebug() << "Error encoding packets: " << errorStr;
            this->deleteFrame(&oFrame);

            return;
        }

        forever {
            AVPacket pkt;
            av_init_packet(&pkt);
            pkt.data = NULL; // packet data will be allocated by the encoder
            pkt.size = 0;

            if (avcodec_receive_packet(codecContext.data(), &pkt) < 0)
                break;

            pkt.stream_index = streamIndex;
            this->rescaleTS(&pkt,
                            codecContext->time_base,
                            stream->time_base);

            // Write the compressed frame to the media file.
            this->m_writeMutex.lock();
            av_interleaved_write_frame(this->m_formatContext, &pkt);
            this->m_writeMutex.unlock();
        }
#else
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = NULL; // packet data will be allocated by the encoder
        pkt.size = 0;

        int gotPacket;

        if (avcodec_encode_video2(codecContext.data(),
                                  &pkt,
                                  &oFrame,
                                  &gotPacket) < 0) {
            this->deleteFrame(&oFrame);

            return;
        }

        // If size is zero, it means the image was buffered.
        if (gotPacket) {
            pkt.stream_index = streamIndex;
            this->rescaleTS(&pkt,
                            codecContext->time_base,
                            stream->time_base);

            // Write the compressed frame to the media file.
            this->m_writeMutex.lock();
            av_interleaved_write_frame(this->m_formatContext, &pkt);
            this->m_writeMutex.unlock();
        }
#endif
    }

    this->deleteFrame(&oFrame);
}

void MediaWriterFFmpeg::writeSubtitlePacket(const AkPacket &packet)
{
    Q_UNUSED(packet)
    // TODO: Implement this.
}
