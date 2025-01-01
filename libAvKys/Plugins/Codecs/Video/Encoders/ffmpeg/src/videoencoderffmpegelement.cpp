/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#include <QLibrary>
#include <QMutex>
#include <QQmlContext>
#include <QThread>
#include <QVariant>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akcompressedvideocaps.h>
#include <akpluginmanager.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>
#include <akcompressedvideopacket.h>
#include <iak/akelement.h>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/opt.h>
    #include <libavutil/parseutils.h>
}

#include "videoencoderffmpegelement.h"

#define CODEC_COMPLIANCE FF_COMPLIANCE_VERY_STRICT

#define VideoCodecID_ffvp8  AkCompressedVideoCaps::VideoCodecID(AK_MAKE_FOURCC(0xFF, 'V', 'P', '8'))
#define VideoCodecID_ffvp9  AkCompressedVideoCaps::VideoCodecID(AK_MAKE_FOURCC(0xFF, 'V', 'P', '9'))
#define VideoCodecID_ffav1  AkCompressedVideoCaps::VideoCodecID(AK_MAKE_FOURCC(0xFF, 'A', 'V', '1'))
#define VideoCodecID_ffh264 AkCompressedVideoCaps::VideoCodecID(AK_MAKE_FOURCC(0xFF, 'A', 'V', 'C'))
#define VideoCodecID_ffhevc AkCompressedVideoCaps::VideoCodecID(AK_MAKE_FOURCC(0xFF, 'H', 'E', 'V'))

struct FFmpegCodecs
{
    AkVideoEncoderCodecID codecID;
    AVCodecID ffCodecID;

    static inline const FFmpegCodecs *table()
    {
        static const FFmpegCodecs ffmpegVideoEncCodecsTable[] = {
            {VideoCodecID_ffvp8                         , AV_CODEC_ID_VP8 },
            {VideoCodecID_ffvp9                         , AV_CODEC_ID_VP9 },
            {VideoCodecID_ffav1                         , AV_CODEC_ID_AV1 },
            {VideoCodecID_ffh264                        , AV_CODEC_ID_H264},
            {VideoCodecID_ffhevc                        , AV_CODEC_ID_HEVC},
            {AkCompressedVideoCaps::VideoCodecID_unknown, AV_CODEC_ID_NONE},
        };

        return ffmpegVideoEncCodecsTable;
    }

    static inline QList<AVCodecID> ffCodecs()
    {
        QList<AVCodecID> ffCodecs;

        for (auto codec = table();
             codec->codecID != AkCompressedVideoCaps::VideoCodecID_unknown;
             codec++) {
            ffCodecs << codec->ffCodecID;
        }

        return ffCodecs;
    }

    static inline const FFmpegCodecs *byCodecID(AkVideoEncoderCodecID codecID)
    {
        auto codec = table();

        for (;
             codec->codecID != AkCompressedVideoCaps::VideoCodecID_unknown;
             codec++) {
            if (codec->codecID == codecID)
                return codec;
        }

        return codec;
    }

    static inline const FFmpegCodecs *byFFCodecID(AVCodecID ffCodecID)
    {
        auto codec = table();

        for (;
             codec->codecID != AkCompressedVideoCaps::VideoCodecID_unknown;
             codec++) {
            if (codec->ffCodecID == ffCodecID)
                return codec;
        }

        return codec;
    }
};

struct MenuOption
{
    QString name;
    QString help;
    QVariant value;
};

struct CodecOption
{
    QString name;
    QString help;
    AVOptionType avType;
    VideoEncoderFFmpegElement::OptionType type;
    qreal min;
    qreal max;
    qreal step;
    QVariant defaultValue;
    QVariant value;
    QString unit;
    QVector<MenuOption> menu;
};

struct CodecInfo
{
    QString name;
    QString description;
    AkVideoEncoderCodecID codecID;
    AkVideoCaps::PixelFormatList formats;
    QVector<CodecOption> options;
};

struct PixelFormatsTable
{
    AkVideoCaps::PixelFormat format;
    AVPixelFormat ffFormat;

