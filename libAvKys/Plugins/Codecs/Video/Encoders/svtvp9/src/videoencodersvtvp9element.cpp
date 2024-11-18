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
#include <EbSvtVp9Enc.h>
#include <EbSvtVp9ErrorCodes.h>

#include "videoencodersvtvp9element.h"

#define RATE_CONTROL_MODE_CQP 0
#define RATE_CONTROL_MODE_VBR 1
#define RATE_CONTROL_MODE_CBR 2

class VideoEncoderSvtVp9ElementPrivate
{
    public:
        VideoEncoderSvtVp9Element *self;
        AkVideoConverter m_videoConverter;
        AkCompressedVideoCaps m_outputCaps;
        int m_speed {9};
        VideoEncoderSvtVp9Element::TuneContent m_tuneContent {VideoEncoderSvtVp9Element::TuneContent_SQ};
        EbComponentType *m_encoder {nullptr};
        EbSvtEncInput m_buffer;
        EbBufferHeaderType m_frame;
        qreal m_clock {0.0};
        bool m_isFirstVideoPackage {true};
        qreal m_videoPts {0.0};
        qreal m_lastVideoDuration {0.0};
        qreal m_videoDiff {0.0};
        QMutex m_mutex;
        qint64 m_id {0};
        int m_index {0};
        bool m_initialized {false};

        explicit VideoEncoderSvtVp9ElementPrivate(VideoEncoderSvtVp9Element *self);
        ~VideoEncoderSvtVp9ElementPrivate();
        bool init();
        void uninit();
        static const char *errortToStr(EbErrorType error);
        void sendFrame(const EbBufferHeaderType *buffer) const;
        int vp9Level(int width, int height, const AkFrac &fps) const;
};

VideoEncoderSvtVp9Element::VideoEncoderSvtVp9Element():
    AkVideoEncoder()
{
    this->d = new VideoEncoderSvtVp9ElementPrivate(this);
}

VideoEncoderSvtVp9Element::~VideoEncoderSvtVp9Element()
{
    this->d->uninit();
    delete this->d;
}

AkVideoEncoderCodecID VideoEncoderSvtVp9Element::codec() const
{
    return AkCompressedVideoCaps::VideoCodecID_vp9;
}

int VideoEncoderSvtVp9Element::speed() const
{
    return this->d->m_speed;
}

VideoEncoderSvtVp9Element::TuneContent VideoEncoderSvtVp9Element::tuneContent() const
{
    return this->d->m_tuneContent;
}

QString VideoEncoderSvtVp9Element::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/VideoEncoderSvtVp9/share/qml/main.qml");
}

void VideoEncoderSvtVp9Element::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("VideoEncoderSvtVp9", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket VideoEncoderSvtVp9Element::iVideoStream(const AkVideoPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_initialized)
        return {};

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    this->d->m_buffer.luma = src.plane(0);
    this->d->m_buffer.cb = src.plane(1);
    this->d->m_buffer.cr = src.plane(2);
    this->d->m_buffer.y_stride  = src.lineSize(0);
    this->d->m_buffer.cb_stride = src.lineSize(1);
    this->d->m_buffer.cr_stride = src.lineSize(2);
    this->d->m_frame.n_filled_len = src.size();

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

    this->d->m_frame.pts = pts;
    auto result =
            eb_vp9_svt_enc_send_picture(this->d->m_encoder, &this->d->m_frame);

    if (result != EB_ErrorNone)
        qCritical() << "Error sending the frame: "
                    << VideoEncoderSvtVp9ElementPrivate::errortToStr(result);

    this->d->m_id = src.id();
    this->d->m_index = src.index();

    for (;;) {
        EbBufferHeaderType *packet = nullptr;
        result = eb_vp9_svt_get_packet(this->d->m_encoder, &packet, 0);

        if (result != EB_ErrorNone)
            break;

        this->d->sendFrame(packet);
        eb_vp9_svt_release_out_buffer(&packet);
    }

    return {};
}

void VideoEncoderSvtVp9Element::setSpeed(int speed)
{
    if (speed == this->d->m_speed)
        return;

    this->d->m_speed = speed;
    emit this->speedChanged(speed);
}

void VideoEncoderSvtVp9Element::setTuneContent(TuneContent tuneContent)
{
    if (tuneContent == this->d->m_tuneContent)
        return;

    this->d->m_tuneContent = tuneContent;
    emit this->tuneContentChanged(tuneContent);
}

void VideoEncoderSvtVp9Element::resetSpeed()
{
    this->setSpeed(9);
}

