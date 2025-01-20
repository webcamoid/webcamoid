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

#include <QMutex>
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
#include <vpx/vp8cx.h>
#include <vpx/vpx_encoder.h>

#include "videoencodervpxelement.h"

struct VpxCodecs
{
    AkVideoEncoderCodecID codecID;
    QString name;
    QString description;

    static inline const VpxCodecs *table()
    {
        static const VpxCodecs vpxEncCodecsTable[] = {
            {AkCompressedVideoCaps::VideoCodecID_vp8    , "vp8", "VP8 (libvpx)"},
            {AkCompressedVideoCaps::VideoCodecID_vp9    , "vp9", "VP9 (libvpx)"},
            {AkCompressedVideoCaps::VideoCodecID_unknown, ""   , ""            },
        };

        return vpxEncCodecsTable;
    }

    static inline QStringList codecs()
    {
        QStringList codecs;

        for (auto codec = table();
             codec->codecID != AkCompressedVideoCaps::VideoCodecID_unknown;
             codec++) {
            codecs << codec->name;
        }

        return codecs;
    }

    static inline const VpxCodecs *byCodecName(const QString &codecName)
    {
        auto codec = table();

        for (;
             codec->codecID != AkCompressedVideoCaps::VideoCodecID_unknown;
             codec++) {
            if (codec->name == codecName)
                return codec;
        }

        return codec;
    }
};

struct VpxPixFormatTable
{
    AkVideoCaps::PixelFormat pixFormat;
    vpx_img_fmt_t vpxFormat;
    size_t depth;
    vpx_codec_flags_t flags;
    unsigned int profile;

    static inline const VpxPixFormatTable *table(AkVideoEncoderCodecID codecID)
    {
        static const VpxPixFormatTable vp8PixFormatTable[] = {
            {AkVideoCaps::Format_nv12     , VPX_IMG_FMT_NV12  , 8 , 0                         , 0},
            {AkVideoCaps::Format_yvu420p  , VPX_IMG_FMT_YV12  , 8 , 0                         , 0},
            {AkVideoCaps::Format_yuv420p  , VPX_IMG_FMT_I420  , 8 , 0                         , 0},
            {AkVideoCaps::Format_none     , VPX_IMG_FMT_NONE  , 0 , 0                         , 0},
        };
        static const VpxPixFormatTable vp9PixFormatTable[] = {
            {AkVideoCaps::Format_nv12     , VPX_IMG_FMT_NV12  , 8 , 0                         , 0},
            {AkVideoCaps::Format_yvu420p  , VPX_IMG_FMT_YV12  , 8 , 0                         , 0},
            {AkVideoCaps::Format_yuv420p  , VPX_IMG_FMT_I420  , 8 , 0                         , 0},
            {AkVideoCaps::Format_yuv422p  , VPX_IMG_FMT_I422  , 8 , 0                         , 1},
            {AkVideoCaps::Format_yuv440p  , VPX_IMG_FMT_I440  , 8 , 0                         , 1},
            {AkVideoCaps::Format_yuv444p  , VPX_IMG_FMT_I444  , 8 , 0                         , 1},
            {AkVideoCaps::Format_yuv420p10, VPX_IMG_FMT_I42016, 10, VPX_CODEC_USE_HIGHBITDEPTH, 2},
            {AkVideoCaps::Format_yuv420p12, VPX_IMG_FMT_I42016, 12, VPX_CODEC_USE_HIGHBITDEPTH, 2},
            {AkVideoCaps::Format_yuv422p10, VPX_IMG_FMT_I42216, 10, VPX_CODEC_USE_HIGHBITDEPTH, 3},
            {AkVideoCaps::Format_yuv422p12, VPX_IMG_FMT_I42216, 12, VPX_CODEC_USE_HIGHBITDEPTH, 3},
            {AkVideoCaps::Format_yuv440p10, VPX_IMG_FMT_I44016, 10, VPX_CODEC_USE_HIGHBITDEPTH, 3},
            {AkVideoCaps::Format_yuv440p12, VPX_IMG_FMT_I44016, 12, VPX_CODEC_USE_HIGHBITDEPTH, 3},
            {AkVideoCaps::Format_yuv444p10, VPX_IMG_FMT_I44416, 10, VPX_CODEC_USE_HIGHBITDEPTH, 3},
            {AkVideoCaps::Format_yuv444p12, VPX_IMG_FMT_I44416, 12, VPX_CODEC_USE_HIGHBITDEPTH, 3},
            {AkVideoCaps::Format_none     , VPX_IMG_FMT_NONE  , 0 , 0                         , 0},
        };

        return codecID == AkCompressedVideoCaps::VideoCodecID_vp9?
                    vp9PixFormatTable:
                    vp8PixFormatTable;
    }

