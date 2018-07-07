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
#include <QSharedPointer>
#include <QSize>
#include <QVector>
#include <QLibrary>
#include <QThreadPool>
#include <QMutex>
#include <QtMath>
#include <akutils.h>
#include <akaudiocaps.h>
#include <akfrac.h>
#include <akcaps.h>
#include <akpacket.h>

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
            AVOutputFormat *outputFormat = nullptr;

            while ((outputFormat = av_oformat_next(outputFormat))) {
                AVCodec *codec = nullptr;

                while ((codec = av_codec_next(codec))) {
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

            for (auto codec = av_codec_next(nullptr);
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
                avcodec_free_context(&codecContext);
            }

            return codecDefaults;
        }
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
        AVFormatContext *m_formatContext;
        QThreadPool m_threadPool;
        qint64 m_maxPacketQueueSize;
        bool m_isRecording;
        QMutex m_packetMutex;
        QMutex m_audioMutex;
        QMutex m_videoMutex;
        QMutex m_subtitleMutex;
        QMutex m_writeMutex;
        QMap<int, AbstractStreamPtr> m_streamsMap;

        MediaWriterFFmpegPrivate(MediaWriterFFmpeg *self):
            self(self),
            m_formatContext(nullptr),
            m_maxPacketQueueSize(15 * 1024 * 1024),
            m_isRecording(false)
        {
        }

        QString guessFormat();
        QVariantList parseOptions(const AVClass *avClass) const;
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
    delete this->d;
}

QString MediaWriterFFmpeg::outputFormat() const
{
    return this->d->m_outputFormat;
}

QVariantList MediaWriterFFmpeg::streams() const
{
    QVariantList streams;

    for (const QVariantMap &stream: this->d->m_streamConfigs)
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

    for (auto &format: mediaWriterFFmpegGlobal->m_supportedCodecs.keys())
        if (!this->m_formatsBlackList.contains(format))
            formats << format;

    return formats;
}

QStringList MediaWriterFFmpeg::fileExtensions(const QString &format)
{
    AVOutputFormat *outputFormat = av_guess_format(format.toStdString().c_str(),
                                                   nullptr,
                                                   nullptr);

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
                                                   nullptr,
                                                   nullptr);

    if (!outputFormat)
        return QString();

    return QString(outputFormat->long_name);
}

QVariantList MediaWriterFFmpeg::formatOptions()
{
    auto outFormat = this->d->guessFormat();

    if (outFormat.isEmpty())
        return QVariantList();

    AVOutputFormat *outputFormat = av_guess_format(outFormat.toStdString().c_str(),
                                                   nullptr,
                                                   nullptr);

    if (!outputFormat)
        return QVariantList();

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
                                        nullptr,
                                        nullptr);

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

    if (codecId == AV_CODEC_ID_VP9)
        codecId = AV_CODEC_ID_VP8;

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
    QString outputFormat = this->d->guessFormat();

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
    QString outputFormat = this->d->guessFormat();

    if (outputFormat.isEmpty())
        return QVariantMap();

    bool streamChanged = false;

    if (codecParams.contains("label")
        && this->d->m_streamConfigs[index]["label"] != codecParams.value("label")) {
        this->d->m_streamConfigs[index]["label"] = codecParams.value("label");
        streamChanged |= true;
    }

    AkCaps streamCaps = this->d->m_streamConfigs[index]["caps"].value<AkCaps>();

    if (codecParams.contains("caps")
        && this->d->m_streamConfigs[index]["caps"] != codecParams.value("caps")) {
        this->d->m_streamConfigs[index]["caps"] = codecParams.value("caps");
        streamChanged |= true;
    }

    QString codec;

    if (codecParams.contains("codec")) {
        if (this->supportedCodecs(outputFormat, streamCaps.mimeType())
            .contains(codecParams["codec"].toString())) {
            codec = codecParams["codec"].toString();
        } else
            codec = this->defaultCodec(outputFormat, streamCaps.mimeType());

        this->d->m_streamConfigs[index]["codec"] = codec;
        streamChanged |= true;
    } else
        codec = this->d->m_streamConfigs[index]["codec"].toString();

    QVariantMap codecDefaults = this->defaultCodecParams(codec);

    if ((streamCaps.mimeType() == "audio/x-raw"
         || streamCaps.mimeType() == "video/x-raw")
        && codecParams.contains("bitrate")) {
        int bitRate = codecParams["bitrate"].toInt();
        this->d->m_streamConfigs[index]["bitrate"] =
                bitRate > 0? bitRate: codecDefaults["defaultBitRate"].toInt();
        streamChanged |= true;
    }

    if (streamCaps.mimeType() == "video/x-raw"
        && codecParams.contains("gop")) {
        int gop = codecParams["gop"].toInt();
        this->d->m_streamConfigs[index]["gop"] =
                gop > 0? gop: codecDefaults["defaultGOP"].toInt();
        streamChanged |= true;
    }

    if (streamChanged)
        emit this->streamsChanged(this->streams());

    return this->d->m_streamConfigs[index];
}

