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
#include <x264.h>

#include "videoencoderx264element.h"

struct X264PixFormatTable
{
    AkVideoCaps::PixelFormat pixFormat;
    int x264Format;
    size_t depth;
    int flags;
    const char *profile;

    static inline const X264PixFormatTable *table()
    {
        static const X264PixFormatTable x264X264PixFormatTable[] = {
            {AkVideoCaps::Format_y8       , X264_CSP_I400, 8 , 0                  , "high"    },
            {AkVideoCaps::Format_yuv420p  , X264_CSP_I420, 8 , 0                  , "baseline"},
            {AkVideoCaps::Format_yvu420p  , X264_CSP_YV12, 8 , 0                  , "baseline"},
            {AkVideoCaps::Format_nv12     , X264_CSP_NV12, 8 , 0                  , "baseline"},
            {AkVideoCaps::Format_nv21     , X264_CSP_NV21, 8 , 0                  , "baseline"},
            {AkVideoCaps::Format_yuv422p  , X264_CSP_I422, 8 , 0                  , "high422" },
            {AkVideoCaps::Format_yvu422p  , X264_CSP_YV16, 8 , 0                  , "high422" },
            {AkVideoCaps::Format_nv16     , X264_CSP_NV16, 8 , 0                  , "high422" },
            {AkVideoCaps::Format_yuyv422  , X264_CSP_YUYV, 8 , 0                  , "high422" },
            {AkVideoCaps::Format_uyvy422  , X264_CSP_UYVY, 8 , 0                  , "high422" },
            {AkVideoCaps::Format_v210     , X264_CSP_V210, 8 , 0                  , "high422" },
            {AkVideoCaps::Format_yuv444p  , X264_CSP_I444, 8 , 0                  , "high444" },
            {AkVideoCaps::Format_yvu444p  , X264_CSP_YV24, 8 , 0                  , "high444" },
            {AkVideoCaps::Format_bgr24    , X264_CSP_BGR , 8 , 0                  , "high444" },
            {AkVideoCaps::Format_bgra     , X264_CSP_BGRA, 8 , 0                  , "high444" },
            {AkVideoCaps::Format_rgb24    , X264_CSP_RGB , 8 , 0                  , "high444" },
            {AkVideoCaps::Format_y10      , X264_CSP_I400, 10, X264_CSP_HIGH_DEPTH, "high"    },
            {AkVideoCaps::Format_y12      , X264_CSP_I400, 12, X264_CSP_HIGH_DEPTH, "high"    },
            {AkVideoCaps::Format_y14      , X264_CSP_I400, 14, X264_CSP_HIGH_DEPTH, "high"    },
            {AkVideoCaps::Format_yuv420p10, X264_CSP_I420, 10, X264_CSP_HIGH_DEPTH, "high10"  },
            {AkVideoCaps::Format_yuv422p10, X264_CSP_I422, 10, X264_CSP_HIGH_DEPTH, "high422" },
            {AkVideoCaps::Format_yuv444p10, X264_CSP_I444, 10, X264_CSP_HIGH_DEPTH, "high444" },
            {AkVideoCaps::Format_yuv420p12, X264_CSP_I420, 12, X264_CSP_HIGH_DEPTH, "high444" },
            {AkVideoCaps::Format_yuv422p12, X264_CSP_I422, 12, X264_CSP_HIGH_DEPTH, "high444" },
            {AkVideoCaps::Format_yuv444p12, X264_CSP_I444, 12, X264_CSP_HIGH_DEPTH, "high444" },
            {AkVideoCaps::Format_yuv420p14, X264_CSP_I420, 14, X264_CSP_HIGH_DEPTH, "high444" },
            {AkVideoCaps::Format_yuv422p14, X264_CSP_I422, 14, X264_CSP_HIGH_DEPTH, "high444" },
            {AkVideoCaps::Format_yuv444p14, X264_CSP_I444, 14, X264_CSP_HIGH_DEPTH, "high444" },
            {AkVideoCaps::Format_bgra64   , X264_CSP_BGRA, 16, X264_CSP_HIGH_DEPTH, "high444" },
            {AkVideoCaps::Format_bgr48    , X264_CSP_BGR , 16, X264_CSP_HIGH_DEPTH, "high444" },
            {AkVideoCaps::Format_none     , X264_CSP_NONE, 0 , 0                  , ""        },
        };

        return x264X264PixFormatTable;
    }