    static inline const VpxPixFormatTable *byPixFormat(AkVideoEncoderCodecID codecID,
                                                       AkVideoCaps::PixelFormat format)
    {
        auto fmt = table(codecID);

        for (; fmt->pixFormat != AkVideoCaps::Format_none; fmt++)
            if (fmt->pixFormat == format)
                return fmt;

        return fmt;
    }

    static inline const VpxPixFormatTable *byVpxFormat(AkVideoEncoderCodecID codecID,
                                                       vpx_img_fmt_t format,
                                                       size_t depth)
    {
        auto fmt = table(codecID);

        for (; fmt->pixFormat != AkVideoCaps::Format_none; fmt++)
            if (fmt->vpxFormat == format && fmt->depth == depth)
                return fmt;

        return fmt;
    }
};

class VideoEncoderVpxElementPrivate
{
    public:
        VideoEncoderVpxElement *self;
        AkVideoConverter m_videoConverter;
        AkCompressedVideoCaps m_outputCaps;
        AkPropertyOptions m_options;
        QByteArray m_headers;
        vpx_codec_iface_t *m_interface {nullptr};
        vpx_codec_ctx_t m_encoder;
        vpx_image_t m_frame;
        QMutex m_mutex;
        qint64 m_id {0};
        int m_index {0};
        bool m_initialized {false};
        bool m_paused {false};
        int m_deadline {VPX_DL_REALTIME};
        qint64 m_encodedTimePts {0};
        AkElementPtr m_fpsControl {akPluginManager->create<AkElement>("VideoFilter/FpsControl")};

        explicit VideoEncoderVpxElementPrivate(VideoEncoderVpxElement *self);
        ~VideoEncoderVpxElementPrivate();
        bool init();
        void uninit();
        void updateHeaders();
        void updateOutputCaps(const AkVideoCaps &inputCaps);
        static void printError(vpx_codec_err_t error,
                               vpx_codec_ctx_t *codecContext=nullptr);
        void encodeFrame(const AkVideoPacket &src);
        void sendFrame(const vpx_codec_cx_pkt_t *vpxPacket) const;
        int vp9Level(const AkVideoCaps &caps) const;
};

VideoEncoderVpxElement::VideoEncoderVpxElement():
    AkVideoEncoder()
{
    this->d = new VideoEncoderVpxElementPrivate(this);
    this->setCodec(this->codecs().value(0));
}

VideoEncoderVpxElement::~VideoEncoderVpxElement()
{
    this->d->uninit();
    delete this->d;
}

QStringList VideoEncoderVpxElement::codecs() const
{
    return VpxCodecs::codecs();
}

AkVideoEncoderCodecID VideoEncoderVpxElement::codecID(const QString &codec) const
{
    return VpxCodecs::byCodecName(codec)->codecID;
}

QString VideoEncoderVpxElement::codecDescription(const QString &codec) const
{
    return VpxCodecs::byCodecName(codec)->description;
}

AkCompressedVideoCaps VideoEncoderVpxElement::outputCaps() const
{
    return this->d->m_outputCaps;
}

