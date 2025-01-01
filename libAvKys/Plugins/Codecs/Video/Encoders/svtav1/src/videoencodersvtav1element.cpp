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
#include <akpluginmanager.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>
#include <akcompressedvideopacket.h>
#include <iak/akelement.h>
#include <EbSvtAv1Enc.h>
#include <EbSvtAv1ErrorCodes.h>
#include <EbSvtAv1Metadata.h>

#include "videoencodersvtav1element.h"

struct Av1PixFormatTable
{
    AkVideoCaps::PixelFormat pixFormat;
    EbColorFormat av1Format;
    size_t depth;
    EbAv1SeqProfile profile;

    static inline const Av1PixFormatTable *table()
    {
        static const Av1PixFormatTable av1PixFormatTable[] = {
            //{AkVideoCaps::Format_y8       , EB_YUV400        ,  8, MAIN_PROFILE        },
            {AkVideoCaps::Format_yuv420p  , EB_YUV420        ,  8, MAIN_PROFILE        },
            //{AkVideoCaps::Format_yuv422p  , EB_YUV422        ,  8, PROFESSIONAL_PROFILE},
            //{AkVideoCaps::Format_yuv444p  , EB_YUV444        ,  8, HIGH_PROFILE        },
            {AkVideoCaps::Format_yuv420p10, EB_YUV420        , 10, MAIN_PROFILE        },
            //{AkVideoCaps::Format_yuv422p10, EB_YUV422        , 10, PROFESSIONAL_PROFILE},
            //{AkVideoCaps::Format_yuv444p10, EB_YUV444        , 10, PROFESSIONAL_PROFILE},
            {AkVideoCaps::Format_none     , EbColorFormat(-1),  0, MAIN_PROFILE        },
        };

        return av1PixFormatTable;
    }

    static inline const Av1PixFormatTable *byPixFormat(AkVideoCaps::PixelFormat format)
    {
        auto fmt = table();

        for (; fmt->pixFormat != AkVideoCaps::Format_none; fmt++)
            if (fmt->pixFormat == format)
                return fmt;

        return fmt;
    }

