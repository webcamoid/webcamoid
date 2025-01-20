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
#include <EbSvtVp9Enc.h>
#include <EbSvtVp9ErrorCodes.h>

#include "videoencodersvtvp9element.h"

#define RATE_CONTROL_MODE_CQP 0
#define RATE_CONTROL_MODE_VBR 1
#define RATE_CONTROL_MODE_CBR 2

#define TUNE_CONTENT_SQ   0
#define TUNE_CONTENT_OQ   1
#define TUNE_CONTENT_VMAF 2

class VideoEncoderSvtVp9ElementPrivate
{
    public:
        VideoEncoderSvtVp9Element *self;
        AkVideoConverter m_videoConverter;
        AkCompressedVideoCaps m_outputCaps;
        AkPropertyOptions m_options;
        QByteArray m_headers;
        EbComponentType *m_encoder {nullptr};
        EbSvtEncInput m_buffer;
        EbBufferHeaderType m_frame;
        QMutex m_mutex;
        qint64 m_id {0};
        int m_index {0};
        bool m_initialized {false};
        bool m_paused {false};
        qint64 m_encodedTimePts {0};
        AkVideoPacket m_curFrame;
        AkElementPtr m_fpsControl {akPluginManager->create<AkElement>("VideoFilter/FpsControl")};

        explicit VideoEncoderSvtVp9ElementPrivate(VideoEncoderSvtVp9Element *self);
        ~VideoEncoderSvtVp9ElementPrivate();
        bool init();
        void uninit();
        void updateHeaders();
        void updateOutputCaps(const AkVideoCaps &inputCaps);
        static const char *errortToStr(EbErrorType error);
        void encodeFrame(const AkVideoPacket &src);
        void sendFrame(const EbBufferHeaderType *buffer) const;
        int vp9Level(const AkVideoCaps &caps) const;
};

VideoEncoderSvtVp9Element::VideoEncoderSvtVp9Element():
    AkVideoEncoder()
{
    this->d = new VideoEncoderSvtVp9ElementPrivate(this);
    this->setCodec(this->codecs().value(0));
}

VideoEncoderSvtVp9Element::~VideoEncoderSvtVp9Element()
{
    this->d->uninit();
    delete this->d;
}

QStringList VideoEncoderSvtVp9Element::codecs() const
{
    return {"svtvp9"};
}

AkVideoEncoderCodecID VideoEncoderSvtVp9Element::codecID(const QString &codec) const
{
    return codec == this->codecs().first()?
                AkCompressedVideoCaps::VideoCodecID_vp9:
                AkCompressedVideoCaps::VideoCodecID_unknown;
}

QString VideoEncoderSvtVp9Element::codecDescription(const QString &codec) const
{
    return codec == this->codecs().first()?
                QStringLiteral("SVT-VP9"):
                QString();
}

AkCompressedVideoCaps VideoEncoderSvtVp9Element::outputCaps() const
{
    return this->d->m_outputCaps;
}

QByteArray VideoEncoderSvtVp9Element::headers() const
{
    return this->d->m_headers;
}

qint64 VideoEncoderSvtVp9Element::encodedTimePts() const
{
    return this->d->m_encodedTimePts;
}

AkPropertyOptions VideoEncoderSvtVp9Element::options() const
{
    return this->d->m_options;
}

AkPacket VideoEncoderSvtVp9Element::iVideoStream(const AkVideoPacket &packet)
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

bool VideoEncoderSvtVp9Element::setState(ElementState state)
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