QByteArray VideoEncoderVpxElement::headers() const
{
    return this->d->m_headers;
}

qint64 VideoEncoderVpxElement::encodedTimePts() const
{
    return this->d->m_encodedTimePts;
}

AkPropertyOptions VideoEncoderVpxElement::options() const
{
    return this->d->m_options;
}

AkPacket VideoEncoderVpxElement::iVideoStream(const AkVideoPacket &packet)
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

bool VideoEncoderVpxElement::setState(ElementState state)
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

VideoEncoderVpxElementPrivate::VideoEncoderVpxElementPrivate(VideoEncoderVpxElement *self):
    self(self)
{
    this->m_options = {
        {"speed" ,
         QObject::tr("Speed"),
         QObject::tr("Encoding speed"),
         AkPropertyOption::OptionType_Number,
         0.0,
         16.0,
         1.0,
         16.0,
         {}},
        {"tuneContent" ,
         QObject::tr("Tune content"),
         "",
         AkPropertyOption::OptionType_Number,
         VP9E_CONTENT_DEFAULT,
         VP9E_CONTENT_FILM,
         1.0,
         VP9E_CONTENT_DEFAULT,
         {{"default", QObject::tr("Default"), "", VP9E_CONTENT_DEFAULT},
          {"screen" , QObject::tr("Screen") , "", VP9E_CONTENT_SCREEN },
          {"film"   , QObject::tr("Film")   , "", VP9E_CONTENT_FILM   }}},
        {"deadline" ,
         QObject::tr("Deadline"),
         "",
         AkPropertyOption::OptionType_Number,
         VPX_DL_BEST_QUALITY,
         VPX_DL_GOOD_QUALITY,
         1.0,
         qreal(VPX_DL_REALTIME),
         {{"best_quality", QObject::tr("Best quality"), "", qreal(VPX_DL_BEST_QUALITY)},
          {"realtime"    , QObject::tr("Real time")   , "", qreal(VPX_DL_REALTIME)    },
          {"good_quality", QObject::tr("Good quality"), "", qreal(VPX_DL_GOOD_QUALITY)}}},
        {"lossless",
         QObject::tr("Lossless"),
         QObject::tr("Enable lossless encoding"),
         AkPropertyOption::OptionType_Boolean,
         0.0,
         1.0,
         1.0,
         0.0,
         {}},
        {"errorResilient" ,
         QObject::tr("Error resilient"),
         QObject::tr("Protect the stream against packet loss"),
         AkPropertyOption::OptionType_Flags,
         0.0,
         1.0,
         1.0,
         0.0,
         {{"default"   , QObject::tr("Default")   , QObject::tr("Improve resiliency against losses of whole frames")                   , VPX_ERROR_RESILIENT_DEFAULT   },
          {"partitions", QObject::tr("Partitions"), QObject::tr("The frame partitions are independently decodable by the bool decoder"), VPX_ERROR_RESILIENT_PARTITIONS}}},
    };

    this->m_videoConverter.setAspectRatioMode(AkVideoConverter::AspectRatioMode_Fit);

    QObject::connect(self,
                     &AkVideoEncoder::inputCapsChanged,
                     [this] (const AkVideoCaps &inputCaps) {
                         this->updateOutputCaps(inputCaps);
                     });

    if (this->m_fpsControl)
        QObject::connect(this->m_fpsControl.data(),
                         &AkElement::oStream,
                         [this] (const AkPacket &packet) {
                             this->encodeFrame(packet);
                         });
}

VideoEncoderVpxElementPrivate::~VideoEncoderVpxElementPrivate()
{

}