    static inline const X264PixFormatTable *byPixFormat(AkVideoCaps::PixelFormat format)
    {
        auto fmt = table();

        for (; fmt->pixFormat != AkVideoCaps::Format_none; fmt++)
            if (fmt->pixFormat == format)
                return fmt;

        return fmt;
    }

    static inline const X264PixFormatTable *byX264Format(int format,
                                                         size_t depth)
    {
        auto fmt = table();

        for (; fmt->pixFormat != AkVideoCaps::Format_none; fmt++)
            if (fmt->x264Format == format
                && fmt->depth == depth)
                return fmt;

        return fmt;
    }
};

class VideoEncoderX264ElementPrivate
{
    public:
        VideoEncoderX264Element *self;
        AkVideoConverter m_videoConverter;
        AkCompressedVideoCaps m_outputCaps;
        VideoEncoderX264Element::Preset m_preset {VideoEncoderX264Element::Preset_UltraFast};
        VideoEncoderX264Element::TuneContent m_tuneContent {VideoEncoderX264Element::TuneContent_ZeroLatency};
        VideoEncoderX264Element::LogLevel m_logLevel {VideoEncoderX264Element::LogLevel_Info};
        AkCompressedVideoPackets m_headers;
        x264_t *m_encoder {nullptr};
        x264_picture_t m_frame;
        x264_picture_t m_frameOut;
        qreal m_clock {0.0};
        qint64 m_iclock {0};
        bool m_isFirstVideoPackage {true};
        qreal m_lastVideoDuration {0.0};
        qreal m_videoDiff {0.0};
        qreal m_fillDiff {0.0};
        QMutex m_mutex;
        qint64 m_id {0};
        int m_index {0};
        qint64 m_prevPts {-1};
        bool m_initialized {false};
        AkVideoPacket m_prevPackage;

        explicit VideoEncoderX264ElementPrivate(VideoEncoderX264Element *self);
        ~VideoEncoderX264ElementPrivate();
        bool init();
        void uninit();
        void updateHeaders();
        void sendFrame(const x264_nal_t *nal, int writtenSize) const;
        unsigned int x264Level(int width, int height, const AkFrac &fps) const;
        const char *presetStr(VideoEncoderX264Element::Preset preset) const;
        const char *tuneStr(VideoEncoderX264Element::TuneContent tune) const;
        int x264LogLevel(VideoEncoderX264Element::LogLevel logLevel) const;
};

VideoEncoderX264Element::VideoEncoderX264Element():
    AkVideoEncoder()
{
    this->d = new VideoEncoderX264ElementPrivate(this);
}

VideoEncoderX264Element::~VideoEncoderX264Element()
{
    this->d->uninit();
    delete this->d;
}

AkVideoEncoderCodecID VideoEncoderX264Element::codec() const
{
    return AkCompressedVideoCaps::VideoCodecID_avc;
}

AkCompressedPackets VideoEncoderX264Element::headers() const
{
    AkCompressedPackets packets;

    for (auto &header: this->d->m_headers)
        packets << header;

    return packets;
}

VideoEncoderX264Element::Preset VideoEncoderX264Element::preset() const
{
    return this->d->m_preset;
}

VideoEncoderX264Element::TuneContent VideoEncoderX264Element::tuneContent() const
{
    return this->d->m_tuneContent;
}

VideoEncoderX264Element::LogLevel VideoEncoderX264Element::logLevel() const
{
    return this->d->m_logLevel;
}

QString VideoEncoderX264Element::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/VideoEncoderX264/share/qml/main.qml");
}