    inline static const PixelFormatsTable *table()
    {
        static const PixelFormatsTable ffmpegEncoderPixelFormatsTable[] {
            {AkVideoCaps::Format_ayuv64     , AV_PIX_FMT_AYUV64   },
            {AkVideoCaps::Format_bgr24      , AV_PIX_FMT_BGR24    },
            {AkVideoCaps::Format_bgra       , AV_PIX_FMT_BGRA     },
            {AkVideoCaps::Format_bgrx       , AV_PIX_FMT_BGR0     },
            {AkVideoCaps::Format_gbrp       , AV_PIX_FMT_GBRP     },
            {AkVideoCaps::Format_gbrp10     , AV_PIX_FMT_GBRP10   },
            {AkVideoCaps::Format_gbrp12     , AV_PIX_FMT_GBRP12   },
            {AkVideoCaps::Format_gbrp16     , AV_PIX_FMT_GBRP16   },
            {AkVideoCaps::Format_nv12       , AV_PIX_FMT_NV12     },
            {AkVideoCaps::Format_nv16       , AV_PIX_FMT_NV16     },
            {AkVideoCaps::Format_nv20       , AV_PIX_FMT_NV20     },
            {AkVideoCaps::Format_nv21       , AV_PIX_FMT_NV21     },
            {AkVideoCaps::Format_nv24       , AV_PIX_FMT_NV24     },
            {AkVideoCaps::Format_p010       , AV_PIX_FMT_P010     },
            {AkVideoCaps::Format_p016       , AV_PIX_FMT_P016     },
            {AkVideoCaps::Format_p210       , AV_PIX_FMT_P210     },
            {AkVideoCaps::Format_p216       , AV_PIX_FMT_P216     },
            {AkVideoCaps::Format_p416       , AV_PIX_FMT_P416     },
            {AkVideoCaps::Format_rgb24      , AV_PIX_FMT_RGB24    },
            {AkVideoCaps::Format_rgba       , AV_PIX_FMT_RGBA     },
            {AkVideoCaps::Format_rgbx       , AV_PIX_FMT_RGB0     },
            {AkVideoCaps::Format_uyvy422    , AV_PIX_FMT_UYVY422  },
            {AkVideoCaps::Format_xbgr       , AV_PIX_FMT_0BGR32   },
            {AkVideoCaps::Format_xbgr2101010, AV_PIX_FMT_X2BGR10  },
            {AkVideoCaps::Format_xrgb       , AV_PIX_FMT_0RGB32   },
            {AkVideoCaps::Format_xrgb2101010, AV_PIX_FMT_X2RGB10  },
            {AkVideoCaps::Format_y10        , AV_PIX_FMT_GRAY10   },
            {AkVideoCaps::Format_y8         , AV_PIX_FMT_GRAY8    },
            {AkVideoCaps::Format_yuv420p    , AV_PIX_FMT_YUV420P  },
            {AkVideoCaps::Format_yuv420p10  , AV_PIX_FMT_YUV420P10},
            {AkVideoCaps::Format_yuv420p12  , AV_PIX_FMT_YUV420P12},
            {AkVideoCaps::Format_yuv422p    , AV_PIX_FMT_YUV422P  },
            {AkVideoCaps::Format_yuv422p10  , AV_PIX_FMT_YUV422P10},
            {AkVideoCaps::Format_yuv422p12  , AV_PIX_FMT_YUV422P12},
            {AkVideoCaps::Format_yuv444p    , AV_PIX_FMT_YUV444P  },
            {AkVideoCaps::Format_yuv444p10  , AV_PIX_FMT_YUV444P10},
            {AkVideoCaps::Format_yuv444p12  , AV_PIX_FMT_YUV444P12},
            {AkVideoCaps::Format_yuva420p   , AV_PIX_FMT_YUVA420P },
            {AkVideoCaps::Format_none       , AV_PIX_FMT_NONE     },
        };

        return ffmpegEncoderPixelFormatsTable;
    }

    inline static const PixelFormatsTable *byFormat(AkVideoCaps::PixelFormat format)
    {
        auto item = table();

        for (; item->format; ++item)
            if (item->format == format)
                return item;

        return item;
    }

    inline static const PixelFormatsTable *byFFFormat(AVPixelFormat format)
    {
        auto item = table();

        for (; item->format; ++item)
            if (item->ffFormat == format)
                return item;

        return item;
    }

    inline static QVector<AVPixelFormat> supportedFFPixelFormats()
    {
        QVector<AVPixelFormat> formats;
        auto item = table();

        for (; item->format; ++item)
            formats << item->ffFormat;

        return formats;
    }
};

using AVCodecParametersPtr = QSharedPointer<AVCodecParameters>;

class VideoEncoderFFmpegElementPrivate
{
    public:
        VideoEncoderFFmpegElement *self;
        AkVideoConverter m_videoConverter;
        AkCompressedVideoCaps m_outputCaps;
        bool m_globalHeaders {true};
        AkCompressedVideoPackets m_headers;
        AVCodecParametersPtr m_codecParameters;
        QVector<CodecInfo> m_codecs;
        AVCodecContext *m_context {nullptr};
        QMutex m_mutex;
        qint64 m_id {0};
        int m_index {0};
        bool m_initialized {false};
        bool m_paused {false};
        qint64 m_encodedTimePts {0};
        AkElementPtr m_fpsControl {akPluginManager->create<AkElement>("VideoFilter/FpsControl")};

        explicit VideoEncoderFFmpegElementPrivate(VideoEncoderFFmpegElement *self);
        ~VideoEncoderFFmpegElementPrivate();
        bool isAvailable(const QString &codec) const;
        void listCodecs();
        void adjustDefaults();
        VideoEncoderFFmpegElement::OptionType optionType(AVOptionType avType) const;
        const CodecOption *codecOption(const QString &option) const;
        AVDictionary *readCodecOptions() const;
        bool init();
        void uninit();
        void updateHeaders();
        void updateOutputCaps();
        void encodeFrame(const AkVideoPacket &src);
        void sendFrame(const AVPacket *avPacket) const;
};

VideoEncoderFFmpegElement::VideoEncoderFFmpegElement():
    AkVideoEncoder()
{
    this->d = new VideoEncoderFFmpegElementPrivate(this);
    this->d->listCodecs();
    this->d->adjustDefaults();
    auto encoder = avcodec_find_encoder(FFmpegCodecs::ffCodecs().first());

    if (encoder)
        this->setCodec(encoder->name);

    QObject::connect(this,
                     &AkVideoEncoder::codecChanged,
                     [this] () {
        this->d->updateOutputCaps();
        emit this->optionsChanged(this->options());
    });
}

