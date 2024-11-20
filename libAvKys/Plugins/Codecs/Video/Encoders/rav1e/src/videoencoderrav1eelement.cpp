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
#include <QTime>
#include <QVariant>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akcompressedvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>
#include <akcompressedvideopacket.h>
#include <rav1e.h>

#include "videoencoderrav1eelement.h"

struct Av1PixFormatTable
{
    AkVideoCaps::PixelFormat pixFormat;
    RaChromaSampling av1Format;
    size_t depth;

    static inline const Av1PixFormatTable *table()
    {
        static const Av1PixFormatTable rav1ePixFormatTable[] = {
            {AkVideoCaps::Format_y8       , RA_CHROMA_SAMPLING_CS400, 8 },
            {AkVideoCaps::Format_y10      , RA_CHROMA_SAMPLING_CS400, 10},
            {AkVideoCaps::Format_y12      , RA_CHROMA_SAMPLING_CS400, 12},
            {AkVideoCaps::Format_yuv420p  , RA_CHROMA_SAMPLING_CS420, 8 },
            {AkVideoCaps::Format_yuv420p10, RA_CHROMA_SAMPLING_CS420, 10},
            {AkVideoCaps::Format_yuv420p12, RA_CHROMA_SAMPLING_CS420, 12},
            {AkVideoCaps::Format_yuv422p  , RA_CHROMA_SAMPLING_CS422, 8 },
            {AkVideoCaps::Format_yuv422p10, RA_CHROMA_SAMPLING_CS422, 10},
            {AkVideoCaps::Format_yuv422p12, RA_CHROMA_SAMPLING_CS422, 12},
            {AkVideoCaps::Format_yuv444p  , RA_CHROMA_SAMPLING_CS444, 8 },
            {AkVideoCaps::Format_yuv444p10, RA_CHROMA_SAMPLING_CS444, 10},
            {AkVideoCaps::Format_yuv444p12, RA_CHROMA_SAMPLING_CS444, 12},
            {AkVideoCaps::Format_none     , RaChromaSampling(-1)    , 0 },
        };

        return rav1ePixFormatTable;
    }

    static inline const Av1PixFormatTable *byPixFormat(AkVideoCaps::PixelFormat format)
    {
        auto fmt = table();

        for (; fmt->pixFormat != AkVideoCaps::Format_none; fmt++)
            if (fmt->pixFormat == format)
                return fmt;

        return fmt;
    }

    static inline const Av1PixFormatTable *byAv1Format(RaChromaSampling format,
                                                       size_t depth)
    {
        auto fmt = table();

        for (; fmt->pixFormat != AkVideoCaps::Format_none; fmt++)
            if (fmt->av1Format == format
                && fmt->depth == depth)
                return fmt;

        return fmt;
    }
};

class VideoEncoderRav1eElementPrivate
{
    public:
        VideoEncoderRav1eElement *self;
        AkVideoConverter m_videoConverter;
        AkCompressedVideoCaps m_outputCaps;
        int m_speed {11};
        bool m_lowLatency {true};
        VideoEncoderRav1eElement::TuneContent m_tuneContent {VideoEncoderRav1eElement::TuneContent_Psychovisual};
        AkCompressedVideoPackets m_headers;
        RaContext *m_encoder {nullptr};
        QMutex m_mutex;
        qint64 m_id {0};
        int m_index {0};
        bool m_initialized {false};

        explicit VideoEncoderRav1eElementPrivate(VideoEncoderRav1eElement *self);
        ~VideoEncoderRav1eElementPrivate();
        bool init();
        void uninit();
        void updateHeaders();
        void sendFrame(const RaPacket *av1Packet) const;
};

VideoEncoderRav1eElement::VideoEncoderRav1eElement():
    AkVideoEncoder()
{
    this->d = new VideoEncoderRav1eElementPrivate(this);
}

VideoEncoderRav1eElement::~VideoEncoderRav1eElement()
{
    this->d->uninit();
    delete this->d;
}

AkVideoEncoderCodecID VideoEncoderRav1eElement::codec() const
{
    return AkCompressedVideoCaps::VideoCodecID_av1;
}

AkCompressedPackets VideoEncoderRav1eElement::headers() const
{
    AkCompressedPackets packets;

    for (auto &header: this->d->m_headers)
        packets << header;

    return packets;
}

int VideoEncoderRav1eElement::speed() const
{
    return this->d->m_speed;
}

bool VideoEncoderRav1eElement::lowLatency() const
{
    return this->d->m_lowLatency;
}

VideoEncoderRav1eElement::TuneContent VideoEncoderRav1eElement::tuneContent() const
{
    return this->d->m_tuneContent;
}

QString VideoEncoderRav1eElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/VideoEncoderRav1e/share/qml/main.qml");
}

