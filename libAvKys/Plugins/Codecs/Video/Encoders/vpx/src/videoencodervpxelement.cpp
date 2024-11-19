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
#include <QQmlContext>
#include <QThread>
#include <QVariant>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akcompressedvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>
#include <akcompressedvideopacket.h>
#include <vpx/vp8cx.h>
#include <vpx/vpx_encoder.h>

#include "videoencodervpxelement.h"

struct VpxPixFormatTable
{
    AkVideoCaps::PixelFormat pixFormat;
    vpx_img_fmt_t vpxFormat;
    size_t depth;
    vpx_codec_flags_t flags;
    unsigned int profile;

    static inline const VpxPixFormatTable *table()
    {
        static const VpxPixFormatTable vpxPixFormatTable[] = {
            {AkVideoCaps::Format_nv12     , VPX_IMG_FMT_NV12  , 8 , 0                         , 0},
            {AkVideoCaps::Format_yvu420p  , VPX_IMG_FMT_YV12  , 8 , 0                         , 0},
            {AkVideoCaps::Format_yuv420p  , VPX_IMG_FMT_I420  , 8 , 0                         , 0},
#ifdef USE_VP9_INTERFACE
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
#endif
        };

        return vpxPixFormatTable;
    }

    static inline const VpxPixFormatTable *byPixFormat(AkVideoCaps::PixelFormat format)
    {
        auto fmt = table();

        for (; fmt->vpxFormat != VPX_IMG_FMT_NONE; fmt++)
            if (fmt->pixFormat == format)
                return fmt;

        return fmt;
    }

    static inline const VpxPixFormatTable *byVpxFormat(vpx_img_fmt_t format,
                                                       size_t depth)
    {
        auto fmt = table();

        for (; fmt->vpxFormat != VPX_IMG_FMT_NONE; fmt++)
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
        VideoEncoderVpxElement::ErrorResilientFlag m_errorResilient {VideoEncoderVpxElement::ErrorResilientFlag_NoFlags};
        int m_deadline {VideoEncoderVpxElement::Deadline_Realtime};
        int m_speed {16};
        bool m_lossless {false};
        VideoEncoderVpxElement::TuneContent m_tuneContent {VideoEncoderVpxElement::TuneContent_Default};
        vpx_codec_iface_t *m_interface {nullptr};
        vpx_codec_ctx_t m_encoder;
        vpx_image_t m_frame;
        qreal m_clock {0.0};
        bool m_isFirstVideoPackage {true};
        qreal m_videoPts {0.0};
        qreal m_lastVideoDuration {0.0};
        qreal m_videoDiff {0.0};
        QMutex m_mutex;
        qint64 m_id {0};
        int m_index {0};
        bool m_initialized {false};

        explicit VideoEncoderVpxElementPrivate(VideoEncoderVpxElement *self);
        ~VideoEncoderVpxElementPrivate();
        bool init();
        void uninit();
        static void printError(vpx_codec_err_t error,
                               vpx_codec_ctx_t *codecContext=nullptr);
        void sendFrame(const void *data,
                       size_t dataSize,
                       qint64 pts,
                       qint64 dts,
                       quint64 duration,
                       AkCompressedVideoPacket::VideoPacketTypeFlag flags) const;
        int vp9Level(int width, int height, const AkFrac &fps) const;
};

VideoEncoderVpxElement::VideoEncoderVpxElement():
    AkVideoEncoder()
{
    this->d = new VideoEncoderVpxElementPrivate(this);
}

VideoEncoderVpxElement::~VideoEncoderVpxElement()
{
    this->d->uninit();
    delete this->d;
}

AkVideoEncoderCodecID VideoEncoderVpxElement::codec() const
{
#ifdef USE_VP8_INTERFACE
    return AkCompressedVideoCaps::VideoCodecID_vp8;
#else
    return AkCompressedVideoCaps::VideoCodecID_vp9;
#endif
}

VideoEncoderVpxElement::ErrorResilientFlag VideoEncoderVpxElement::errorResilient() const
{
    return this->d->m_errorResilient;
}

int VideoEncoderVpxElement::deadline() const
{
    return this->d->m_deadline;
}

int VideoEncoderVpxElement::speed() const
{
    return this->d->m_speed;
}

bool VideoEncoderVpxElement::lossless() const
{
    return this->d->m_lossless;
}

VideoEncoderVpxElement::TuneContent VideoEncoderVpxElement::tuneContent() const
{
    return this->d->m_tuneContent;
}

QString VideoEncoderVpxElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/VideoEncoderVpx/share/qml/main.qml");
}