QVariantList MediaWriterFFmpeg::codecOptions(int index)
{
    auto outputFormat = this->d->guessFormat();

    if (outputFormat.isEmpty())
        return QVariantList();

    auto codec = this->d->m_streamConfigs.value(index).value("codec").toString();

    if (codec.isEmpty())
        return QVariantList();

    auto avCodec = avcodec_find_encoder_by_name(codec.toStdString().c_str());

    if (!avCodec)
        return QVariantList();

    auto optKey = QString("%1/%2/%3").arg(outputFormat).arg(index).arg(codec);
    auto options = this->d->parseOptions(avCodec->priv_class);
    auto globalCodecOptions = this->d->m_codecOptions.value(optKey);
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

AVDictionary *MediaWriterFFmpegPrivate::formatContextOptions(AVFormatContext *formatContext,
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

    AVDictionary *formatOptions = nullptr;

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

    for (auto &key: formatOptions.keys())
        if (formatOptions[key] != this->d->m_formatOptions.value(outputFormat).value(key)) {
            this->d->m_formatOptions[outputFormat][key] = formatOptions[key];
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

    for (auto &key: codecOptions.keys())
        if (codecOptions[key] != this->d->m_codecOptions.value(optKey).value(key)) {
            this->d->m_codecOptions[optKey][key] = codecOptions[key];
            modified = true;
        }

    if (modified)
        emit this->codecOptionsChanged(optKey, this->d->m_formatOptions.value(optKey));
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

    if (avformat_alloc_output_context2(&this->d->m_formatContext,
                                       nullptr,
                                       this->d->m_outputFormat.isEmpty()?
                                            nullptr: this->d->m_outputFormat.toStdString().c_str(),
                                       this->m_location.toStdString().c_str()) < 0)
        return false;

    auto streamConfigs = this->d->m_streamConfigs.toVector();

    if (!strcmp(this->d->m_formatContext->oformat->name, "mxf_opatom")) {
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
        auto stream = avformat_new_stream(this->d->m_formatContext, nullptr);
        stream->id = i;

        // Confihure streams parameters.
        AkCaps streamCaps = configs["caps"].value<AkCaps>();
        AbstractStreamPtr mediaStream;
        int inputId = configs["index"].toInt();

        if (streamCaps.mimeType() == "audio/x-raw") {
            mediaStream =
                    AbstractStreamPtr(new AudioStream(this->d->m_formatContext,
                                                      uint(i), inputId,
                                                      configs,
                                                      this->d->m_codecOptions,
                                                      this));
        } else if (streamCaps.mimeType() == "video/x-raw") {
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
                             SIGNAL(packetReady(AVPacket *)),
                             this,
                             SLOT(writePacket(AVPacket *)),
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
    int error = avformat_write_header(this->d->m_formatContext, &formatOptions);
    av_dict_free(&formatOptions);

    if (error < 0) {
        char errorStr[1024];
        av_strerror(AVERROR(error), errorStr, 1024);
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
    this->d->m_writeMutex.lock();
    av_interleaved_write_frame(this->d->m_formatContext, packet);
    this->d->m_writeMutex.unlock();
}

#include "moc_mediawriterffmpeg.cpp"