void VideoEncoderRav1eElement::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("VideoEncoderRav1e", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket VideoEncoderRav1eElement::iVideoStream(const AkVideoPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_initialized)
        return {};

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    auto frame = rav1e_frame_new(this->d->m_encoder);

    if (!frame) {
        qCritical() << "Could not allocate rav1e frame";

        return {};
    }

    auto specs = AkVideoCaps::formatSpecs(src.caps().format());
    for (size_t plane = 0; plane < src.planes(); ++plane) {
        rav1e_frame_fill_plane(frame,
                               plane,
                               src.constPlane(plane),
                               src.planeSize(plane),
                               src.lineSize(plane),
                               specs.plane(plane).component(0).byteDepth());
    }

    bool send = true;

    while (send) {
        send = false;
        auto result = rav1e_send_frame(this->d->m_encoder, frame);

        if (result != RA_ENCODER_STATUS_SUCCESS) {
            if (result == RA_ENCODER_STATUS_ENOUGH_DATA) {
                send = true;
            } else {
                qCritical() << "Failed sending frame: "
                            << rav1e_status_to_str(result);
                rav1e_frame_unref(frame);

                return {};
            }
        }

        RaPacket *packet = nullptr;

        for (;;) {
            auto result = rav1e_receive_packet(this->d->m_encoder, &packet);

            if (result != RA_ENCODER_STATUS_SUCCESS) {
                if (result != RA_ENCODER_STATUS_ENCODED) {
                    if (result != RA_ENCODER_STATUS_NEED_MORE_DATA)
                        qCritical() << "Failed receive frame: "
                                    << rav1e_status_to_str(result);

                    break;
                }

                continue;
            }

            this->d->sendFrame(packet);
            rav1e_packet_unref(packet);
            packet = nullptr;
        }
    }

    rav1e_frame_unref(frame);

    return {};
}

void VideoEncoderRav1eElement::setSpeed(int speed)
{
    if (speed == this->d->m_speed)
        return;

    this->d->m_speed = speed;
    emit this->speedChanged(speed);
}

void VideoEncoderRav1eElement::setLowLatency(bool lowLatency)
{
    if (lowLatency == this->d->m_lowLatency)
        return;

    this->d->m_lowLatency = lowLatency;
    emit this->lowLatencyChanged(lowLatency);
}

void VideoEncoderRav1eElement::setTuneContent(TuneContent tuneContent)
{
    if (tuneContent == this->d->m_tuneContent)
        return;

    this->d->m_tuneContent = tuneContent;
    emit this->tuneContentChanged(tuneContent);
}

void VideoEncoderRav1eElement::resetSpeed()
{
    this->setSpeed(11);
}

void VideoEncoderRav1eElement::resetLowLatency()
{
    this->setLowLatency(true);
}

void VideoEncoderRav1eElement::resetTuneContent()
{
    this->setTuneContent(TuneContent_Psychovisual);
}

void VideoEncoderRav1eElement::resetOptions()
{
    AkVideoEncoder::resetOptions();
    this->resetSpeed();
    this->resetLowLatency();
    this->resetTuneContent();
}

bool VideoEncoderRav1eElement::setState(ElementState state)
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

VideoEncoderRav1eElementPrivate::VideoEncoderRav1eElementPrivate(VideoEncoderRav1eElement *self):
    self(self)
{
}

VideoEncoderRav1eElementPrivate::~VideoEncoderRav1eElementPrivate()
{

}