bool VideoEncoderVpxElementPrivate::init()
{
    this->uninit();
    auto codecID = VpxCodecs::byCodecName(self->codec())->codecID;

    switch (codecID) {
    case AkCompressedVideoCaps::VideoCodecID_vp8:
        this->m_interface = vpx_codec_vp8_cx();
        break;

    case AkCompressedVideoCaps::VideoCodecID_vp9:
        this->m_interface = vpx_codec_vp9_cx();
        break;

    default:
        return false;
    }

    if (!this->m_interface) {
        qCritical() << "VPX Codec interface was not initialized.";

        return false;
    }

    auto inputCaps = self->inputCaps();

    if (!inputCaps) {
        qCritical() << "Invalid input format.";

        return false;
    }

    auto eqFormat =
            VpxPixFormatTable::byPixFormat(codecID, this->m_videoConverter.outputCaps().format());

    if (eqFormat->pixFormat == AkVideoCaps::Format_none)
        eqFormat = VpxPixFormatTable::byPixFormat(codecID, AkVideoCaps::Format_yuv420p);

    vpx_codec_enc_cfg_t codecConfigs;
    memset(&codecConfigs, 0, sizeof(vpx_codec_enc_cfg));
    auto result = vpx_codec_enc_config_default(this->m_interface,
                                               &codecConfigs,
                                               0);

    if (result != VPX_CODEC_OK) {
        printError(result);

        return false;
    }

    codecConfigs.g_profile = eqFormat->profile;
    codecConfigs.g_w = this->m_videoConverter.outputCaps().width();
    codecConfigs.g_h = this->m_videoConverter.outputCaps().height();
    codecConfigs.g_timebase.num =
            this->m_videoConverter.outputCaps().fps().den();
    codecConfigs.g_timebase.den =
            this->m_videoConverter.outputCaps().fps().num();
    codecConfigs.g_threads = QThread::idealThreadCount();
    codecConfigs.rc_end_usage = VPX_CBR;
    codecConfigs.rc_target_bitrate = self->bitrate() / 1000;
    codecConfigs.g_bit_depth = vpx_bit_depth(eqFormat->depth);
    codecConfigs.g_input_bit_depth = eqFormat->depth;
    codecConfigs.g_error_resilient = self->optionValue("errorResilient").toInt();
    codecConfigs.g_pass = VPX_RC_ONE_PASS;
    codecConfigs.kf_max_dist =
            qMax(self->gop() * this->m_videoConverter.outputCaps().fps().num()
                 / (1000 * this->m_videoConverter.outputCaps().fps().den()), 1);

    memset(&this->m_encoder, 0, sizeof(vpx_codec_ctx));
    result = vpx_codec_enc_init(&this->m_encoder,
                                this->m_interface,
                                &codecConfigs,
                                eqFormat->flags);

    if (result != VPX_CODEC_OK) {
        printError(result, &this->m_encoder);

        return false;
    }

    int speed = self->optionValue("speed").toInt();
    speed = codecID == AkCompressedVideoCaps::VideoCodecID_vp9?
                qBound(0, 9 * speed / 16, 9):
                qBound(0, speed, 16);

    vpx_codec_control(&this->m_encoder, VP8E_SET_CPUUSED, speed);

    if (codecID == AkCompressedVideoCaps::VideoCodecID_vp9) {
        auto level = this->vp9Level(this->m_videoConverter.outputCaps());
        vpx_codec_control(&this->m_encoder,
                          VP9E_SET_TARGET_LEVEL,
                          static_cast<unsigned int>(level));
        vpx_codec_control(&this->m_encoder,
                          VP9E_SET_LOSSLESS,
                          self->optionValue("lossless").toUInt());

        int tune = self->optionValue("tuneContent").toInt();
        vpx_codec_control(&this->m_encoder, VP9E_SET_TUNE_CONTENT, tune);
    } else {
        unsigned int screenContentMode =
                self->optionValue("tuneContent").toInt() == VP9E_CONTENT_SCREEN;
        vpx_codec_control(&this->m_encoder,
                          VP8E_SET_SCREEN_CONTENT_MODE,
                          screenContentMode);
    }

    memset(&this->m_frame, 0, sizeof(vpx_image_t));

    if (!vpx_img_alloc(&this->m_frame,
                       eqFormat->vpxFormat,
                       this->m_videoConverter.outputCaps().width(),
                       this->m_videoConverter.outputCaps().height(),
                       1)) {
        qCritical() << "Failed to allocate the input frame";
        vpx_codec_destroy(&this->m_encoder);

        return false;
    }

    this->m_deadline = self->optionValue("deadline").toInt();
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

void VideoEncoderVpxElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;
    vpx_codec_iter_t iter = nullptr;

    for (;;) {
        auto packet = vpx_codec_get_cx_data(&this->m_encoder, &iter);

        if (!packet)
            break;

        if (packet->kind != VPX_CODEC_CX_FRAME_PKT)
            continue;

        this->sendFrame(packet);
    }

    vpx_img_free(&this->m_frame);
    vpx_codec_destroy(&this->m_encoder);

    if (this->m_fpsControl)
        QMetaObject::invokeMethod(this->m_fpsControl.data(),
                                  "restart",
                                  Qt::DirectConnection);

    this->m_paused = false;
}

