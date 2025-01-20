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
        static const X264PixFormatTable x264PixFormatTable[] = {
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
            {AkVideoCaps::Format_yuv420p10, X264_CSP_I420, 10, X264_CSP_HIGH_DEPTH, "high10"  },
            {AkVideoCaps::Format_yuv422p10, X264_CSP_I422, 10, X264_CSP_HIGH_DEPTH, "high422" },
            {AkVideoCaps::Format_yuv444p10, X264_CSP_I444, 10, X264_CSP_HIGH_DEPTH, "high444" },
            {AkVideoCaps::Format_none     , X264_CSP_NONE, 0 , 0                  , ""        },
        };

        return x264PixFormatTable;
    }

    static inline const X264PixFormatTable *byPixFormat(AkVideoCaps::PixelFormat format)
    {
        auto fmt = table();

        for (; fmt->pixFormat != AkVideoCaps::Format_none; fmt++)
            if (fmt->pixFormat == format)
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
        AkPropertyOptions m_options;
        QByteArray m_headers;
        x264_t *m_encoder {nullptr};
        x264_picture_t m_frame;
        x264_picture_t m_frameOut;
        QMutex m_mutex;
        qint64 m_id {0};
        int m_index {0};
        bool m_initialized {false};
        bool m_paused {false};
        qint64 m_encodedTimePts {0};
        AkElementPtr m_fpsControl {akPluginManager->create<AkElement>("VideoFilter/FpsControl")};

        explicit VideoEncoderX264ElementPrivate(VideoEncoderX264Element *self);
        ~VideoEncoderX264ElementPrivate();
        bool init();
        void uninit();
        void updateHeaders();
        void updateOutputCaps(const AkVideoCaps &inputCaps);
        void encodeFrame(const AkVideoPacket &src);
        void sendFrame(const x264_nal_t *nal, int writtenSize) const;
        unsigned int x264Level(const AkVideoCaps &caps) const;
};

VideoEncoderX264Element::VideoEncoderX264Element():
    AkVideoEncoder()
{
    this->d = new VideoEncoderX264ElementPrivate(this);
    this->setCodec(this->codecs().value(0));
}

VideoEncoderX264Element::~VideoEncoderX264Element()
{
    this->d->uninit();
    delete this->d;
}

QStringList VideoEncoderX264Element::codecs() const
{
    return {"x264"};
}

AkVideoEncoderCodecID VideoEncoderX264Element::codecID(const QString &codec) const
{
    return codec == this->codecs().first()?
                AkCompressedVideoCaps::VideoCodecID_h264:
                AkCompressedVideoCaps::VideoCodecID_unknown;
}

QString VideoEncoderX264Element::codecDescription(const QString &codec) const
{
    return codec == this->codecs().first()?
                QStringLiteral("H264 (libx264)"):
                QString();
}

AkCompressedVideoCaps VideoEncoderX264Element::outputCaps() const
{
    return this->d->m_outputCaps;
}

QByteArray VideoEncoderX264Element::headers() const
{
    return this->d->m_headers;
}

qint64 VideoEncoderX264Element::encodedTimePts() const
{
    return this->d->m_encodedTimePts;
}

AkPropertyOptions VideoEncoderX264Element::options() const
{
    return this->d->m_options;
}

AkPacket VideoEncoderX264Element::iVideoStream(const AkVideoPacket &packet)
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

bool VideoEncoderX264Element::setState(ElementState state)
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