VideoEncoderFFmpegElement::~VideoEncoderFFmpegElement()
{
    this->d->uninit();
    delete this->d;
}

QStringList VideoEncoderFFmpegElement::codecs() const
{
    QStringList codecs;

    for (auto &codec: this->d->m_codecs)
        codecs << codec.name;

    return codecs;
}

AkVideoEncoderCodecID VideoEncoderFFmpegElement::codecID(const QString &codec) const
{
    auto it = std::find_if(this->d->m_codecs.constBegin(),
                           this->d->m_codecs.constEnd(),
                           [&codec] (const CodecInfo &codecInfo) -> bool {
        return codecInfo.name == codec;
    });

    if (it == this->d->m_codecs.constEnd())
        return AkCompressedVideoCaps::VideoCodecID_unknown;

    return it->codecID;
}

QString VideoEncoderFFmpegElement::codecDescription(const QString &codec) const
{
    auto it = std::find_if(this->d->m_codecs.constBegin(),
                           this->d->m_codecs.constEnd(),
                           [&codec] (const CodecInfo &codecInfo) -> bool {
        return codecInfo.name == codec;
    });

    if (it == this->d->m_codecs.constEnd())
        return {};

    return it->description;
}

AkCompressedVideoCaps VideoEncoderFFmpegElement::outputCaps() const
{
    return this->d->m_outputCaps;
}

AkCompressedPackets VideoEncoderFFmpegElement::headers() const
{
    AkCompressedPackets packets;

    for (auto &header: this->d->m_headers)
        packets << header;

    return packets;
}

qint64 VideoEncoderFFmpegElement::encodedTimePts() const
{
    return this->d->m_encodedTimePts;
}

QStringList VideoEncoderFFmpegElement::options() const
{
    auto it = std::find_if(this->d->m_codecs.constBegin(),
                           this->d->m_codecs.constEnd(),
                           [this] (const CodecInfo &codec) -> bool {
        return codec.name == this->codec();
    });

    if (it == this->d->m_codecs.constEnd())
        return {};

    QStringList options;

    for (auto &option: it->options)
        options << option.name;

    return options;
}

bool VideoEncoderFFmpegElement::globalHeaders() const
{
    return this->d->m_globalHeaders;
}

QString VideoEncoderFFmpegElement::optionHelp(const QString &option) const
{
    auto codecOption = this->d->codecOption(option);

    if (!codecOption)
        return {};

    return codecOption->help;
}

VideoEncoderFFmpegElement::OptionType VideoEncoderFFmpegElement::optionType(const QString &option) const
{
    auto codecOption = this->d->codecOption(option);

    if (!codecOption)
        return OptionType_Unknown;

    return codecOption->type;
}

qreal VideoEncoderFFmpegElement::optionMin(const QString &option) const
{
    auto codecOption = this->d->codecOption(option);

    if (!codecOption)
        return 0.0;

    return codecOption->min;
}

qreal VideoEncoderFFmpegElement::optionMax(const QString &option) const
{
    auto codecOption = this->d->codecOption(option);

    if (!codecOption)
        return 0.0;

    return codecOption->max;
}

qreal VideoEncoderFFmpegElement::optionStep(const QString &option) const
{
    auto codecOption = this->d->codecOption(option);

    if (!codecOption)
        return 0.0;

    return codecOption->step;
}

QVariant VideoEncoderFFmpegElement::optionDefaultValue(const QString &option) const
{
    auto codecOption = this->d->codecOption(option);

    if (!codecOption)
        return {};

    return codecOption->defaultValue;
}

QVariant VideoEncoderFFmpegElement::optionValue(const QString &option) const
{
    auto codecOption = this->d->codecOption(option);

    if (!codecOption)
        return {};

    return codecOption->value;
}

QStringList VideoEncoderFFmpegElement::optionMenu(const QString &option) const
{
    auto codecOption = this->d->codecOption(option);

    if (!codecOption)
        return {};

    QStringList menu;

    for (auto &menuOption: codecOption->menu)
        menu << menuOption.name;

    return menu;
}

QString VideoEncoderFFmpegElement::menuOptionHelp(const QString &option,
                                                  const QString &menuOption) const
{
    auto codecOption = this->d->codecOption(option);

    if (!codecOption)
        return {};

    auto it = std::find_if(codecOption->menu.constBegin(),
                           codecOption->menu.constEnd(),
                           [&menuOption] (const MenuOption &option) -> bool {
        return option.name == menuOption;
    });

    if (it == codecOption->menu.constEnd())
        return {};

    return it->help;
}

QVariant VideoEncoderFFmpegElement::menuOptionValue(const QString &option,
                                                    const QString &menuOption) const
{
    auto codecOption = this->d->codecOption(option);

    if (!codecOption)
        return {};

    auto it = std::find_if(codecOption->menu.constBegin(),
                           codecOption->menu.constEnd(),
                           [&menuOption] (const MenuOption &option) -> bool {
        return option.name == menuOption;
    });

    if (it == codecOption->menu.constEnd())
        return {};

    return it->value;
}

QString VideoEncoderFFmpegElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/VideoEncoderFFmpeg/share/qml/main.qml");
}

void VideoEncoderFFmpegElement::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("VideoEncoderFFmpeg", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket VideoEncoderFFmpegElement::iVideoStream(const AkVideoPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (this->d->m_paused || !this->d->m_initialized || !this->d->m_fpsControl)
        return {};

    bool discard = false;
    QMetaObject::invokeMethod(this->d->m_fpsControl.data(),
                              "discard",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(bool, discard),
                              Q_ARG(AkVideoPacket, packet));

    if (discard)
        return {};

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    this->d->m_fpsControl->iStream(src);

    return {};
}

void VideoEncoderFFmpegElement::setGlobalHeaders(bool globalHeaders)
{
    if (globalHeaders == this->d->m_globalHeaders)
        return;

    this->d->m_globalHeaders = globalHeaders;
    emit this->globalHeadersChanged(globalHeaders);
}

void VideoEncoderFFmpegElement::setOptionValue(const QString &option,
                                               const QVariant &value)
{
    auto codec = std::find_if(this->d->m_codecs.begin(),
                              this->d->m_codecs.end(),
                              [this] (const CodecInfo &codec) -> bool {
        return codec.name == this->codec();
    });

    if (codec == this->d->m_codecs.constEnd())
        return;

    auto codecOption =
            std::find_if(codec->options.begin(),
                         codec->options.end(),
                         [&option] (const CodecOption &codecOption) -> bool {
        return codecOption.name == option;
    });

    if (codecOption == codec->options.constEnd())
        return;

    if (value == codecOption->value)
        return;

    codecOption->value = value;
    emit this->optionValueChanged(option, value);
}

void VideoEncoderFFmpegElement::resetGlobalHeaders()
{
    this->setGlobalHeaders(true);
}

void VideoEncoderFFmpegElement::resetOptionValue(const QString &option)
{
    auto codec = std::find_if(this->d->m_codecs.begin(),
                              this->d->m_codecs.end(),
                              [this] (const CodecInfo &codec) -> bool {
        return codec.name == this->codec();
    });

    if (codec == this->d->m_codecs.constEnd())
        return;

    auto codecOption =
            std::find_if(codec->options.begin(),
                         codec->options.end(),
                         [&option] (const CodecOption &codecOption) -> bool {
        return codecOption.name == option;
    });

    if (codecOption == codec->options.constEnd())
        return;

    if (codecOption->defaultValue == codecOption->value)
        return;

    codecOption->value = codecOption->defaultValue;
    emit this->optionValueChanged(option, codecOption->defaultValue);
}

void VideoEncoderFFmpegElement::resetCodecOptions()
{
    auto codec = std::find_if(this->d->m_codecs.begin(),
                              this->d->m_codecs.end(),
                              [this] (const CodecInfo &codec) -> bool {
        return codec.name == this->codec();
    });

    if (codec == this->d->m_codecs.constEnd())
        return;

    for (auto &codecOption: codec->options) {
        if (codecOption.defaultValue == codecOption.value)
            return;

        codecOption.value = codecOption.defaultValue;
        emit this->optionValueChanged(codecOption.name, codecOption.defaultValue);
    }
}

void VideoEncoderFFmpegElement::resetOptions()
{
    AkVideoEncoder::resetOptions();
    this->resetCodecOptions();
}

bool VideoEncoderFFmpegElement::setState(ElementState state)
{
    auto curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
            this->d->m_paused = state == AkElement::ElementStatePaused;
        case AkElement::ElementStatePlaying:
            if (!this->d->init()) {
                this->d->m_paused = false;

                return false;
            }

            return AkElement::setState(state);
        default:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->uninit();

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            this->d->m_paused = false;

            return AkElement::setState(state);
        default:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->uninit();

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            this->d->m_paused = true;

            return AkElement::setState(state);
        default:
            break;
        }

        break;
    }
    }

    return false;
}

VideoEncoderFFmpegElementPrivate::VideoEncoderFFmpegElementPrivate(VideoEncoderFFmpegElement *self):
    self(self)
{
    this->m_videoConverter.setAspectRatioMode(AkVideoConverter::AspectRatioMode_Fit);

    QObject::connect(self,
                     &AkVideoEncoder::inputCapsChanged,
                     [this] (const AkVideoCaps &inputCaps) {
                        Q_UNUSED(inputCaps)

                        this->updateOutputCaps();
                     });

    if (this->m_fpsControl)
        QObject::connect(this->m_fpsControl.data(),
                         &AkElement::oStream,
                         [this] (const AkPacket &packet) {
                             this->encodeFrame(packet);
                         });
}

VideoEncoderFFmpegElementPrivate::~VideoEncoderFFmpegElementPrivate()
{

}