void VideoEncoderVpxElementPrivate::updateHeaders()
{
#if 0
    // VP9 seems to provide stream headers, but crash when enabled.
    // Disabling for now.

    auto vpxHeaders = vpx_codec_get_global_headers(&this->m_encoder);

    if (!vpxHeaders)
        return;

    QByteArray headers(reinterpret_cast<char *>(vpxHeaders->buf),
                       vpxHeaders->sz);
    free(vpxHeaders->buf);
    free(vpxHeaders);

    if (this->m_headers == headers)
        return;

    this->m_headers = headers;
    emit self->headersChanged(headers);
#endif
}

void VideoEncoderVpxElementPrivate::updateOutputCaps(const AkVideoCaps &inputCaps)
{
    if (!inputCaps) {
        if (!this->m_outputCaps)
            return;

        this->m_outputCaps = AkCompressedVideoCaps();
        emit self->outputCapsChanged(this->m_outputCaps);

        return;
    }

    auto codecID = self->codecID(self->codec());

    if (codecID == AkCompressedVideoCaps::VideoCodecID_unknown) {
        if (!this->m_outputCaps)
            return;

        this->m_outputCaps = AkCompressedVideoCaps();
        emit self->outputCapsChanged(this->m_outputCaps);

        return;
    }

    auto eqFormat = VpxPixFormatTable::byPixFormat(codecID, inputCaps.format());

    if (eqFormat->pixFormat == AkVideoCaps::Format_none)
        eqFormat = VpxPixFormatTable::byPixFormat(codecID,
                                                  AkVideoCaps::Format_yuv420p);

    auto fps = inputCaps.fps();

    if (!fps)
        fps = {30, 1};

    this->m_videoConverter.setOutputCaps({eqFormat->pixFormat,
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

void VideoEncoderVpxElementPrivate::printError(vpx_codec_err_t error,
                                               vpx_codec_ctx_t *codecContext)
{
    if (codecContext) {
        auto errorStr = vpx_codec_error_detail(codecContext);

        if (QString(errorStr).isEmpty())
            qCritical() << vpx_codec_err_to_string(error);
        else
            qCritical() << errorStr;
    } else {
        qCritical() << vpx_codec_err_to_string(error);
    }
}

void VideoEncoderVpxElementPrivate::encodeFrame(const AkVideoPacket &src)
{
    this->m_id = src.id();
    this->m_index = src.index();

    // Write the current frame.
    for (int plane = 0; plane < src.planes(); ++plane) {
        auto planeData = this->m_frame.planes[plane];
        auto oLineSize = this->m_frame.stride[plane];
        auto lineSize = qMin<size_t>(src.lineSize(plane), oLineSize);
        auto heightDiv = src.heightDiv(plane);

        for (int y = 0; y < src.caps().height(); ++y) {
            auto ys = y >> heightDiv;
            memcpy(planeData + ys * oLineSize,
                   src.constLine(plane, y),
                   lineSize);
        }
    }

    auto result = vpx_codec_encode(&this->m_encoder,
                                   &this->m_frame,
                                   src.pts(),
                                   src.duration(),
                                   0,
                                   this->m_deadline);

    if (result != VPX_CODEC_OK)
        this->printError(result, &this->m_encoder);

    vpx_codec_iter_t iter = nullptr;

    for (;;) {
        auto vpxPacket = vpx_codec_get_cx_data(&this->m_encoder, &iter);

        if (!vpxPacket)
            break;

        if (vpxPacket->kind != VPX_CODEC_CX_FRAME_PKT)
            continue;

        this->sendFrame(vpxPacket);
    }

    this->m_encodedTimePts = src.pts() + src.duration();
    emit self->encodedTimePtsChanged(this->m_encodedTimePts);
}

void VideoEncoderVpxElementPrivate::sendFrame(const vpx_codec_cx_pkt_t *vpxPacket) const
{
    AkCompressedVideoPacket packet(this->m_outputCaps,
                                   vpxPacket->data.frame.sz);
    memcpy(packet.data(), vpxPacket->data.frame.buf, packet.size());
    packet.setFlags(vpxPacket->data.frame.flags & VPX_FRAME_IS_KEY?
                        AkCompressedVideoPacket::VideoPacketTypeFlag_KeyFrame:
                        AkCompressedVideoPacket::VideoPacketTypeFlag_None);
    packet.setPts(vpxPacket->data.frame.pts);
    packet.setDts(vpxPacket->data.frame.pts);
    packet.setDuration(vpxPacket->data.frame.duration);
    packet.setTimeBase({this->m_encoder.config.enc->g_timebase.num,
                        this->m_encoder.config.enc->g_timebase.den});
    packet.setId(this->m_id);
    packet.setIndex(this->m_index);

    emit self->oStream(packet);
}

int VideoEncoderVpxElementPrivate::vp9Level(const AkVideoCaps &caps) const
{
    // https://www.webmproject.org/vp9/levels

    static const struct Vp9LevelsDef
    {
        int level;
        quint64 lumaSampleRate;
        quint64 maxLumaPictureSize;
        int maxBitrate;
        int maxDimension;
    } vp9Levels[] = {
        {10, 829440L    , 36864   , 200   , 512  },
        {11, 2764800L   , 73728   , 800   , 768  },
        {20, 4608000L   , 122880  , 1800  , 960  },
        {21, 9216000L   , 245760  , 3600  , 1344 },
        {30, 20736000L  , 552960  , 7200  , 2048 },
        {31, 36864000L  , 983040  , 12000 , 2752 },
        {40, 83558400L  , 2228224 , 18000 , 4160 },
        {41, 160432128L , 2228224 , 30000 , 4160 },
        {50, 311951360L , 8912896 , 60000 , 8384 },
        {51, 588251136L , 8912896 , 120000, 8384 },
        {52, 1176502272L, 8912896 , 180000, 8384 },
        {60, 1176502272L, 35651584, 180000, 16832},
        {61, 2353004544L, 35651584, 240000, 16832},
        {62, 4706009088L, 35651584, 480000, 16832},
        {0 , 0L         , 0       , 0     , 0    }
    };

    quint64 lumaPictureSize = caps.width() * caps.height();
    quint64 lumaSampleRate = qRound64(lumaPictureSize * caps.fps().value());
    int bitrate = self->bitrate();
    int dimension = qMax(caps.width(), caps.height());

    for (auto level = vp9Levels; level->level; ++level)
        if (level->lumaSampleRate >= lumaSampleRate
            && level->maxLumaPictureSize >= lumaPictureSize
            && 1000 * level->maxBitrate >= bitrate
            && level->maxDimension >= dimension) {
            return level->level;
        }

    return 0;
}

#include "moc_videoencodervpxelement.cpp"