void VideoEncoderX264Element::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("VideoEncoderX264", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket VideoEncoderX264Element::iVideoStream(const AkVideoPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_initialized)
        return {};

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    if (!this->d->m_isFirstVideoPackage
        && this->d->m_id == src.id()
        && src.pts() == this->d->m_prevPts
        && !this->fillGaps()) {
        return {};
    }

    qreal pts = src.pts() * src.timeBase().value();
    quint64 fill = 0;

    if (this->fillGaps()) {
        if (!this->d->m_isFirstVideoPackage && this->d->m_id == src.id()) {
            qreal fillr = (src.pts() - this->d->m_prevPts)
                          * src.timeBase().value()
                          * this->d->m_outputCaps.fps().value()
                          + this->d->m_fillDiff
                          - 1.0;
            fill = qRound64(qMax(fillr, 0.0)) * 2;
            this->d->m_fillDiff = fillr - fill;
        }

        this->d->m_isFirstVideoPackage = false;
    } else {
        if (this->d->m_isFirstVideoPackage) {
            this->d->m_videoDiff = -pts;
            this->d->m_isFirstVideoPackage = false;
        } else {
            if (this->d->m_id == src.id()) {
                this->d->m_clock = pts + this->d->m_videoDiff;
            } else {
                this->d->m_clock += this->d->m_lastVideoDuration;
                this->d->m_videoDiff = this->d->m_clock - pts;
            }
        }

        this->d->m_lastVideoDuration = src.duration() * src.timeBase().value();
    }

    this->d->m_id = src.id();
    this->d->m_index = src.index();
    this->d->m_prevPts = src.pts();

    if (fill > 0) {
        auto &prev = this->d->m_prevPackage;

        for (int plane = 0; plane < prev.planes(); ++plane) {
            auto planeData = this->d->m_frame.img.plane[plane];
            auto oLineSize = this->d->m_frame.img.i_stride[plane];
            auto lineSize = qMin<size_t>(prev.lineSize(plane), oLineSize);
            auto heightDiv = prev.heightDiv(plane);

            for (int y = 0; y < prev.caps().height(); ++y) {
                auto ys = y >> heightDiv;
                memcpy(planeData + ys * oLineSize,
                       prev.constLine(plane, y),
                       lineSize);
            }
        }

        for (quint64 i = 0; i < fill; ++i) {
            x264_nal_t *nal = nullptr;
            int inal = 0;
            this->d->m_frame.i_pts = this->d->m_iclock;
            auto writtenSize = x264_encoder_encode(this->d->m_encoder,
                                                   &nal,
                                                   &inal,
                                                   &this->d->m_frame,
                                                   &this->d->m_frameOut);

            if (writtenSize > 0)
                this->d->sendFrame(nal, writtenSize);
            else if (writtenSize < 0)
                qCritical() << "Failed to encode frame";

            ++this->d->m_iclock;
        }
    }

    for (int plane = 0; plane < src.planes(); ++plane) {
        auto planeData = this->d->m_frame.img.plane[plane];
        auto oLineSize = this->d->m_frame.img.i_stride[plane];
        auto lineSize = qMin<size_t>(src.lineSize(plane), oLineSize);
        auto heightDiv = src.heightDiv(plane);

        for (int y = 0; y < src.caps().height(); ++y) {
            auto ys = y >> heightDiv;
            memcpy(planeData + ys * oLineSize,
                   src.constLine(plane, y),
                   lineSize);
        }
    }

    x264_nal_t *nal = nullptr;
    int inal = 0;
    this->d->m_frame.i_pts =
            this->fillGaps()?
                this->d->m_iclock:
                qRound64(this->d->m_clock * this->d->m_outputCaps.fps().value());
    auto writtenSize = x264_encoder_encode(this->d->m_encoder,
                                           &nal,
                                           &inal,
                                           &this->d->m_frame,
                                           &this->d->m_frameOut);

    if (writtenSize < 1) {
        if (writtenSize < 0)
            qCritical() << "Failed to encode frame";
        else if (this->fillGaps())
            ++this->d->m_iclock;

        return {};
    }

    this->d->sendFrame(nal, writtenSize);

    if (this->fillGaps()) {
        ++this->d->m_iclock;
        this->d->m_prevPackage = src;
    }

    return {};
}

