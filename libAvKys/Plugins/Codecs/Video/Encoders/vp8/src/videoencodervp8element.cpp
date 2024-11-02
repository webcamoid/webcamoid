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

#include "videoencodervp8element.h"

struct VpxPixFormatTable
{
    AkVideoCaps::PixelFormat pixFormat;
    vpx_img_fmt_t vpxFormat;
    size_t depth;

    static inline const VpxPixFormatTable *table()
    {
        static const VpxPixFormatTable vpxPixFormatTable[] = {
            {AkVideoCaps::Format_yvu420p  , VPX_IMG_FMT_YV12  , 8 },
            {AkVideoCaps::Format_yuv420p  , VPX_IMG_FMT_I420  , 8 },
            {AkVideoCaps::Format_yuv422p  , VPX_IMG_FMT_I422  , 8 },
            {AkVideoCaps::Format_yuv444p  , VPX_IMG_FMT_I444  , 8 },
            {AkVideoCaps::Format_yuv440p  , VPX_IMG_FMT_I440  , 8 },
            {AkVideoCaps::Format_nv12     , VPX_IMG_FMT_NV12  , 8 },
            {AkVideoCaps::Format_yuv420p10, VPX_IMG_FMT_I42016, 10},
            {AkVideoCaps::Format_yuv422p10, VPX_IMG_FMT_I42216, 10},
            {AkVideoCaps::Format_yuv444p10, VPX_IMG_FMT_I44416, 10},
            {AkVideoCaps::Format_yuv440p10, VPX_IMG_FMT_I44016, 10},
            {AkVideoCaps::Format_yuv420p12, VPX_IMG_FMT_I42016, 12},
            {AkVideoCaps::Format_yuv422p12, VPX_IMG_FMT_I42216, 12},
            {AkVideoCaps::Format_yuv444p12, VPX_IMG_FMT_I44416, 12},
            {AkVideoCaps::Format_yuv440p12, VPX_IMG_FMT_I44016, 12},
            {AkVideoCaps::Format_none     , VPX_IMG_FMT_NONE  , 0 },
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

class VideoEncoderVp8ElementPrivate
{
    public:
        VideoEncoderVp8Element *self;
        AkVideoConverter m_videoConverter;
        AkCompressedVideoCaps m_outputCaps;
        VideoEncoderVp8Element::ErrorResilientFlag m_errorResilient {VideoEncoderVp8Element::ErrorResilientFlag_NoFlags};
        int m_deadline {VideoEncoderVp8Element::Deadline_Realtime};
        vpx_codec_iface_t *m_interface {nullptr};
        vpx_codec_ctx_t m_encoder;
        vpx_image_t m_frame;
        QMutex m_mutex;
        qint64 m_id {0};
        int m_index {0};
        bool m_initialized {false};

        explicit VideoEncoderVp8ElementPrivate(VideoEncoderVp8Element *self);
        ~VideoEncoderVp8ElementPrivate();
        bool init();
        void uninit();
        static void printError(vpx_codec_err_t error,
                               const vpx_codec_ctx_t *codecContext=nullptr);
        void sendFrame(const void *data,
                       size_t dataSize,
                       qint64 pts,
                       qint64 dts,
                       AkCompressedVideoPacket::VideoPacketTypeFlag flags) const;
};

VideoEncoderVp8Element::VideoEncoderVp8Element():
    AkVideoEncoder()
{
    this->d = new VideoEncoderVp8ElementPrivate(this);
}

VideoEncoderVp8Element::~VideoEncoderVp8Element()
{
    this->d->uninit();
    delete this->d;
}

AkVideoEncoderCodecID VideoEncoderVp8Element::codec() const
{
    return AkCompressedVideoCaps::VideoCodecID_vp8;
}

VideoEncoderVp8Element::ErrorResilientFlag VideoEncoderVp8Element::errorResilient() const
{
    return this->d->m_errorResilient;
}

int VideoEncoderVp8Element::deadline() const
{
    return this->d->m_deadline;
}

QString VideoEncoderVp8Element::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/VideoEncoderVp8/share/qml/main.qml");
}

void VideoEncoderVp8Element::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("VideoEncoderVp8", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket VideoEncoderVp8Element::iVideoStream(const AkVideoPacket &packet)
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
        auto lineSize = qMin<size_t>(packet.lineSize(plane), oLineSize);
        auto heightDiv = packet.heightDiv(plane);

        for (int y = 0; y < src.caps().height(); ++y) {
            auto ys = y >> heightDiv;
            memcpy(planeData + ys * oLineSize,
                   src.constLine(plane, y),
                   lineSize);
        }
    }

    vpx_codec_pts_t pts =
            qRound64(qreal(src.pts()
                           * src.timeBase().num()
                           * this->d->m_encoder.config.enc->g_timebase.den)
                     / (src.timeBase().den()
                        * this->d->m_encoder.config.enc->g_timebase.num));
    unsigned long duration =
            qRound64(qreal(src.timeBase().num()
                           * this->d->m_encoder.config.enc->g_timebase.den)
                     / (src.timeBase().den()
                        * this->d->m_encoder.config.enc->g_timebase.num));
    auto result = vpx_codec_encode(&this->d->m_encoder,
                                   &this->d->m_frame,
                                   pts,
                                   duration,
                                   0,
                                   this->d->m_deadline);

    if (result != VPX_CODEC_OK)
        this->d->printError(result, &this->d->m_encoder);

    this->d->m_id = src.id();
    this->d->m_index = src.index();
    vpx_codec_iter_t iter = nullptr;

    for (;;) {
        auto packet = vpx_codec_get_cx_data(&this->d->m_encoder, &iter);

        if (!packet)
            break;

        if (packet->kind != VPX_CODEC_CX_FRAME_PKT)
            continue;

        AkCompressedVideoPacket::VideoPacketTypeFlag flags =
                packet->data.frame.flags & VPX_FRAME_IS_KEY?
                    AkCompressedVideoPacket::VideoPacketTypeFlag_KeyFrame:
                    AkCompressedVideoPacket::VideoPacketTypeFlag_None;
        this->d->sendFrame(packet->data.frame.buf,
                           packet->data.frame.sz,
                           packet->data.frame.pts,
                           packet->data.frame.pts,
                           flags);
    }

    return {};
}

void VideoEncoderVp8Element::setErrorResilient(ErrorResilientFlag errorResilient)
{
    if (errorResilient == this->d->m_errorResilient)
        return;

    this->d->m_errorResilient = errorResilient;
    emit this->errorResilientChanged(errorResilient);
}

void VideoEncoderVp8Element::setDeadline(int deadline)
{
    if (deadline == this->d->m_deadline)
        return;

    this->d->m_deadline = deadline;
    emit this->deadlineChanged(deadline);
}

void VideoEncoderVp8Element::resetErrorResilient()
{
    this->setErrorResilient(VideoEncoderVp8Element::ErrorResilientFlag_NoFlags);
}

void VideoEncoderVp8Element::resetDeadline()
{
    this->setDeadline(VideoEncoderVp8Element::Deadline_Realtime);
}

void VideoEncoderVp8Element::resetOptions()
{
    AkVideoEncoder::resetOptions();
    this->resetErrorResilient();
    this->resetDeadline();
}

bool VideoEncoderVp8Element::setState(ElementState state)
{
    AkElement::ElementState curState = this->state();

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

VideoEncoderVp8ElementPrivate::VideoEncoderVp8ElementPrivate(VideoEncoderVp8Element *self):
    self(self)
{
    this->m_interface = vpx_codec_vp8_cx();
}

VideoEncoderVp8ElementPrivate::~VideoEncoderVp8ElementPrivate()
{

}

bool VideoEncoderVp8ElementPrivate::init()
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
    auto vpxFormat = eqFormat->vpxFormat;
    auto vpxDepth = eqFormat->depth;

    if (vpxFormat == VPX_IMG_FMT_NONE) {
        eqFormat = VpxPixFormatTable::byPixFormat(AkVideoCaps::Format_yuv420p);
        vpxFormat = eqFormat->vpxFormat;
        vpxDepth = eqFormat->depth;
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

    codecConfigs.g_w = inputCaps.width();
    codecConfigs.g_h = inputCaps.height();
    codecConfigs.g_threads = QThread::idealThreadCount();
    codecConfigs.g_timebase.num = fps.den();
    codecConfigs.g_timebase.den = fps.num();
    codecConfigs.rc_end_usage = VPX_CBR;
    codecConfigs.rc_target_bitrate = self->bitrate() / 1000;
    codecConfigs.g_input_bit_depth = vpxDepth;
    codecConfigs.g_error_resilient = this->m_errorResilient;
    codecConfigs.g_pass = VPX_RC_ONE_PASS;
    codecConfigs.kf_max_dist = gop;

    memset(&this->m_encoder, 0, sizeof(vpx_codec_ctx));
    vpx_codec_flags_t flags = 0;

    switch (vpxFormat) {
        case VPX_IMG_FMT_I42016:
        case VPX_IMG_FMT_I42216:
        case VPX_IMG_FMT_I44416:
        case VPX_IMG_FMT_I44016:
            flags |= VPX_CODEC_USE_HIGHBITDEPTH;

            break;

        default:
            break;
    }

    result = vpx_codec_enc_init(&this->m_encoder,
                                this->m_interface,
                                &codecConfigs,
                                flags);

    if (result != VPX_CODEC_OK) {
        printError(result, &this->m_encoder);

        return false;
    }

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
    this->m_videoConverter.setOutputCaps(inputCaps);
    this->m_outputCaps = {AkCompressedVideoCaps::VideoCodecID_vp8,
                          inputCaps.width(),
                          inputCaps.height(),
                          fps};
    this->m_initialized = true;

    return true;
}

void VideoEncoderVp8ElementPrivate::uninit()
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
                        flags);
    }

    vpx_img_free(&this->m_frame);
    vpx_codec_destroy(&this->m_encoder);
}

void VideoEncoderVp8ElementPrivate::printError(vpx_codec_err_t error,
                                               const vpx_codec_ctx_t *codecContext)
{
    if (codecContext)
        qCritical() << vpx_codec_error_detail(codecContext);
    else
        qCritical() << vpx_codec_err_to_string(error);
}

void VideoEncoderVp8ElementPrivate::sendFrame(const void *data,
                                              size_t dataSize,
                                              qint64 pts,
                                              qint64 dts,
                                              AkCompressedVideoPacket::VideoPacketTypeFlag flags) const
{
    AkCompressedVideoPacket packet(this->m_outputCaps,
                                   dataSize);
    memcpy(packet.data(), data, dataSize);
    packet.setFlags(flags);
    packet.setPts(pts);
    packet.setDts(dts);
    packet.setTimeBase({this->m_encoder.config.enc->g_timebase.num,
                        this->m_encoder.config.enc->g_timebase.den});
    packet.setId(this->m_id);
    packet.setIndex(this->m_index);

    emit self->oStream(packet);
}

#include "moc_videoencodervp8element.cpp"