bool VideoEncoderRav1eElementPrivate::init()
{
    this->uninit();

    auto inputCaps = self->inputCaps();

    if (!inputCaps) {
        qCritical() << "Invalid input format.";

        return false;
    }

    auto eqFormat = Av1PixFormatTable::byPixFormat(inputCaps.format());
    auto av1Format = eqFormat->av1Format;
    auto av1Depth = eqFormat->depth;

    if (eqFormat->pixFormat == AkVideoCaps::Format_none) {
        eqFormat = Av1PixFormatTable::byPixFormat(AkVideoCaps::Format_yuv420p);
        av1Format = eqFormat->av1Format;
        av1Depth = eqFormat->depth;
    }

    auto pixFormat =
            Av1PixFormatTable::byAv1Format(av1Format,
                                           av1Depth)->pixFormat;
    auto fps = inputCaps.fps();

    if (!fps)
        fps = {30, 1};

    auto config = rav1e_config_default();

    if (!config) {
        qCritical() << "Could not allocate rav1e config";

        return false;
    }

    rav1e_config_set_time_base(config, RaRational {uint64_t(fps.den()),
                                                   uint64_t(fps.num())});

    if (rav1e_config_parse_int(config, "width", inputCaps.width()) < 0) {
        qCritical() << "Invalid width passed to rav1e";
        rav1e_config_unref(config);

        return false;
    }

    if (rav1e_config_parse_int(config, "height", inputCaps.height()) < 0) {
        qCritical() << "Invalid height passed to rav1e";
        rav1e_config_unref(config);

        return false;
    }

    if (rav1e_config_parse_int(config,
                               "threads",
                               QThread::idealThreadCount()) < 0)
        qCritical() << "Invalid number of threads, defaulting to auto";

    if (rav1e_config_parse(config,
                           "tune",
                           this->m_tuneContent == VideoEncoderRav1eElement::TuneContent_PSNR?
                                "pnsr":
                                "psychovisual") < 0)
        qCritical() << "Error setting the tunning parameter";

    int speed = qBound(0, this->m_speed, 10);

    if (rav1e_config_parse_int(config, "speed", speed) < 0)
        qCritical() << "Could not set speed preset, defaulting to auto";

    if (rav1e_config_parse_int(config, "low_latency", this->m_lowLatency) < 0)
        qCritical() << "Could not set the low latency mode";

    int gop = qMax(self->gop() * fps.num() / (1000 * fps.den()), 1);

    if (rav1e_config_parse_int(config, "key_frame_interval", gop) < 0) {
        qCritical() << "Could not set GOP";
        rav1e_config_unref(config);

        return false;
    }

    if (rav1e_config_parse_int(config, "bitrate", self->bitrate()) < 0) {
        qCritical() << "Could not set bitrate";
        rav1e_config_unref(config);

        return false;
    }

    if (rav1e_config_set_pixel_format(config,
                                      av1Depth,
                                      av1Format,
                                      RA_CHROMA_SAMPLE_POSITION_UNKNOWN,
                                      RA_PIXEL_RANGE_LIMITED) < 0) {
        qCritical() << "Failed to set pixel format properties";
        rav1e_config_unref(config);

        return false;
    }

    if (rav1e_config_set_color_description(config,
                                           RA_MATRIX_COEFFICIENTS_UNSPECIFIED,
                                           RA_COLOR_PRIMARIES_UNSPECIFIED,
                                           RA_TRANSFER_CHARACTERISTICS_UNSPECIFIED) < 0) {
        qCritical() << "Failed to set color properties";
        rav1e_config_unref(config);

        return false;
    }

    this->m_encoder = rav1e_context_new(config);

    if (!this->m_encoder) {
        qCritical() << "Failed to create rav1e encode context";
        rav1e_config_unref(config);

        return false;
    }

    rav1e_config_unref(config);

    inputCaps.setFormat(pixFormat);
    inputCaps.setFps(fps);
    this->m_videoConverter.setAspectRatioMode(AkVideoConverter::AspectRatioMode_Fit);
    this->m_videoConverter.setOutputCaps(inputCaps);
    this->m_outputCaps = {AkCompressedVideoCaps::VideoCodecID_av1,
                          inputCaps.width(),
                          inputCaps.height(),
                          fps};
    this->updateHeaders();

    this->m_initialized = true;

    return true;
}

void VideoEncoderRav1eElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;

    if (this->m_encoder) {
        RaPacket *packet = nullptr;

        for (;;) {
            auto result = rav1e_receive_packet(this->m_encoder, &packet);

            if (result != RA_ENCODER_STATUS_SUCCESS) {
                if (result != RA_ENCODER_STATUS_ENCODED) {
                    if (result != RA_ENCODER_STATUS_NEED_MORE_DATA)
                        qCritical() << "Failed receive frame: "
                                    << rav1e_status_to_str(result);

                    break;
                }

                continue;
            }

            this->sendFrame(packet);
            rav1e_packet_unref(packet);
            packet = nullptr;
        }
    }

    if (this->m_encoder) {
        rav1e_context_unref(this->m_encoder);
        this->m_encoder = nullptr;
    }
}

void VideoEncoderRav1eElementPrivate::updateHeaders()
{
    auto headers = rav1e_container_sequence_header(this->m_encoder);

    if (!headers)
        return;

    AkCompressedVideoPacket headerPacket(this->m_outputCaps,
                                         headers->len);
    memcpy(headerPacket.data(),
           headers->data,
           headerPacket.size());
    headerPacket.setTimeBase(this->m_outputCaps.fps().invert());
    headerPacket.setFlags(AkCompressedVideoPacket::VideoPacketTypeFlag_Header);
    this->m_headers << headerPacket;
    emit self->headersChanged(self->headers());
    rav1e_data_unref(headers);
}

void VideoEncoderRav1eElementPrivate::sendFrame(const RaPacket *av1Packet) const
{
    AkCompressedVideoPacket packet(this->m_outputCaps,
                                   av1Packet->len);
    memcpy(packet.data(), av1Packet->data, packet.size());
    packet.setFlags(av1Packet->frame_type == RA_FRAME_TYPE_KEY?
                        AkCompressedVideoPacket::VideoPacketTypeFlag_KeyFrame:
                        AkCompressedVideoPacket::VideoPacketTypeFlag_None);

    qint64 pts = QTime::currentTime().msecsSinceStartOfDay()
                 * this->m_outputCaps.fps().num()
                 / (1000 * this->m_outputCaps.fps().den());

    packet.setPts(pts);
    packet.setDts(pts);
    packet.setDuration(1);
    packet.setTimeBase(this->m_outputCaps.fps().invert());
    packet.setId(this->m_id);
    packet.setIndex(this->m_index);

    emit self->oStream(packet);
}

#include "moc_videoencoderrav1eelement.cpp"