void VideoEncoderX264Element::setPreset(Preset preset)
{
    if (preset == this->d->m_preset)
        return;

    this->d->m_preset = preset;
    emit this->presetChanged(preset);
}

void VideoEncoderX264Element::setTuneContent(TuneContent tuneContent)
{
    if (tuneContent == this->d->m_tuneContent)
        return;

    this->d->m_tuneContent = tuneContent;
    emit this->tuneContentChanged(tuneContent);
}

void VideoEncoderX264Element::setLogLevel(LogLevel logLevel)
{
    if (logLevel == this->d->m_logLevel)
        return;

    this->d->m_logLevel = logLevel;
    emit this->logLevelChanged(logLevel);
}

void VideoEncoderX264Element::resetPreset()
{
    this->setPreset(Preset_UltraFast);
}

void VideoEncoderX264Element::resetTuneContent()
{
    this->setTuneContent(TuneContent_ZeroLatency);
}

void VideoEncoderX264Element::resetLogLevel()
{
    this->setLogLevel(LogLevel_Info);
}

void VideoEncoderX264Element::resetOptions()
{
    AkVideoEncoder::resetOptions();
    this->resetPreset();
    this->resetTuneContent();
}

bool VideoEncoderX264Element::setState(ElementState state)
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

VideoEncoderX264ElementPrivate::VideoEncoderX264ElementPrivate(VideoEncoderX264Element *self):
    self(self)
{
}

VideoEncoderX264ElementPrivate::~VideoEncoderX264ElementPrivate()
{

}

bool VideoEncoderX264ElementPrivate::init()
{
    this->uninit();

    auto inputCaps = self->inputCaps();

    if (!inputCaps) {
        qCritical() << "Invalid input format.";

        return false;
    }

    auto eqFormat = X264PixFormatTable::byPixFormat(inputCaps.format());
    auto profile = eqFormat->profile;
    auto x264Format = eqFormat->x264Format;
    auto x264Depth = eqFormat->depth;
    auto x264Flags = eqFormat->flags;

    if (eqFormat->pixFormat == AkVideoCaps::Format_none) {
        eqFormat = X264PixFormatTable::byPixFormat(AkVideoCaps::Format_yuv420p);
        profile = eqFormat->profile;
        x264Format = eqFormat->x264Format;
        x264Depth = eqFormat->depth;
        x264Flags = eqFormat->flags;
    }

    auto pixFormat =
            X264PixFormatTable::byX264Format(x264Format,
                                             x264Depth)->pixFormat;
    auto fps = inputCaps.fps();

    if (!fps)
        fps = {30, 1};

    x264_param_t params;
    memset(&params, 0, sizeof(x264_param_t));

    if (x264_param_default_preset(&params,
                                  this->presetStr(this->m_preset),
                                  this->tuneStr(this->m_tuneContent)) < 0) {
        qCritical() << "Can't read the default preset";

        return false;
    }

    params.i_bitdepth = x264Depth;
    params.i_csp = x264Format;
    params.i_width = inputCaps.width();
    params.i_height = inputCaps.height();
    params.b_vfr_input = 1;
    params.b_repeat_headers = 0;
    params.b_annexb = 0;
    params.i_threads = QThread::idealThreadCount();
    params.i_fps_num = inputCaps.fps().num();
    params.i_fps_den = inputCaps.fps().den();
    params.i_timebase_num = inputCaps.fps().den();
    params.i_timebase_den = inputCaps.fps().num();
    params.i_keyint_max = qMax(self->gop() * fps.num() / (1000 * fps.den()), 1);
    params.rc.i_rc_method = X264_RC_ABR;
    params.rc.i_bitrate = self->bitrate() / 1000;
    params.i_log_level = this->x264LogLevel(this->m_logLevel);
    params.i_level_idc = this->x264Level(inputCaps.width(),
                                         inputCaps.height(),
                                         fps);

    if (x264_param_apply_profile(&params, profile) < 0) {
        qCritical() << "Can't apply profile";

        return false;
    }

    this->m_encoder = x264_encoder_open(&params);

    if (!this->m_encoder) {
        qCritical() << "Can't apply profile";

        return false;
    }

    memset(&this->m_frame, 0, sizeof(x264_picture_t));

    if (x264_picture_alloc(&this->m_frame,
                           params.i_csp,
                           params.i_width,
                           params.i_height) < 0) {
        qCritical() << "Can't allocate the frame";
        x264_encoder_close(this->m_encoder);

        return false;
    }

    memset(&this->m_frameOut, 0, sizeof(x264_picture_t));

    inputCaps.setFormat(pixFormat);
    inputCaps.setFps(fps);
    this->m_videoConverter.setAspectRatioMode(AkVideoConverter::AspectRatioMode_Fit);
    this->m_videoConverter.setOutputCaps(inputCaps);
    this->m_outputCaps = {self->codec(),
                          inputCaps.width(),
                          inputCaps.height(),
                          fps};
    this->updateHeaders();

    this->m_clock = 0.0;
    this->m_iclock = 0;
    this->m_isFirstVideoPackage = true;
    this->m_lastVideoDuration = 0.0;
    this->m_videoDiff = 0.0;
    this->m_fillDiff = 0.0;
    this->m_id = -1;
    this->m_prevPts = -1;

    this->m_initialized = true;

    return true;
}