bool VideoEncoderFFmpegElementPrivate::isAvailable(const QString &codec) const
{
    static struct
    {
        const char *codec;
        bool isAvailable;
    } ffmpegVideoEncAvailableCodecs[32];
    static size_t ffmpegVideoEncAvailableCodecsSize = 0;

    for (size_t i = 0; i < ffmpegVideoEncAvailableCodecsSize; ++i)
        if (ffmpegVideoEncAvailableCodecs[i].codec == codec)
            return ffmpegVideoEncAvailableCodecs[i].isAvailable;

    auto encoder =
            avcodec_find_encoder_by_name(codec.toStdString().c_str());

    if (!encoder)
        return false;

    bool isAvailable = false;
    auto context = avcodec_alloc_context3(encoder);

    if (context) {
        const AVPixelFormat *avFormats = nullptr;
        int nFormats = 0;
        avcodec_get_supported_config(nullptr,
                                     encoder,
                                     AV_CODEC_CONFIG_PIX_FORMAT,
                                     0,
                                     reinterpret_cast<const void **>(&avFormats),
                                     &nFormats);

        context->pix_fmt = nFormats > 0? *avFormats: AV_PIX_FMT_YUV420P;
        context->width = 640;
        context->height = 480;
        context->framerate = {30, 1};
        context->time_base = {context->framerate.den, context->framerate.num};
        context->bit_rate = 1500000;

        isAvailable = avcodec_open2(context, encoder, nullptr) >= 0;
        avcodec_free_context(&context);
    }

    auto i = ffmpegVideoEncAvailableCodecsSize++;
    ffmpegVideoEncAvailableCodecs[i].codec = encoder->name;
    ffmpegVideoEncAvailableCodecs[i].isAvailable = isAvailable;

    return isAvailable;
}