void VideoEncoderVpxElement::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("VideoEncoderVpx", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket VideoEncoderVpxElement::iVideoStream(const AkVideoPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_initialized)
        return {};

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    for (int plane = 0; plane < src.planes(); ++plane) {
        auto planeData = this->d->m_frame.planes[plane];
        auto oLineSize = this->d->m_frame.stride[plane];
        auto lineSize = qMin<size_t>(src.lineSize(plane), oLineSize);
        auto heightDiv = src.heightDiv(plane);

        for (int y = 0; y < src.caps().height(); ++y) {
            auto ys = y >> heightDiv;
            memcpy(planeData + ys * oLineSize,
                   src.constLine(plane, y),
                   lineSize);
        }
    }

    qreal pts = src.pts() * src.timeBase().value();
    this->d->m_videoPts = pts + this->d->m_videoDiff;
    this->d->m_lastVideoDuration = src.duration() * src.timeBase().value();

    if (this->d->m_isFirstVideoPackage) {
        this->d->m_videoDiff = this->d->m_clock - pts;
        this->d->m_videoPts = this->d->m_clock;
        this->d->m_isFirstVideoPackage = false;
    } else {
        if (this->d->m_videoPts <= this->d->m_clock) {
            this->d->m_clock += this->d->m_lastVideoDuration;
            this->d->m_videoDiff = this->d->m_clock - pts;
        } else {
            this->d->m_clock = this->d->m_videoPts;
        }
    }

    vpx_codec_pts_t vpxPts =
            qRound64(qreal(this->d->m_clock * this->d->m_encoder.config.enc->g_timebase.den)
                     / this->d->m_encoder.config.enc->g_timebase.num);
    unsigned long duration =
            qRound64(qreal(src.duration()
                           * src.timeBase().num()
                           * this->d->m_encoder.config.enc->g_timebase.den)
                     / (src.timeBase().den()
                        * this->d->m_encoder.config.enc->g_timebase.num));
    auto result = vpx_codec_encode(&this->d->m_encoder,
                                   &this->d->m_frame,
                                   vpxPts,
                                   duration,
                                   0,
                                   this->d->m_deadline);

    if (result != VPX_CODEC_OK)
        this->d->printError(result, &this->d->m_encoder);

    this->d->m_id = src.id();
    this->d->m_index = src.index();
    vpx_codec_iter_t iter = nullptr;

    for (;;) {
        auto vpxPacket = vpx_codec_get_cx_data(&this->d->m_encoder, &iter);

        if (!vpxPacket)
            break;

        if (vpxPacket->kind != VPX_CODEC_CX_FRAME_PKT)
            continue;

        AkCompressedVideoPacket::VideoPacketTypeFlag flags =
                vpxPacket->data.frame.flags & VPX_FRAME_IS_KEY?
                    AkCompressedVideoPacket::VideoPacketTypeFlag_KeyFrame:
                    AkCompressedVideoPacket::VideoPacketTypeFlag_None;
        this->d->sendFrame(vpxPacket->data.frame.buf,
                           vpxPacket->data.frame.sz,
                           vpxPacket->data.frame.pts,
                           vpxPacket->data.frame.pts,
                           vpxPacket->data.frame.duration,
                           flags);
    }

    return {};
}

void VideoEncoderVpxElement::setErrorResilient(ErrorResilientFlag errorResilient)
{
    if (errorResilient == this->d->m_errorResilient)
        return;

    this->d->m_errorResilient = errorResilient;
    emit this->errorResilientChanged(errorResilient);
}

void VideoEncoderVpxElement::setDeadline(int deadline)
{
    if (deadline == this->d->m_deadline)
        return;

    this->d->m_deadline = deadline;
    emit this->deadlineChanged(deadline);
}

void VideoEncoderVpxElement::setSpeed(int speed)
{
    if (speed == this->d->m_speed)
        return;

    this->d->m_speed = speed;
    emit this->speedChanged(speed);
}

void VideoEncoderVpxElement::setLossless(bool lossless)
{
    if (lossless == this->d->m_lossless)
        return;

    this->d->m_lossless = lossless;
    emit this->losslessChanged(lossless);
}

void VideoEncoderVpxElement::setTuneContent(TuneContent tuneContent)
{
    if (tuneContent == this->d->m_tuneContent)
        return;

    this->d->m_tuneContent = tuneContent;
    emit this->tuneContentChanged(tuneContent);
}