void VideoEncoderX264ElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;

    if (this->m_encoder) {
        for (;;) {
            x264_nal_t *nal = nullptr;
            int inal = 0;
            auto writtenSize = x264_encoder_encode(this->m_encoder,
                                                   &nal,
                                                   &inal,
                                                   nullptr,
                                                   &this->m_frameOut);

            if (writtenSize < 1) {
                if (writtenSize < 0)
                    qCritical() << "Failed to encode frame";

                break;
            }

            this->sendFrame(nal, writtenSize);
        }

        x264_encoder_close(this->m_encoder);
        this->m_encoder = nullptr;
    }

    x264_picture_clean(&this->m_frame);
}

void VideoEncoderX264ElementPrivate::updateHeaders()
{
    x264_nal_t *x264Headers = nullptr;
    int inal = 0;

    if (x264_encoder_headers(this->m_encoder, &x264Headers, &inal) < 1)
        return;

    QByteArray extraData;
    QDataStream ds(&extraData, QIODeviceBase::WriteOnly);
    ds << quint64(inal);

    for (int i = 0; i < inal; i++) {
        ds << quint64(x264Headers[i].i_payload);
        ds.writeRawData(reinterpret_cast<char *>(x264Headers[i].p_payload), x264Headers[i].i_payload);
    }

    AkCompressedVideoPacket headerPacket(this->m_outputCaps,
                                         extraData.size());
    memcpy(headerPacket.data(),
           extraData.constData(),
           headerPacket.size());
    headerPacket.setTimeBase(this->m_outputCaps.fps().invert());
    headerPacket.setFlags(AkCompressedVideoPacket::VideoPacketTypeFlag_Header);
    this->m_headers = {headerPacket};
    emit self->headersChanged(self->headers());
}

void VideoEncoderX264ElementPrivate::sendFrame(const x264_nal_t *nal,
                                               int writtenSize) const
{
    AkCompressedVideoPacket packet(this->m_outputCaps, writtenSize);
    memcpy(packet.data(), nal->p_payload, packet.size());
    packet.setFlags(this->m_frameOut.b_keyframe?
                        AkCompressedVideoPacket::VideoPacketTypeFlag_KeyFrame:
                        AkCompressedVideoPacket::VideoPacketTypeFlag_None);
    packet.setPts(this->m_frameOut.i_pts);
    packet.setDts(this->m_frameOut.i_dts);
    packet.setDuration(1);
    packet.setTimeBase(this->m_outputCaps.fps().invert());
    packet.setId(this->m_id);
    packet.setIndex(this->m_index);

    emit self->oStream(packet);
}