void VideoEncoderSvtVp9Element::resetTuneContent()
{
    this->setTuneContent(TuneContent_SQ);
}

void VideoEncoderSvtVp9Element::resetOptions()
{
    AkVideoEncoder::resetOptions();
    this->resetSpeed();
    this->resetTuneContent();
}

bool VideoEncoderSvtVp9Element::setState(ElementState state)
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

VideoEncoderSvtVp9ElementPrivate::VideoEncoderSvtVp9ElementPrivate(VideoEncoderSvtVp9Element *self):
    self(self)
{
}

VideoEncoderSvtVp9ElementPrivate::~VideoEncoderSvtVp9ElementPrivate()
{

}

bool VideoEncoderSvtVp9ElementPrivate::init()
{
    this->uninit();

    auto inputCaps = self->inputCaps();

    if (!inputCaps) {
        qCritical() << "Invalid input format.";

        return false;
    }

    auto fps = inputCaps.fps();

    if (!fps)
        fps = {30, 1};

    EbSvtVp9EncConfiguration configs;
    auto result = eb_vp9_svt_init_handle(&this->m_encoder,
                                         this,
                                         &configs);

    if (result != EB_ErrorNone) {
        qCritical() << "Error initializing the encoder: " << errortToStr(result);

        return false;
    }

    configs.enc_mode = qBound(0, this->m_speed, 9);
    configs.tune = uint8_t(this->m_tuneContent);
    configs.target_socket = -1;
    configs.logical_processors = QThread::idealThreadCount();
    configs.rate_control_mode = RATE_CONTROL_MODE_CBR;
    configs.target_bit_rate = self->bitrate();
    configs.profile = 0;
    int gop = qMax(self->gop() * fps.num() / (1000 * fps.den()), 1);
    configs.intra_period = gop;

#if SVT_CHECK_VERSION(1, 1, 0)
    configs.force_key_frames = gop == 1;
#endif

    configs.source_width = inputCaps.width();
    configs.source_height = inputCaps.height();
    configs.frame_rate_numerator = fps.num();
    configs.frame_rate_denominator = fps.den();
    configs.encoder_bit_depth = 8;
    configs.asm_type = 0; // Required otherwise it crash
    configs.level = this->vp9Level(inputCaps.width(),
                                   inputCaps.height(),
                                   fps);
    result = eb_vp9_svt_enc_set_parameter(this->m_encoder, &configs);

    if (result != EB_ErrorNone) {
        qCritical() << "Error initializing the encoder: " << errortToStr(result);
        eb_vp9_deinit_handle(this->m_encoder);

        return false;
    }

    result = eb_vp9_init_encoder(this->m_encoder);

    if (result != EB_ErrorNone) {
        qCritical() << "Error initializing the encoder: " << errortToStr(result);
        eb_vp9_deinit_handle(this->m_encoder);

        return false;
    }

    inputCaps.setFormat(AkVideoCaps::Format_yuv420p);
    inputCaps.setFps(fps);

    memset(&this->m_buffer, 0, sizeof(EbSvtEncInput));
    memset(&this->m_frame, 0, sizeof(EbBufferHeaderType));
    this->m_frame.size = sizeof(EbBufferHeaderType);
    this->m_frame.p_buffer = reinterpret_cast<uint8_t *>(&this->m_buffer);
    this->m_frame.pic_type = gop == 1? EB_IDR_PICTURE: EB_INVALID_PICTURE;

    this->m_videoConverter.setAspectRatioMode(AkVideoConverter::AspectRatioMode_Fit);
    this->m_videoConverter.setOutputCaps(inputCaps);
    this->m_outputCaps = {AkCompressedVideoCaps::VideoCodecID_vp9,
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

void VideoEncoderSvtVp9ElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;

    EbBufferHeaderType eos;
    memset(&eos, 0, sizeof(EbBufferHeaderType));
    eos.pic_type = EB_INVALID_PICTURE;
    eos.flags    = EB_BUFFERFLAG_EOS;
    auto result = eb_vp9_svt_enc_send_picture(this->m_encoder, &eos);

    if (result != EB_ErrorNone)
        qCritical() << "Error sending EOS:" << errortToStr(result);

    for (;;) {
        EbBufferHeaderType *packet = nullptr;
        result = eb_vp9_svt_get_packet(this->m_encoder, &packet, 1);

        if (result != EB_ErrorNone)
            break;

        bool isEos = packet->flags & EB_BUFFERFLAG_EOS;
        this->sendFrame(packet);
        eb_vp9_svt_release_out_buffer(&packet);

        if (isEos)
            break;
    }

    eb_vp9_deinit_encoder(this->m_encoder);
    eb_vp9_deinit_handle(this->m_encoder);
}

const char *VideoEncoderSvtVp9ElementPrivate::errortToStr(EbErrorType error)
{
    struct ErrorToStr
    {
            EbErrorType key;
            const char *value;
    };

    static const ErrorToStr errorToStrTbl[] = {
        {EB_ErrorInsufficientResources , "Insufficient Resources"  },
        {EB_ErrorUndefined             , "Undefined"               },
        {EB_ErrorComponentNotFound     , "Component not found"     },
        {EB_ErrorInvalidComponent      , "Invalid component"       },
        {EB_ErrorBadParameter          , "Bad parameter"           },
        {EB_ErrorNotImplemented        , "Not implemented"         },
        {EB_ErrorCreateThreadFailed    , "Create thread failed"    },
        {EB_ErrorThreadUnresponsive    , "Thread Unresponsive"     },
        {EB_ErrorDestroyThreadFailed   , "Destroy thread failed"   },
        {EB_ErrorNullThread            , "Null thread"             },
        {EB_ErrorCreateSemaphoreFailed , "Create semaphore failed" },
        {EB_ErrorSemaphoreUnresponsive , "Semaphore unresponsive"  },
        {EB_ErrorDestroySemaphoreFailed, "Destroy semaphore failed"},
        {EB_ErrorCreateMutexFailed     , "Create mutex failed"     },
        {EB_ErrorMutexUnresponsive     , "Mutex unresponsive"      },
        {EB_ErrorDestroyMutexFailed    , "Destroy mutex failed"    },
        {EB_NoErrorEmptyQueue          , "Empty queue"             },
        {EB_ErrorMax                   , "Max"                     },
        {EB_ErrorNone                  , "No error"                }
    };

    for (int i = 0; errorToStrTbl[i].key != EB_ErrorNone; i++)
        if (error == errorToStrTbl[i].key)
            return errorToStrTbl[i].value;

    return "No error";
}

void VideoEncoderSvtVp9ElementPrivate::sendFrame(const EbBufferHeaderType *buffer) const
{
    AkCompressedVideoPacket packet(this->m_outputCaps,
                                   buffer->n_filled_len);
    memcpy(packet.data(), buffer->p_buffer, packet.size());
    packet.setFlags(buffer->pic_type == EB_IDR_PICTURE?
                        AkCompressedVideoPacket::VideoPacketTypeFlag_KeyFrame:
                        AkCompressedVideoPacket::VideoPacketTypeFlag_None);
    packet.setPts(buffer->pts);
    packet.setDts(buffer->dts);
    packet.setDuration(1);
    packet.setTimeBase(this->m_outputCaps.fps().invert());
    packet.setId(this->m_id);
    packet.setIndex(this->m_index);

    emit self->oStream(packet);
}

int VideoEncoderSvtVp9ElementPrivate::vp9Level(int width,
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
        {10, 829440    , 36864   , 200   , 512  },
        {11, 2764800   , 73728   , 800   , 768  },
        {20, 4608000   , 122880  , 1800  , 960  },
        {21, 9216000   , 245760  , 3600  , 1344 },
        {30, 20736000  , 552960  , 7200  , 2048 },
        {31, 36864000  , 983040  , 12000 , 2752 },
        {40, 83558400  , 2228224 , 18000 , 4160 },
        {41, 160432128 , 2228224 , 30000 , 4160 },
        {50, 311951360 , 8912896 , 60000 , 8384 },
        {51, 588251136 , 8912896 , 120000, 8384 },
        {52, 1176502272, 8912896 , 180000, 8384 },
        {60, 1176502272, 35651584, 180000, 16832},
        {61, 2353004544, 35651584, 240000, 16832},
        {62, 4706009088, 35651584, 480000, 16832},
        {0 , 0         , 0       , 0     , 0    }
    };

    quint64 lumaPictureSize = width * height;
    quint64 lumaSampleRate = qRound64(lumaPictureSize * fps.value());
    int bitrate = self->bitrate();
    int dimension = qMax(width, height);

    for (auto vp9Level = vp9Levels; vp9Level->level; ++vp9Level)
        if (vp9Level->lumaSampleRate >= lumaSampleRate
            && vp9Level->maxLumaPictureSize >= lumaPictureSize
            && 1000 * vp9Level->maxBitrate >= bitrate
            && vp9Level->maxDimension >= dimension) {
            return vp9Level->level;
        }

    return 0;
}

#include "moc_videoencodersvtvp9element.cpp"