void VideoEncoderVpxElement::resetErrorResilient()
{
    this->setErrorResilient(VideoEncoderVpxElement::ErrorResilientFlag_NoFlags);
}

void VideoEncoderVpxElement::resetDeadline()
{
    this->setDeadline(VideoEncoderVpxElement::Deadline_Realtime);
}

void VideoEncoderVpxElement::resetSpeed()
{
    this->setSpeed(16);
}

void VideoEncoderVpxElement::resetLossless()
{
    this->setLossless(false);
}

void VideoEncoderVpxElement::resetTuneContent()
{
    this->setTuneContent(TuneContent_Default);
}

void VideoEncoderVpxElement::resetOptions()
{
    AkVideoEncoder::resetOptions();
    this->resetErrorResilient();
    this->resetDeadline();
    this->resetSpeed();
    this->resetLossless();
    this->resetTuneContent();
}

bool VideoEncoderVpxElement::setState(ElementState state)
{
    auto curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            if (!this->d->init())
                return false;

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
#ifdef USE_VP8_INTERFACE
    this->m_interface = vpx_codec_vp8_cx();
#else
    this->m_interface = vpx_codec_vp9_cx();
#endif
}

VideoEncoderVpxElementPrivate::~VideoEncoderVpxElementPrivate()
{

}

bool VideoEncoderVpxElementPrivate::init()
{
    this->uninit();

    if (!this->m_interface) {
        qCritical() << "VPX Codec interface was not initialized.";

        return false;
    }

    auto inputCaps = self->inputCaps();

    if (!inputCaps) {
        qCritical() << "Invalid input format.";

        return false;
    }

    auto eqFormat = VpxPixFormatTable::byPixFormat(inputCaps.format());
    auto profile = eqFormat->profile;
    auto vpxFormat = eqFormat->vpxFormat;
    auto vpxDepth = eqFormat->depth;
    auto vpxFlags = eqFormat->flags;

    if (vpxFormat == VPX_IMG_FMT_NONE) {
        eqFormat = VpxPixFormatTable::byPixFormat(AkVideoCaps::Format_yuv420p);
        profile = eqFormat->profile;
        vpxFormat = eqFormat->vpxFormat;
        vpxDepth = eqFormat->depth;
        vpxFlags = eqFormat->flags;
    }

    auto pixFormat = VpxPixFormatTable::byVpxFormat(vpxFormat, vpxDepth)->pixFormat;
    auto fps = inputCaps.fps();

    if (!fps)
        fps = {30, 1};

    vpx_codec_enc_cfg_t codecConfigs;
    memset(&codecConfigs, 0, sizeof(vpx_codec_enc_cfg));
    auto result = vpx_codec_enc_config_default(this->m_interface, &codecConfigs, 0);

    if (result != VPX_CODEC_OK) {
        printError(result);

        return false;
    }

    auto gop = self->gop() * fps.num() / (1000 * fps.den());

    if (gop < 1)
        gop = 1;

    codecConfigs.g_profile = profile;
    codecConfigs.g_w = inputCaps.width();
    codecConfigs.g_h = inputCaps.height();
    codecConfigs.g_timebase.num = fps.den();
    codecConfigs.g_timebase.den = fps.num();
    codecConfigs.g_threads = QThread::idealThreadCount();
    codecConfigs.rc_end_usage = VPX_CBR;
    codecConfigs.rc_target_bitrate = self->bitrate() / 1000;
    codecConfigs.g_bit_depth = vpx_bit_depth(vpxDepth);
    codecConfigs.g_input_bit_depth = vpxDepth;
    codecConfigs.g_error_resilient = this->m_errorResilient;
    codecConfigs.g_pass = VPX_RC_ONE_PASS;
    codecConfigs.kf_max_dist = gop;

    memset(&this->m_encoder, 0, sizeof(vpx_codec_ctx));
    result = vpx_codec_enc_init(&this->m_encoder,
                                this->m_interface,
                                &codecConfigs,
                                vpxFlags);

    if (result != VPX_CODEC_OK) {
        printError(result, &this->m_encoder);

        return false;
    }

#ifdef USE_VP8_INTERFACE
    int speed = qBound(0, this->m_speed, 16);
#else
    int speed = qBound(0, 9 * this->m_speed / 16, 9);
#endif

    vpx_codec_control(&this->m_encoder, VP8E_SET_CPUUSED, speed);

#ifdef USE_VP8_INTERFACE
    unsigned int screenContentMode =
            this->m_tuneContent == VideoEncoderVpxElement::TuneContent_Screen;
    vpx_codec_control(&this->m_encoder,
                      VP8E_SET_SCREEN_CONTENT_MODE,
                      screenContentMode);
#else
    auto level = this->vp9Level(inputCaps.width(),
                                inputCaps.height(),
                                fps);
    vpx_codec_control(&this->m_encoder,
                      VP9E_SET_TARGET_LEVEL,
                      static_cast<unsigned int>(level));
    vpx_codec_control(&this->m_encoder,
                      VP9E_SET_LOSSLESS,
                      static_cast<unsigned int>(this->m_lossless));

    int tune = VP9E_CONTENT_DEFAULT;

    switch (this->m_tuneContent) {
    case VideoEncoderVpxElement::TuneContent_Screen:
        tune = VP9E_CONTENT_SCREEN;
        break;
    case VideoEncoderVpxElement::TuneContent_Film:
        tune = VP9E_CONTENT_FILM;
        break;
    default:
        break;
    }

    vpx_codec_control(&this->m_encoder, VP9E_SET_TUNE_CONTENT, tune);
#endif

    memset(&this->m_frame, 0, sizeof(vpx_image_t));

    if (!vpx_img_alloc(&this->m_frame,
                       vpxFormat,
                       inputCaps.width(),
                       inputCaps.height(),
                       1)) {
        qCritical() << "Failed to allocate the input frame";
        vpx_codec_destroy(&this->m_encoder);

        return false;
    }

    inputCaps.setFormat(pixFormat);
    inputCaps.setFps(fps);
    this->m_videoConverter.setAspectRatioMode(AkVideoConverter::AspectRatioMode_Fit);
    this->m_videoConverter.setOutputCaps(inputCaps);

#ifdef USE_VP8_INTERFACE
    AkCompressedVideoCaps::VideoCodecID codecID = AkCompressedVideoCaps::VideoCodecID_vp8;
#else
    AkCompressedVideoCaps::VideoCodecID codecID = AkCompressedVideoCaps::VideoCodecID_vp9;
#endif

    this->m_outputCaps = {codecID,
                          inputCaps.width(),
                          inputCaps.height(),
                          fps};

    this->m_clock = 0.0;
    this->m_isFirstVideoPackage = true;
    this->m_videoPts = 0.0;
    this->m_lastVideoDuration = 0.0;
    this->m_videoDiff = 0.0;

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

        AkCompressedVideoPacket::VideoPacketTypeFlag flags =
                packet->data.frame.flags & VPX_FRAME_IS_KEY?
                    AkCompressedVideoPacket::VideoPacketTypeFlag_KeyFrame:
                    AkCompressedVideoPacket::VideoPacketTypeFlag_None;
        this->sendFrame(packet->data.frame.buf,
                        packet->data.frame.sz,
                        packet->data.frame.pts,
                        packet->data.frame.pts,
                        packet->data.frame.duration,
                        flags);
    }

    vpx_img_free(&this->m_frame);
    vpx_codec_destroy(&this->m_encoder);
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