unsigned int VideoEncoderX264ElementPrivate::x264Level(int width,
                                                       int height,
                                                       const AkFrac &fps) const
{
    int mbWidth = (width + 15) / 16;
    int mbHeight = (height + 15) / 16;
    quint64 lumaPictureSize = mbWidth * mbHeight;
    quint64 lumaSampleRate = qRound64(lumaPictureSize * fps.value());
    int bitrate = self->bitrate();

    for (auto level = x264_levels; level->level_idc; ++level)
        if (level->frame_size >= lumaPictureSize
            && level->mbps >= lumaSampleRate
            && 1000 * level->bitrate >= bitrate) {
            return level->level_idc;
        }

    return 0;
}

const char *VideoEncoderX264ElementPrivate::presetStr(VideoEncoderX264Element::Preset preset) const
{
    struct PresetString
    {
        VideoEncoderX264Element::Preset preset;
        const char *presetStr;
    };

    static const PresetString x264PresetTable[] = {
        {VideoEncoderX264Element::Preset_UltraFast, "ultrafast"},
        {VideoEncoderX264Element::Preset_SuperFast, "superfast"},
        {VideoEncoderX264Element::Preset_VeryFast , "veryfast" },
        {VideoEncoderX264Element::Preset_Faster   , "faster"   },
        {VideoEncoderX264Element::Preset_Fast     , "fast"     },
        {VideoEncoderX264Element::Preset_Medium   , "medium"   },
        {VideoEncoderX264Element::Preset_Slow     , "slow"     },
        {VideoEncoderX264Element::Preset_Slower   , "slower"   },
        {VideoEncoderX264Element::Preset_VerySlow , "veryslow" },
        {VideoEncoderX264Element::Preset_Placebo  , "placebo"  },
        {VideoEncoderX264Element::Preset_Unknown  , ""         },
    };

    auto pr = x264PresetTable;

    for (;
         pr->preset != VideoEncoderX264Element::Preset_Unknown;
         ++pr)
        if (pr->preset == preset)
            return pr->presetStr;

    return pr->presetStr;
}

const char *VideoEncoderX264ElementPrivate::tuneStr(VideoEncoderX264Element::TuneContent tune) const
{
    struct TuneString
    {
        VideoEncoderX264Element::TuneContent tune;
        const char *tuneStr;
    };

    static const TuneString x264TuneTable[] = {
        {VideoEncoderX264Element::TuneContent_Film       , "film"       },
        {VideoEncoderX264Element::TuneContent_Animation  , "animation"  },
        {VideoEncoderX264Element::TuneContent_Grain      , "grain"      },
        {VideoEncoderX264Element::TuneContent_StillImage , "stillimage" },
        {VideoEncoderX264Element::TuneContent_PSNR       , "psnr"       },
        {VideoEncoderX264Element::TuneContent_SSIM       , "ssim"       },
        {VideoEncoderX264Element::TuneContent_FastDecode , "fastdecode" },
        {VideoEncoderX264Element::TuneContent_ZeroLatency, "zerolatency"},
        {VideoEncoderX264Element::TuneContent_Unknown    , ""           },
    };

    auto tun = x264TuneTable;

    for (;
         tun->tune != VideoEncoderX264Element::TuneContent_Unknown;
         ++tun)
        if (tun->tune == tune)
            return tun->tuneStr;

    return tun->tuneStr;
}

int VideoEncoderX264ElementPrivate::x264LogLevel(VideoEncoderX264Element::LogLevel logLevel) const
{
    struct LogLevelValue
    {
        VideoEncoderX264Element::LogLevel level;
        int x264Level;
    };

    static const LogLevelValue x264LogLevelTable[] = {
        {VideoEncoderX264Element::LogLevel_Error  , X264_LOG_ERROR  },
        {VideoEncoderX264Element::LogLevel_Warning, X264_LOG_WARNING},
        {VideoEncoderX264Element::LogLevel_Info   , X264_LOG_INFO   },
        {VideoEncoderX264Element::LogLevel_Debug  , X264_LOG_DEBUG  },
        {VideoEncoderX264Element::LogLevel_None   , X264_LOG_NONE   },
    };

    auto level = x264LogLevelTable;

    for (;
         level->level != VideoEncoderX264Element::LogLevel_None;
         ++level)
        if (level->level == logLevel)
            return level->x264Level;

    return level->x264Level;
}

#include "moc_videoencoderx264element.cpp"