void VideoEncoderFFmpegElementPrivate::listCodecs()
{
    auto supportedFormats = PixelFormatsTable::supportedFFPixelFormats();
    void *opaqueCdc = nullptr;
    auto supportedCodecs = FFmpegCodecs::ffCodecs();

    while (auto codec = av_codec_iterate(&opaqueCdc)) {
        if (codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL
            && CODEC_COMPLIANCE > FF_COMPLIANCE_EXPERIMENTAL)
            continue;

        if (!supportedCodecs.contains(codec->id) || !av_codec_is_encoder(codec))
            continue;

        if (!this->isAvailable(codec->name))
            continue;

        const AVPixelFormat *avFormats = nullptr;
        int nFormats = 0;
        avcodec_get_supported_config(nullptr,
                                     codec,
                                     AV_CODEC_CONFIG_PIX_FORMAT,
                                     0,
                                     reinterpret_cast<const void **>(&avFormats),
                                     &nFormats);

        QVector<AkVideoCaps::PixelFormat> formats;

        for (int i = 0; i < nFormats; i++)
            if (supportedFormats.contains(avFormats[i]))
                formats << PixelFormatsTable::byFFFormat(avFormats[i])->format;

        if (formats.isEmpty())
            continue;

        QVector<CodecOption> options;
        QMap<QString, QVector<MenuOption>> menu;

        if (codec->priv_class)
            for (auto option = codec->priv_class->option;
                 option;
                 option = av_opt_next(&codec->priv_class, option)) {
                if (option->flags & AV_OPT_FLAG_DEPRECATED)
                    continue;

                auto optionType = this->optionType(option->type);

                if (optionType == VideoEncoderFFmpegElement::OptionType_Unknown)
                    continue;

                QVariant value;
                qreal step = 0.0;

                switch (option->type) {
                    case AV_OPT_TYPE_FLAGS:
                    case AV_OPT_TYPE_INT:
                    case AV_OPT_TYPE_INT64:
                    case AV_OPT_TYPE_CONST:
                    case AV_OPT_TYPE_DURATION:
                    case AV_OPT_TYPE_BOOL:
                    case AV_OPT_TYPE_UINT:
                        value = qint64(option->default_val.i64);
                        step = 1.0;
                        break;
                    case AV_OPT_TYPE_DOUBLE:
                    case AV_OPT_TYPE_FLOAT:
                        value = option->default_val.dbl;
                        step = 0.01;
                        break;
                    case AV_OPT_TYPE_STRING:
                        value = option->default_val.str?
                                    QString(option->default_val.str):
                                    QString();
                        break;
                    case AV_OPT_TYPE_IMAGE_SIZE: {
                        int width = 0;
                        int height = 0;

                        if (!option->default_val.str
                            || av_parse_video_size(&width, &height, option->default_val.str) < 0)
                            value = "";
                        else
                            value = QString("%1x%2")
                                        .arg(width)
                                        .arg(height);

                        break;
                    }
                    case AV_OPT_TYPE_VIDEO_RATE: {
                        AVRational rate;

                        if (!option->default_val.str
                            || av_parse_video_rate(&rate, option->default_val.str) < 0)
                            value = "";
                        else
                            value = QString("%1/%2")
                                        .arg(rate.num)
                                        .arg(rate.den);

                        break;
                    }
                    case AV_OPT_TYPE_COLOR: {
                        uint8_t color[4];

                        if (!option->default_val.str
                            || av_parse_color(color,
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
                        value = QString("%1/%2")
                                    .arg(option->default_val.q.num)
                                    .arg(option->default_val.q.den);
                        break;
                    default:
                        continue;
                }

                if (option->type == AV_OPT_TYPE_CONST) {
                    MenuOption menuOption {option->name,
                                           option->help,
                                           value};

                    if (menu.contains(option->name))
                        menu[option->name] << menuOption;
                    else
                        menu[option->name] = {menuOption};
                } else {
                    options << CodecOption {option->name,
                                            option->help,
                                            option->type,
                                            optionType,
                                            option->min,
                                            option->max,
                                            step,
                                            value,
                                            value,
                                            option->unit,
                                            {}};
                }
            }

        for (auto &option: options)
            if (menu.contains(option.unit))
                option.menu = menu[option.unit];

        this->m_codecs << CodecInfo {QString(codec->name),
                                     QString(codec->long_name),
                                     FFmpegCodecs::byFFCodecID(codec->id)->codecID,
                                     formats,
                                     options};
    }
}

void VideoEncoderFFmpegElementPrivate::adjustDefaults()
{
    for (auto &codec: this->m_codecs) {
        if (codec.name == "libvpx" || codec.name == "libvpx-vp9") {
            for (auto &option: codec.options) {
                if (option.name == "quality")
                    option.value = "realtime";
                else if (option.name == "deadline")
                    option.value = "realtime";
                else if (option.name == "speed")
                    option.value = codec.name == "libvpx"? 16: 9;
            }
        } else if (codec.name == "libaom-av1") {
            for (auto &option: codec.options) {
                if (option.name == "usage")
                    option.value = "realtime";
            }
        } else if (codec.name == "av1_amf") {
            for (auto &option: codec.options)
                if (option.name == "usage")
                    option.value = "ultralowlatency";
                else if (option.name == "quality")
                    option.value = "speed";
                else if (option.name == "preset")
                    option.value = "speed";
                else if (option.name == "latency")
                    option.value = "real_time";
        } else if (codec.name == "av1_nvenc") {
            for (auto &option: codec.options)
                if (option.name == "preset")
                    option.value = "fast";
                else if (option.name == "tune")
                    option.value = "ull";
        } else if (codec.name == "av1_mf") {
            for (auto &option: codec.options)
                if (option.name == "scenario")
                    option.value = "live_streaming";
        } else if (codec.name == "libx264" || codec.name == "libx264rgb") {
            for (auto &option: codec.options)
                if (option.name == "preset") {
                    option.value = "ultrafast";

                    if (option.menu.isEmpty()) {
                        static const char *ffx264EncPresets[] = {
                            "ultrafast",
                            "superfast",
                            "veryfast",
                            "faster",
                            "fast",
                            "medium",
                            "slow",
                            "slower",
                            "veryslow",
                            "placebo",
                            nullptr
                        };

                        QVector<MenuOption> menu;

                        for (auto preset = ffx264EncPresets; *preset; preset++)
                            menu << MenuOption {*preset, "", *preset};

                        option.menu = menu;
                    }
                } else if (option.name == "tune") {
                    option.value = "zerolatency";

                    if (option.menu.isEmpty()) {
                        static const char *ffx264EncTunes[] = {
                            "film",
                            "animation",
                            "grain",
                            "stillimage",
                            "psnr",
                            "ssim",
                            "fastdecode",
                            "zerolatency",
                            nullptr
                        };

                        QVector<MenuOption> menu;

                        for (auto tune = ffx264EncTunes; *tune; tune++)
                            menu << MenuOption {*tune, "", *tune};

                        option.menu = menu;
                    }
                }
        } else if (codec.name == "h264_amf") {
            for (auto &option: codec.options)
                if (option.name == "usage")
                    option.value = "ultralowlatency";
                else if (option.name == "quality")
                    option.value = "speed";
                else if (option.name == "preset")
                    option.value = "speed";
                else if (option.name == "latency")
                    option.value = true;
        } else if (codec.name == "h264_nvenc") {
            for (auto &option: codec.options)
                if (option.name == "preset")
                    option.value = "fast";
                else if (option.name == "tune")
                    option.value = "ull";
        } else if (codec.name == "h264_qsv") {
            for (auto &option: codec.options)
                if (option.name == "veryfast")
                    option.value = "ll";
                else if (option.name == "scenario")
                    option.value = "livestreaming";
        } else if (codec.name == "h264_mf") {
            for (auto &option: codec.options)
                if (option.name == "scenario")
                    option.value = "live_streaming";
        }
    }
}

VideoEncoderFFmpegElement::OptionType VideoEncoderFFmpegElementPrivate::optionType(AVOptionType avType) const
{
    static const struct
    {
        AVOptionType avType;
        VideoEncoderFFmpegElement::OptionType type;
    } ffmpegVideoEncCodecOptionTypes [] = {
        {AV_OPT_TYPE_BOOL      , VideoEncoderFFmpegElement::OptionType_Boolean  },
        {AV_OPT_TYPE_COLOR     , VideoEncoderFFmpegElement::OptionType_Color    },
        {AV_OPT_TYPE_CONST     , VideoEncoderFFmpegElement::OptionType_Number   },
        {AV_OPT_TYPE_DOUBLE    , VideoEncoderFFmpegElement::OptionType_Number   },
        {AV_OPT_TYPE_DURATION  , VideoEncoderFFmpegElement::OptionType_Number   },
        {AV_OPT_TYPE_FLAGS     , VideoEncoderFFmpegElement::OptionType_Flags    },
        {AV_OPT_TYPE_FLOAT     , VideoEncoderFFmpegElement::OptionType_Number   },
        {AV_OPT_TYPE_IMAGE_SIZE, VideoEncoderFFmpegElement::OptionType_ImageSize},
        {AV_OPT_TYPE_INT       , VideoEncoderFFmpegElement::OptionType_Number   },
        {AV_OPT_TYPE_INT64     , VideoEncoderFFmpegElement::OptionType_Number   },
        {AV_OPT_TYPE_RATIONAL  , VideoEncoderFFmpegElement::OptionType_Frac     },
        {AV_OPT_TYPE_STRING    , VideoEncoderFFmpegElement::OptionType_String   },
        {AV_OPT_TYPE_UINT      , VideoEncoderFFmpegElement::OptionType_Number   },
        {AV_OPT_TYPE_UINT64    , VideoEncoderFFmpegElement::OptionType_Number   },
        {AV_OPT_TYPE_VIDEO_RATE, VideoEncoderFFmpegElement::OptionType_Frac     },
        {AVOptionType(0)       , VideoEncoderFFmpegElement::OptionType_Unknown  },
    };

    auto type = ffmpegVideoEncCodecOptionTypes;

    for (; type->type != VideoEncoderFFmpegElement::OptionType_Unknown; ++type)
        if (type->avType == avType)
            return type->type;

    return type->type;
}

const CodecOption *VideoEncoderFFmpegElementPrivate::codecOption(const QString &option) const
{
    auto it = std::find_if(this->m_codecs.constBegin(),
                           this->m_codecs.constEnd(),
                           [this] (const CodecInfo &codec) -> bool {
        return codec.name == self->codec();
    });

    if (it == this->m_codecs.constEnd())
        return nullptr;

    auto oit = std::find_if(it->options.constBegin(),
                            it->options.constEnd(),
                           [&option] (const CodecOption &codecOption) -> bool {
        return codecOption.name == option;
    });

    if (oit == it->options.constEnd())
        return nullptr;

    return &(*oit);
}

AVDictionary *VideoEncoderFFmpegElementPrivate::readCodecOptions() const
{
    AVDictionary *options = nullptr;

    auto codec
            = std::find_if(this->m_codecs.constBegin(),
                           this->m_codecs.constEnd(),
                           [this] (const CodecInfo &codec) -> bool {
        return codec.name == self->codec();
    });

    if (codec == this->m_codecs.constEnd())
        return nullptr;

    for (auto &option: codec->options) {
        auto value = option.value.toString();

        av_dict_set(&options,
                    option.name.toStdString().c_str(),
                    value.isEmpty()? nullptr: value.toStdString().c_str(),
                    0);
    }

    return options;
}

bool VideoEncoderFFmpegElementPrivate::init()
{
    this->uninit();

    auto inputCaps = self->inputCaps();

    if (!inputCaps) {
        qCritical() << "Invalid input format.";

        return false;
    }

    auto encoder =
            avcodec_find_encoder_by_name(self->codec().toStdString().c_str());

    if (!encoder) {
        qCritical() << "Encoder not found";

        return false;
    }

    this->m_context = avcodec_alloc_context3(encoder);

    if (!this->m_context) {
        qCritical() << "Context not created";

        return false;
    }

    this->m_context->width = this->m_videoConverter.outputCaps().width();
    this->m_context->height = this->m_videoConverter.outputCaps().height();
    this->m_context->pix_fmt =
            PixelFormatsTable::byFormat(this->m_videoConverter.outputCaps().format())->ffFormat;
    this->m_context->framerate =
    {int(this->m_videoConverter.outputCaps().fps().num()),
     int(this->m_videoConverter.outputCaps().fps().den())};
    this->m_context->time_base = {this->m_context->framerate.den,
                                  this->m_context->framerate.num};
    this->m_context->bit_rate = self->bitrate();
    this->m_context->gop_size =
            qMax(self->gop() * this->m_videoConverter.outputCaps().fps().num()
                 / (1000 * this->m_videoConverter.outputCaps().fps().den()), 1);

    if (this->m_globalHeaders)
        this->m_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    auto options = this->readCodecOptions();
    auto result = avcodec_open2(this->m_context, encoder, &options);
    av_dict_free(&options);

    if (result < 0) {
        char error[1024];
        av_strerror(result, error, 1024);
        qCritical() << "Failed to open the codec:" << error;

        return false;
    }

    this->updateHeaders();

    if (this->m_fpsControl) {
        this->m_fpsControl->setProperty("fps", QVariant::fromValue(this->m_videoConverter.outputCaps().fps()));
        this->m_fpsControl->setProperty("fillGaps", self->fillGaps());
        QMetaObject::invokeMethod(this->m_fpsControl.data(),
                                  "restart",
                                  Qt::DirectConnection);
    }

    this->m_encodedTimePts = 0;
    this->m_initialized = true;

    return true;
}

void VideoEncoderFFmpegElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;

    auto result = avcodec_send_frame(this->m_context, nullptr);

    if (result >= 0) {
        auto packet = av_packet_alloc();

        while (avcodec_receive_packet(this->m_context, packet) >= 0)
            this->sendFrame(packet);

        av_packet_free(&packet);
    } else {
        char error[1024];
        av_strerror(result, error, 1024);
        qCritical() << "Failed to encode the frame:" << error;
    }

    avcodec_free_context(&this->m_context);
    this->m_context = nullptr;

    this->m_codecParameters.clear();

    if (this->m_fpsControl)
        QMetaObject::invokeMethod(this->m_fpsControl.data(),
                                  "restart",
                                  Qt::DirectConnection);

    this->m_paused = false;
}

void VideoEncoderFFmpegElementPrivate::updateHeaders()
{
    QByteArray headers;
    QDataStream ds(&headers, QIODeviceBase::WriteOnly);
    ds << quint64(this->m_context->codec);
    this->m_codecParameters =
            AVCodecParametersPtr(avcodec_parameters_alloc(),
                                 [] (AVCodecParameters *parameters) {
        avcodec_parameters_free(&parameters);
    });
    auto result =
            avcodec_parameters_from_context(this->m_codecParameters.data(),
                                            this->m_context);

    if (result < 0) {
        char error[1024];
        av_strerror(result, error, 1024);
        qCritical() << "Failed to read the codec parameters:" << error;

        return;
    }
    ds.writeRawData(reinterpret_cast<char *>(this->m_codecParameters.data()),
                    sizeof(AVCodecParameters));

    AkCompressedVideoPacket headerPacket(this->m_outputCaps, headers.size());
    memcpy(headerPacket.data(), headers.constData(), headerPacket.size());
    headerPacket.setTimeBase(this->m_outputCaps.rawCaps().fps().invert());
    headerPacket.setFlags(AkCompressedVideoPacket::VideoPacketTypeFlag_Header);
    this->m_headers = {headerPacket};
    emit self->headersChanged(self->headers());
}

void VideoEncoderFFmpegElementPrivate::updateOutputCaps()
{
    auto inputCaps = self->inputCaps();

    if (!inputCaps) {
        if (!this->m_outputCaps)
            return;

        this->m_outputCaps = {};
        emit self->outputCapsChanged({});

        return;
    }

    auto codecID = self->codecID(self->codec());

    if (codecID == AkCompressedVideoCaps::VideoCodecID_unknown) {
        if (!this->m_outputCaps)
            return;

        this->m_outputCaps = {};
        emit self->outputCapsChanged({});

        return;
    }

    auto it = std::find_if(this->m_codecs.constBegin(),
                           this->m_codecs.constEnd(),
                           [this] (const CodecInfo &codec) -> bool {
        return codec.name == self->codec();
    });

    if (it == this->m_codecs.constEnd()) {
        if (!this->m_outputCaps)
            return;

        this->m_outputCaps = {};
        emit self->outputCapsChanged({});

        return;
    }

    AkVideoCaps::PixelFormat format =
            it->formats.contains(inputCaps.format())?
                inputCaps.format():
                it->formats.first();

    auto fps = inputCaps.fps();

    if (!fps)
        fps = {30, 1};

    this->m_videoConverter.setOutputCaps({format,
                                          inputCaps.width(),
                                          inputCaps.height(),
                                          fps});
    AkCompressedVideoCaps outputCaps(codecID,
                                     this->m_videoConverter.outputCaps(),
                                     self->bitrate());

    if (this->m_outputCaps == outputCaps)
        return;

    this->m_outputCaps = outputCaps;
    emit self->outputCapsChanged(outputCaps);
}

void VideoEncoderFFmpegElementPrivate::encodeFrame(const AkVideoPacket &src)
{
    this->m_id = src.id();
    this->m_index = src.index();

    // Write the current frame.
    auto frame = av_frame_alloc();

    for (int plane = 0; plane < src.planes(); ++plane) {
        frame->linesize[plane] = src.lineSize(plane);
        frame->data[plane] = const_cast<quint8 *>(src.constPlane(plane));
    }

    frame->format = this->m_context->pix_fmt;
    frame->width = this->m_context->width;
    frame->height = this->m_context->height;
    frame->pts = src.pts();
    frame->duration = src.duration();
    frame->time_base = {int(src.timeBase().num()), int(src.timeBase().den())};

    auto result = avcodec_send_frame(this->m_context, frame);
    av_frame_free(&frame);

    if (result >= 0) {
        auto packet = av_packet_alloc();

        while (avcodec_receive_packet(this->m_context, packet) >= 0)
            this->sendFrame(packet);

        av_packet_free(&packet);
    } else {
        char error[1024];
        av_strerror(result, error, 1024);
        qCritical() << "Failed to encode the frame:" << error;
    }

    this->m_encodedTimePts = src.pts() + src.duration();
    emit self->encodedTimePtsChanged(this->m_encodedTimePts);
}

void VideoEncoderFFmpegElementPrivate::sendFrame(const AVPacket *avPacket) const
{
    AkCompressedVideoPacket packet(this->m_outputCaps, avPacket->size);
    memcpy(packet.data(), avPacket->data, packet.size());
    packet.setFlags(avPacket->flags & AV_PKT_FLAG_KEY?
                        AkCompressedVideoPacket::VideoPacketTypeFlag_KeyFrame:
                        AkCompressedVideoPacket::VideoPacketTypeFlag_None);
    packet.setPts(avPacket->pts);
    packet.setDts(avPacket->dts);
    packet.setDuration(avPacket->duration);
    packet.setTimeBase(this->m_outputCaps.rawCaps().fps().invert());
    packet.setId(this->m_id);
    packet.setIndex(this->m_index);

    emit self->oStream(packet);
}

#include "moc_videoencoderffmpegelement.cpp"