void VideoEncoderVpxElementPrivate::sendFrame(const void *data,
                                              size_t dataSize,
                                              qint64 pts,
                                              qint64 dts,
                                              quint64 duration,
                                              AkCompressedVideoPacket::VideoPacketTypeFlag flags) const
{
    AkCompressedVideoPacket packet(this->m_outputCaps,
                                   dataSize);
    memcpy(packet.data(), data, dataSize);
    packet.setFlags(flags);
    packet.setPts(pts);
    packet.setDts(dts);
    packet.setDuration(duration);
    packet.setTimeBase({this->m_encoder.config.enc->g_timebase.num,
                        this->m_encoder.config.enc->g_timebase.den});
    packet.setId(this->m_id);
    packet.setIndex(this->m_index);

    emit self->oStream(packet);
}

int VideoEncoderVpxElementPrivate::vp9Level(int width,
                                            int height,
                                            const AkFrac &fps) const
{
    // https://www.webmproject.org/vp9/levels

    struct Vp9LevelsDef
    {
        int level;
        quint64 lumaSampleRate;
        quint64 maxLumaPictureSize;
        int maxBitrate;
        int maxDimension;
    };
    static const Vp9LevelsDef vp9Levels[] = {
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

    quint64 lumaPictureSize = width * height;
    quint64 lumaSampleRate = qRound64(lumaPictureSize * fps.value());
    int bitrate = self->bitrate();
    int dimension = qMax(width, height);

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