VideoEncoderSvtVp9ElementPrivate::VideoEncoderSvtVp9ElementPrivate(VideoEncoderSvtVp9Element *self):
    self(self)
{
    this->m_options = {
        {"speed" ,
         QObject::tr("Speed"),
         QObject::tr("Encoding speed"),
         AkPropertyOption::OptionType_Number,
         0.0,
         9.0,
         1.0,
         9.0,
         {}},
        {"tuneContent" ,
         QObject::tr("Tune content"),
         "",
         AkPropertyOption::OptionType_Number,
         TUNE_CONTENT_SQ,
         TUNE_CONTENT_VMAF,
         1.0,
         TUNE_CONTENT_SQ,
         {{"sq"  , QObject::tr("SQ")  , "", TUNE_CONTENT_SQ  },
          {"oq"  , QObject::tr("OQ")  , "", TUNE_CONTENT_OQ  },
          {"vmaf", QObject::tr("VMAF"), "", TUNE_CONTENT_VMAF}}},
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

    EbSvtVp9EncConfiguration configs;
    auto result = eb_vp9_svt_init_handle(&this->m_encoder,
                                         this,
                                         &configs);

    if (result != EB_ErrorNone) {
        qCritical() << "Error initializing the encoder: " << errortToStr(result);

        return false;
    }

    configs.enc_mode = uint8_t(qBound(0, self->optionValue("speed").toInt(), 9));
    configs.tune = uint8_t(self->optionValue("tuneContent").toInt());
    configs.target_socket = -1;
    configs.logical_processors = QThread::idealThreadCount();
    configs.rate_control_mode = RATE_CONTROL_MODE_CBR;
    configs.target_bit_rate = self->bitrate();
    configs.profile = 0;
    int gop = qMax(self->gop() * this->m_videoConverter.outputCaps().fps().num()
                   / (1000 * this->m_videoConverter.outputCaps().fps().den()), 1);
    configs.intra_period = gop;
    configs.source_width = this->m_videoConverter.outputCaps().width();
    configs.source_height = this->m_videoConverter.outputCaps().height();
    configs.frame_rate_numerator =
            this->m_videoConverter.outputCaps().fps().num();
    configs.frame_rate_denominator =
            this->m_videoConverter.outputCaps().fps().den();
    configs.encoder_bit_depth = 8;
    configs.asm_type = 0; // Required otherwise it crash
    configs.level = this->vp9Level(this->m_videoConverter.outputCaps());
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

    memset(&this->m_buffer, 0, sizeof(EbSvtEncInput));
    memset(&this->m_frame, 0, sizeof(EbBufferHeaderType));
    this->m_frame.size = sizeof(EbBufferHeaderType);
    this->m_frame.p_buffer = reinterpret_cast<uint8_t *>(&this->m_buffer);
    this->m_frame.pic_type = gop == 1? EB_IDR_PICTURE: EB_INVALID_PICTURE;
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

void VideoEncoderSvtVp9ElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;

    if (this->m_encoder) {
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
        this->m_encoder = nullptr;
    }

    if (this->m_fpsControl)
        QMetaObject::invokeMethod(this->m_fpsControl.data(),
                                  "restart",
                                  Qt::DirectConnection);

    this->m_paused = false;
}

void VideoEncoderSvtVp9ElementPrivate::updateHeaders()
{
#if 0
    // VP9 seems to provide stream headers, but crash when enabled.
    // Disabling for now.

    EbBufferHeaderType *svtHeaders = nullptr;

    if (eb_vp9_svt_enc_stream_header(this->m_encoder,
                                     &svtHeaders) != EB_ErrorNone)
        return;

    QByteArray headers(reinterpret_cast<char *>(svtHeaders->p_buffer),
                       svtHeaders->n_filled_len);
    eb_vp9_svt_release_out_buffer(&svtHeaders);

    if (this->m_headers == headers)
        return;

    this->m_headers = headers;
    emit self->headersChanged(headers);
#endif
}

void VideoEncoderSvtVp9ElementPrivate::updateOutputCaps(const AkVideoCaps &inputCaps)
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

    auto fps = inputCaps.fps();

    if (!fps)
        fps = {30, 1};

    this->m_videoConverter.setOutputCaps({AkVideoCaps::Format_yuv420p,
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

const char *VideoEncoderSvtVp9ElementPrivate::errortToStr(EbErrorType error)
{
    struct ErrorToStr
    {
        EbErrorType key;
        const char *value;
    };

    static const ErrorToStr errorToStrTbl[] = {
        {EB_ErrorInsufficientResources , "Insufficient resources"  },
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
        {EB_ErrorNone                  , "No error"                },
    };

    for (int i = 0; errorToStrTbl[i].key != EB_ErrorNone; i++)
        if (error == errorToStrTbl[i].key)
            return errorToStrTbl[i].value;

    return "No error";
}

void VideoEncoderSvtVp9ElementPrivate::encodeFrame(const AkVideoPacket &src)
{
    this->m_id = src.id();
    this->m_index = src.index();

    // Write the current frame.
    this->m_curFrame = src;
    this->m_buffer.luma = this->m_curFrame.plane(0);
    this->m_buffer.cb = this->m_curFrame.plane(1);
    this->m_buffer.cr = this->m_curFrame.plane(2);
    this->m_buffer.y_stride  = this->m_curFrame.lineSize(0);
    this->m_buffer.cb_stride = this->m_curFrame.lineSize(1);
    this->m_buffer.cr_stride = this->m_curFrame.lineSize(2);
    this->m_frame.n_filled_len = this->m_curFrame.size();

    this->m_frame.pts = src.pts();
    auto result = eb_vp9_svt_enc_send_picture(this->m_encoder, &this->m_frame);

    if (result != EB_ErrorNone)
        qCritical() << "Error sending the frame: "
                    << VideoEncoderSvtVp9ElementPrivate::errortToStr(result);

    for (;;) {
        EbBufferHeaderType *packet = nullptr;
        result = eb_vp9_svt_get_packet(this->m_encoder, &packet, 0);

        if (result != EB_ErrorNone)
            break;

        this->sendFrame(packet);
        eb_vp9_svt_release_out_buffer(&packet);
    }

    this->m_encodedTimePts = src.pts() + src.duration();
    emit self->encodedTimePtsChanged(this->m_encodedTimePts);
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
    packet.setTimeBase(this->m_outputCaps.rawCaps().fps().invert());
    packet.setId(this->m_id);
    packet.setIndex(this->m_index);

    emit self->oStream(packet);
}

int VideoEncoderSvtVp9ElementPrivate::vp9Level(const AkVideoCaps &caps) const
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

    quint64 lumaPictureSize = caps.width() * caps.height();
    quint64 lumaSampleRate = qRound64(lumaPictureSize * caps.fps().value());
    int bitrate = self->bitrate();
    int dimension = qMax(caps.width(), caps.height());

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