VideoEncoderX264ElementPrivate::VideoEncoderX264ElementPrivate(VideoEncoderX264Element *self):
    self(self)
{
    this->m_options = {
        {"preset" ,
         QObject::tr("Preset"),
         "",
         AkPropertyOption::OptionType_String,
         0.0,
         0.0,
         0.0,
         "ultrafast",
         {{"ultrafast", QObject::tr("Ultra Fast"), "", "ultrafast"},
          {"superfast", QObject::tr("Super Fast"), "", "superfast"},
          {"veryfast" , QObject::tr("Very Fast") , "", "veryfast" },
          {"faster"   , QObject::tr("Faster")    , "", "faster"   },
          {"fast"     , QObject::tr("Fast")      , "", "fast"     },
          {"medium"   , QObject::tr("Medium")    , "", "medium"   },
          {"slow"     , QObject::tr("Slow")      , "", "slow"     },
          {"slower"   , QObject::tr("Slower")    , "", "slower"   },
          {"veryslow" , QObject::tr("Very Slow") , "", "veryslow" },
          {"placebo"  , QObject::tr("Placebo")   , "", "placebo"  }}},
        {"tuneContent" ,
         QObject::tr("Tune content"),
         "",
         AkPropertyOption::OptionType_String,
         0.0,
         0.0,
         0.0,
         "zerolatency",
         {{"film"       , QObject::tr("Film")        , "", "film"       },
          {"animation"  , QObject::tr("Animation")   , "", "animation"  },
          {"grain"      , QObject::tr("Grain")       , "", "grain"      },
          {"stillimage" , QObject::tr("Still image") , "", "stillimage" },
          {"psnr"       , QObject::tr("PSNR")        , "", "psnr"       },
          {"ssim"       , QObject::tr("SSIM")        , "", "ssim"       },
          {"fastdecode" , QObject::tr("Fast decode") , "", "fastdecode" },
          {"zerolatency", QObject::tr("Zero latency"), "", "zerolatency"}}},
        {"logLevel" ,
         QObject::tr("Log level"),
         "",
         AkPropertyOption::OptionType_Number,
         X264_LOG_NONE,
         X264_LOG_DEBUG,
         1.0,
         X264_LOG_INFO,
         {{"none"   , QObject::tr("None")   , "", X264_LOG_NONE   },
          {"error"  , QObject::tr("Error")  , "", X264_LOG_ERROR  },
          {"warning", QObject::tr("Warning"), "", X264_LOG_WARNING},
          {"info"   , QObject::tr("Info")   , "", X264_LOG_INFO   },
          {"debug"  , QObject::tr("Debug")  , "", X264_LOG_DEBUG  }}},
        {"repeatHeaders",
         QObject::tr("Repeat headers"),
         QObject::tr("Enable stand alone stream without a container format"),
         AkPropertyOption::OptionType_Boolean,
         0.0,
         1.0,
         1.0,
         0.0,
         {}},
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

    auto eqFormat =
            X264PixFormatTable::byPixFormat(this->m_videoConverter.outputCaps().format());

    if (eqFormat->pixFormat == AkVideoCaps::Format_none)
        eqFormat = X264PixFormatTable::byPixFormat(AkVideoCaps::Format_yuv420p);

    x264_param_t params;
    memset(&params, 0, sizeof(x264_param_t));

    if (x264_param_default_preset(&params,
                                  self->optionValue("preset").toString().toStdString().c_str(),
                                  self->optionValue("tuneContent").toString().toStdString().c_str()) < 0) {
        qCritical() << "Can't read the default preset";

        return false;
    }

    params.i_bitdepth = eqFormat->depth;
    params.i_csp = eqFormat->x264Format | eqFormat->flags;
    params.i_width = this->m_videoConverter.outputCaps().width();
    params.i_height = this->m_videoConverter.outputCaps().height();
    params.b_vfr_input = 1;
    params.b_repeat_headers = self->optionValue("repeatHeaders").toBool();
    params.b_annexb = 0;
    params.i_threads = QThread::idealThreadCount();
    params.i_fps_num = this->m_videoConverter.outputCaps().fps().num();
    params.i_fps_den = this->m_videoConverter.outputCaps().fps().den();
    params.i_timebase_num = this->m_videoConverter.outputCaps().fps().den();
    params.i_timebase_den = this->m_videoConverter.outputCaps().fps().num();
    params.i_keyint_max =
            qMax(self->gop() * this->m_videoConverter.outputCaps().fps().num()
                 / (1000 * this->m_videoConverter.outputCaps().fps().den()), 1);
    params.rc.i_rc_method = X264_RC_ABR;
    params.rc.i_bitrate = self->bitrate() / 1000;
    params.i_log_level = self->optionValue("logLevel").toInt();
    params.i_level_idc = this->x264Level(this->m_videoConverter.outputCaps());

    if (x264_param_apply_profile(&params, eqFormat->profile) < 0) {
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

    if (this->m_fpsControl)
        QMetaObject::invokeMethod(this->m_fpsControl.data(),
                                  "restart",
                                  Qt::DirectConnection);

    this->m_paused = false;
}

void VideoEncoderX264ElementPrivate::updateHeaders()
{
    x264_nal_t *x264Headers = nullptr;
    int inal = 0;

    if (x264_encoder_headers(this->m_encoder, &x264Headers, &inal) < 1)
        return;

    QByteArray headers;
    QDataStream ds(&headers, QIODeviceBase::WriteOnly);
    ds << quint64(inal);

    for (int i = 0; i < inal; i++) {
        ds << quint64(x264Headers[i].i_payload);
        ds.writeRawData(reinterpret_cast<char *>(x264Headers[i].p_payload),
                        x264Headers[i].i_payload);
    }

    if (this->m_headers == headers)
        return;

    this->m_headers = headers;
    emit self->headersChanged(headers);
}

void VideoEncoderX264ElementPrivate::updateOutputCaps(const AkVideoCaps &inputCaps)
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

    auto eqFormat = X264PixFormatTable::byPixFormat(inputCaps.format());

    if (eqFormat->pixFormat == AkVideoCaps::Format_none)
        eqFormat = X264PixFormatTable::byPixFormat(AkVideoCaps::Format_yuv420p);

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

void VideoEncoderX264ElementPrivate::encodeFrame(const AkVideoPacket &src)
{
    this->m_id = src.id();
    this->m_index = src.index();

    // Write the current frame.
    for (int plane = 0; plane < src.planes(); ++plane) {
        auto planeData = this->m_frame.img.plane[plane];
        auto oLineSize = this->m_frame.img.i_stride[plane];
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
    this->m_frame.i_pts = src.pts();
    auto writtenSize = x264_encoder_encode(this->m_encoder,
                                           &nal,
                                           &inal,
                                           &this->m_frame,
                                           &this->m_frameOut);

    if (writtenSize > 0)
        this->sendFrame(nal, writtenSize);
    else if (writtenSize < 0)
        qCritical() << "Failed to encode frame";

    this->m_encodedTimePts = src.pts() + src.duration();
    emit self->encodedTimePtsChanged(this->m_encodedTimePts);
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
    packet.setTimeBase(this->m_outputCaps.rawCaps().fps().invert());
    packet.setId(this->m_id);
    packet.setIndex(this->m_index);

    emit self->oStream(packet);
}

unsigned int VideoEncoderX264ElementPrivate::x264Level(const AkVideoCaps &caps) const
{
    int mbWidth = (caps.width() + 15) / 16;
    int mbHeight = (caps.height() + 15) / 16;
    quint64 lumaPictureSize = mbWidth * mbHeight;
    quint64 lumaSampleRate = qRound64(lumaPictureSize * caps.fps().value());
    int bitrate = self->bitrate();

    for (auto level = x264_levels; level->level_idc; ++level)
        if (level->frame_size >= lumaPictureSize
            && level->mbps >= lumaSampleRate
            && 1000 * level->bitrate >= bitrate) {
            return level->level_idc;
        }

    return 0;
}

#include "moc_videoencoderx264element.cpp"