    static inline const Av1PixFormatTable *byAv1Format(EbColorFormat format,
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

class VideoEncoderSvtAv1ElementPrivate
{
    public:
        VideoEncoderSvtAv1Element *self;
        AkVideoConverter m_videoConverter;
        AkCompressedVideoCaps m_outputCaps;
        int m_speed {9};
        VideoEncoderSvtAv1Element::TuneContent m_tuneContent {VideoEncoderSvtAv1Element::TuneContent_PSNR};
        AkCompressedVideoPackets m_headers;
        EbComponentType *m_encoder {nullptr};
        EbSvtIOFormat m_buffer;
        EbBufferHeaderType m_frame;
        QMutex m_mutex;
        qint64 m_id {0};
        int m_index {0};
        bool m_initialized {false};
        bool m_paused {false};
        qint64 m_encodedTimePts {0};
        AkVideoPacket m_curFrame;
        AkElementPtr m_fpsControl {akPluginManager->create<AkElement>("VideoFilter/FpsControl")};

        explicit VideoEncoderSvtAv1ElementPrivate(VideoEncoderSvtAv1Element *self);
        ~VideoEncoderSvtAv1ElementPrivate();
        bool init();
        void uninit();
        void updateHeaders();
        void updateOutputCaps(const AkVideoCaps &inputCaps);
        static const char *errortToStr(EbErrorType error);
        void encodeFrame(const AkVideoPacket &src);
        void sendFrame(const EbBufferHeaderType *buffer) const;
        int av1Level(const AkVideoCaps &caps) const;
};

VideoEncoderSvtAv1Element::VideoEncoderSvtAv1Element():
    AkVideoEncoder()
{
    this->d = new VideoEncoderSvtAv1ElementPrivate(this);
    this->setCodec(this->codecs().value(0));
}

VideoEncoderSvtAv1Element::~VideoEncoderSvtAv1Element()
{
    this->d->uninit();
    delete this->d;
}

QStringList VideoEncoderSvtAv1Element::codecs() const
{
    return {"svtav1"};
}

AkVideoEncoderCodecID VideoEncoderSvtAv1Element::codecID(const QString &codec) const
{
    return codec == this->codecs().first()?
                AkCompressedVideoCaps::VideoCodecID_av1:
                AkCompressedVideoCaps::VideoCodecID_unknown;
}

QString VideoEncoderSvtAv1Element::codecDescription(const QString &codec) const
{
    return codec == this->codecs().first()?
                QStringLiteral("SVT-AV1"):
                QString();
}

AkCompressedVideoCaps VideoEncoderSvtAv1Element::outputCaps() const
{
    return this->d->m_outputCaps;
}

AkCompressedPackets VideoEncoderSvtAv1Element::headers() const
{
    AkCompressedPackets packets;

    for (auto &header: this->d->m_headers)
        packets << header;

    return packets;
}

qint64 VideoEncoderSvtAv1Element::encodedTimePts() const
{
    return this->d->m_encodedTimePts;
}

int VideoEncoderSvtAv1Element::speed() const
{
    return this->d->m_speed;
}

VideoEncoderSvtAv1Element::TuneContent VideoEncoderSvtAv1Element::tuneContent() const
{
    return this->d->m_tuneContent;
}

QString VideoEncoderSvtAv1Element::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/VideoEncoderSvtAv1/share/qml/main.qml");
}

void VideoEncoderSvtAv1Element::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("VideoEncoderSvtAv1", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket VideoEncoderSvtAv1Element::iVideoStream(const AkVideoPacket &packet)
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

void VideoEncoderSvtAv1Element::setSpeed(int speed)
{
    if (speed == this->d->m_speed)
        return;

    this->d->m_speed = speed;
    emit this->speedChanged(speed);
}

void VideoEncoderSvtAv1Element::setTuneContent(TuneContent tuneContent)
{
    if (tuneContent == this->d->m_tuneContent)
        return;

    this->d->m_tuneContent = tuneContent;
    emit this->tuneContentChanged(tuneContent);
}

void VideoEncoderSvtAv1Element::resetSpeed()
{
    this->setSpeed(9);
}

void VideoEncoderSvtAv1Element::resetTuneContent()
{
    this->setTuneContent(TuneContent_PSNR);
}

void VideoEncoderSvtAv1Element::resetOptions()
{
    AkVideoEncoder::resetOptions();
    this->resetSpeed();
    this->resetTuneContent();
}

bool VideoEncoderSvtAv1Element::setState(ElementState state)
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

VideoEncoderSvtAv1ElementPrivate::VideoEncoderSvtAv1ElementPrivate(VideoEncoderSvtAv1Element *self):
    self(self)
{
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

VideoEncoderSvtAv1ElementPrivate::~VideoEncoderSvtAv1ElementPrivate()
{
}

bool VideoEncoderSvtAv1ElementPrivate::init()
{
    this->uninit();

    auto inputCaps = self->inputCaps();

    if (!inputCaps) {
        qCritical() << "Invalid input format.";

        return false;
    }

    auto eqFormat =
            Av1PixFormatTable::byPixFormat(this->m_videoConverter.outputCaps().format());

    if (eqFormat->pixFormat == AkVideoCaps::Format_none)
        eqFormat = Av1PixFormatTable::byPixFormat(AkVideoCaps::Format_yuv420p);

    EbSvtAv1EncConfiguration configs;
    auto result = svt_av1_enc_init_handle(&this->m_encoder,
                                          this,
                                          &configs);

    if (result != EB_ErrorNone) {
        qCritical() << "Error initializing the encoder: "
                    << errortToStr(result);

        return false;
    }

    configs.enc_mode = qBound(0, this->m_speed, 9);
    configs.tune = uint8_t(this->m_tuneContent);
    configs.target_socket = -1;
    configs.logical_processors = QThread::idealThreadCount();
    configs.rate_control_mode = SVT_AV1_RC_MODE_CBR;
    configs.pred_structure = SVT_AV1_PRED_LOW_DELAY_B; // Otherwise CBR won't work
    configs.target_bit_rate = self->bitrate();
    configs.profile = eqFormat->profile;
    configs.color_primaries = EB_CICP_CP_UNSPECIFIED;
    configs.matrix_coefficients = EB_CICP_MC_UNSPECIFIED;
    configs.transfer_characteristics = EB_CICP_TC_UNSPECIFIED;
    configs.color_range = EB_CR_STUDIO_RANGE;
    int gop = qMax(self->gop() * this->m_videoConverter.outputCaps().fps().num()
                   / (1000 * this->m_videoConverter.outputCaps().fps().den()), 1);
    configs.intra_period_length = gop;

#if SVT_AV1_CHECK_VERSION(1, 1, 0)
    if (gop == 1)
        configs.force_key_frames = 1;
#endif

    configs.source_width = this->m_videoConverter.outputCaps().width();
    configs.source_height = this->m_videoConverter.outputCaps().height();
    configs.frame_rate_numerator =
            this->m_videoConverter.outputCaps().fps().num();
    configs.frame_rate_denominator =
            this->m_videoConverter.outputCaps().fps().den();
    configs.encoder_bit_depth = eqFormat->depth;
    configs.encoder_color_format = eqFormat->av1Format;
    configs.level = this->av1Level(this->m_videoConverter.outputCaps());
    result = svt_av1_enc_set_parameter(this->m_encoder, &configs);

    if (result != EB_ErrorNone) {
        qCritical() << "Error initializing the encoder: "
                    << errortToStr(result);

        return false;
    }

    result = svt_av1_enc_init(this->m_encoder);

    if (result != EB_ErrorNone) {
        qCritical() << "Error initializing the encoder: "
                    << errortToStr(result);

        return false;
    }

    memset(&this->m_buffer, 0, sizeof(EbSvtIOFormat));
    memset(&this->m_frame, 0, sizeof(EbBufferHeaderType));
    this->m_frame.size = sizeof(EbBufferHeaderType);
    this->m_frame.p_buffer = reinterpret_cast<uint8_t *>(&this->m_buffer);
    this->m_frame.pic_type =
            gop == 1? EB_AV1_KEY_PICTURE: EB_AV1_INVALID_PICTURE;
    this->updateHeaders();

    if (this->m_fpsControl) {
        this->m_fpsControl->setProperty("fps",
                                        QVariant::fromValue(this->m_videoConverter.outputCaps().fps()));
        this->m_fpsControl->setProperty("fillGaps", self->fillGaps());
        QMetaObject::invokeMethod(this->m_fpsControl.data(),
                                  "restart",
                                  Qt::DirectConnection);
    }

    this->m_encodedTimePts = 0;
    this->m_initialized = true;

    return true;
}

void VideoEncoderSvtAv1ElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;

    if (this->m_encoder) {
        EbBufferHeaderType eos;
        memset(&eos, 0, sizeof(EbBufferHeaderType));
        eos.pic_type = EB_AV1_INVALID_PICTURE;
        eos.flags    = EB_BUFFERFLAG_EOS;
        auto result = svt_av1_enc_send_picture(this->m_encoder, &eos);

        if (result != EB_ErrorNone)
            qCritical() << "Error sending EOS:" << errortToStr(result);

        for (;;) {
            EbBufferHeaderType *packet = nullptr;
            result = svt_av1_enc_get_packet(this->m_encoder, &packet, 1);

            if (result != EB_ErrorNone)
                break;

            bool isEos = packet->flags & EB_BUFFERFLAG_EOS;
            this->sendFrame(packet);
            svt_av1_enc_release_out_buffer(&packet);

            if (isEos)
                break;
        }

        svt_av1_enc_deinit(this->m_encoder);
        svt_av1_enc_deinit_handle(this->m_encoder);
        this->m_encoder = nullptr;
    }

    if (this->m_fpsControl)
        QMetaObject::invokeMethod(this->m_fpsControl.data(),
                                  "restart",
                                  Qt::DirectConnection);

    this->m_paused = false;
}

void VideoEncoderSvtAv1ElementPrivate::updateHeaders()
{
    EbBufferHeaderType *header = nullptr;

    if (svt_av1_enc_stream_header(this->m_encoder, &header) == EB_ErrorNone) {
        AkCompressedVideoPacket headerPacket(this->m_outputCaps,
                                             header->n_filled_len);
        memcpy(headerPacket.data(),
               header->p_buffer,
               headerPacket.size());
        headerPacket.setTimeBase(this->m_outputCaps.rawCaps().fps().invert());
        headerPacket.setFlags(AkCompressedVideoPacket::VideoPacketTypeFlag_Header);
        this->m_headers = {headerPacket};
        emit self->headersChanged(self->headers());
        svt_av1_enc_stream_header_release(header);
    }
}

void VideoEncoderSvtAv1ElementPrivate::updateOutputCaps(const AkVideoCaps &inputCaps)
{
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

    auto eqFormat = Av1PixFormatTable::byPixFormat(inputCaps.format());

    if (eqFormat->pixFormat == AkVideoCaps::Format_none)
        eqFormat = Av1PixFormatTable::byPixFormat(AkVideoCaps::Format_yuv420p);

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

const char *VideoEncoderSvtAv1ElementPrivate::errortToStr(EbErrorType error)
{
    struct ErrorToStr
    {
        EbErrorType key;
        const char *value;
    };

    static const ErrorToStr errorToStrTbl[] = {
        {EB_DecUnsupportedBitstream    , "Unsupported bitstream"   },
        {EB_DecNoOutputPicture         , "No output picture"       },
        {EB_DecDecodingError           , "Decoding error"          },
        {EB_Corrupt_Frame              , "Corrupt frame"           },
        {EB_ErrorInsufficientResources , "Insufficient resources"  },
        {EB_ErrorUndefined             , "Undefined"               },
        {EB_ErrorInvalidComponent      , "Invalid component"       },
        {EB_ErrorBadParameter          , "Bad parameter"           },
        {EB_ErrorDestroyThreadFailed   , "Destroy thread failed"   },
        {EB_ErrorSemaphoreUnresponsive , "Semaphore unresponsive"  },
        {EB_ErrorDestroySemaphoreFailed, "Destroy semaphore failed"},
        {EB_ErrorCreateMutexFailed     , "Create mutex failed"     },
        {EB_ErrorMutexUnresponsive     , "Mutex unresponsive"      },
        {EB_ErrorDestroyMutexFailed    , "Mutex failed"            },
        {EB_NoErrorEmptyQueue          , "Empty queue"             },
        {EB_NoErrorFifoShutdown        , "Fifo shutdown"           },
        {EB_ErrorNone                  , "No error"                },
    };

    for (int i = 0; errorToStrTbl[i].key != EB_ErrorNone; i++)
        if (error == errorToStrTbl[i].key)
            return errorToStrTbl[i].value;

    return "No error";
}

void VideoEncoderSvtAv1ElementPrivate::encodeFrame(const AkVideoPacket &src)
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
    auto result = svt_av1_enc_send_picture(this->m_encoder, &this->m_frame);

    if (result != EB_ErrorNone)
        qCritical() << "Error sending the frame: "
                    << VideoEncoderSvtAv1ElementPrivate::errortToStr(result);

    for (;;) {
        EbBufferHeaderType *packet = nullptr;
        result = svt_av1_enc_get_packet(this->m_encoder, &packet, 0);

        if (result != EB_ErrorNone)
            break;

        this->sendFrame(packet);
        svt_av1_enc_release_out_buffer(&packet);
    }

    this->m_encodedTimePts = src.pts() + src.duration();
    emit self->encodedTimePtsChanged(this->m_encodedTimePts);
}

void VideoEncoderSvtAv1ElementPrivate::sendFrame(const EbBufferHeaderType *buffer) const
{
    AkCompressedVideoPacket packet(this->m_outputCaps,
                                   buffer->n_filled_len);
    memcpy(packet.data(), buffer->p_buffer, packet.size());
    packet.setFlags(buffer->pic_type == EB_AV1_KEY_PICTURE?
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

int VideoEncoderSvtAv1ElementPrivate::av1Level(const AkVideoCaps &caps) const
{
    // https://aomediacodec.github.io/av1-spec/#levels

    struct Av1LevelsDef
    {
        int level;
        quint64 maxLumaPictureSize;
        int maxWidth;
        int maxHeight;
        quint64 lumaSampleRate;
        quint64 maxBitrate;
    };
    static const Av1LevelsDef av1Levels[] = {
        {20, 147456L  , 2048 , 1152, 4423680L   , 5529600L   },
        {21, 278784L  , 2816 , 1584, 8363520L   , 10454400L  },
        {30, 665856L  , 4352 , 2448, 19975680L  , 24969600L  },
        {31, 1065024L , 5504 , 3096, 31950720L  , 39938400L  },
        {40, 2359296L , 6144 , 3456, 70778880L  , 77856768L  },
        {41, 2359296L , 6144 , 3456, 141557760L , 155713536L },
        {50, 8912896L , 8192 , 4352, 267386880L , 273715200L },
        {51, 8912896L , 8192 , 4352, 534773760L , 547430400L },
        {52, 8912896L , 8192 , 4352, 1069547520L, 1094860800L},
        {53, 8912896L , 8192 , 4352, 1069547520L, 1176502272L},
        {60, 35651584L, 16384, 8704, 1069547520L, 1176502272L},
        {61, 35651584L, 16384, 8704, 2139095040L, 2189721600L},
        {62, 35651584L, 16384, 8704, 4278190080L, 4379443200L},
        {63, 35651584L, 16384, 8704, 4278190080L, 4706009088L},
        {0 , 0L       , 0    , 0   , 0L         , 0L         },
    };

    quint64 lumaPictureSize = caps.width() * caps.height();
    quint64 lumaSampleRate = qRound64(lumaPictureSize * caps.fps().value());
    int bitrate = self->bitrate();

    for (auto level = av1Levels; level->level; ++level)
        if (level->maxLumaPictureSize >= lumaPictureSize
            && level->maxWidth >= caps.width()
            && level->maxHeight >= caps.height()
            && level->lumaSampleRate >= lumaSampleRate
            && level->maxBitrate >= bitrate) {
            return level->level;
        }

    return 0;
}

#include "moc_videoencodersvtav1element.cpp"
