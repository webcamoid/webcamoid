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
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akcompressedvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>
#include <akcompressedvideopacket.h>
#include <iak/akelement.h>
#include <aom/aomcx.h>
#include <aom/aom_encoder.h>

#include "videoencoderav1element.h"

struct Av1PixFormatTable
{
    AkVideoCaps::PixelFormat pixFormat;
    aom_img_fmt_t av1Format;
    size_t depth;
    bool monochrome;
    aom_codec_flags_t flags;
    unsigned int profile;

    static inline const Av1PixFormatTable *table()
    {
        static const Av1PixFormatTable aomAv1PixFormatTable[] = {
            {AkVideoCaps::Format_y8       , AOM_IMG_FMT_I420  , 8 , true , 0                         , 0},
            {AkVideoCaps::Format_y10      , AOM_IMG_FMT_I42016, 10, true , AOM_CODEC_USE_HIGHBITDEPTH, 0},
            {AkVideoCaps::Format_y12      , AOM_IMG_FMT_I42016, 12, true , AOM_CODEC_USE_HIGHBITDEPTH, 0},
            {AkVideoCaps::Format_nv12     , AOM_IMG_FMT_NV12  , 8 , false, 0                         , 1},
            {AkVideoCaps::Format_yvu420p  , AOM_IMG_FMT_YV12  , 8 , false, 0                         , 1},
            {AkVideoCaps::Format_yuv420p  , AOM_IMG_FMT_I420  , 8 , false, 0                         , 0},
            {AkVideoCaps::Format_yuv422p  , AOM_IMG_FMT_I422  , 8 , false, 0                         , 2},
            {AkVideoCaps::Format_yuv444p  , AOM_IMG_FMT_I444  , 8 , false, 0                         , 1},
            {AkVideoCaps::Format_yvu420p10, AOM_IMG_FMT_YV1216, 10, false, AOM_CODEC_USE_HIGHBITDEPTH, 2},
            {AkVideoCaps::Format_yvu420p12, AOM_IMG_FMT_YV1216, 12, false, AOM_CODEC_USE_HIGHBITDEPTH, 2},
            {AkVideoCaps::Format_yuv420p10, AOM_IMG_FMT_I42016, 10, false, AOM_CODEC_USE_HIGHBITDEPTH, 0},
            {AkVideoCaps::Format_yuv420p12, AOM_IMG_FMT_I42016, 12, false, AOM_CODEC_USE_HIGHBITDEPTH, 2},
            {AkVideoCaps::Format_yuv422p10, AOM_IMG_FMT_I42216, 10, false, AOM_CODEC_USE_HIGHBITDEPTH, 2},
            {AkVideoCaps::Format_yuv422p12, AOM_IMG_FMT_I42216, 12, false, AOM_CODEC_USE_HIGHBITDEPTH, 2},
            {AkVideoCaps::Format_yuv444p10, AOM_IMG_FMT_I44416, 10, false, AOM_CODEC_USE_HIGHBITDEPTH, 2},
            {AkVideoCaps::Format_yuv444p12, AOM_IMG_FMT_I44416, 12, false, AOM_CODEC_USE_HIGHBITDEPTH, 2},
            {AkVideoCaps::Format_none     , AOM_IMG_FMT_NONE  , 0 , false, 0                         , 0},
        };

        return aomAv1PixFormatTable;
    }

    static inline const Av1PixFormatTable *byPixFormat(AkVideoCaps::PixelFormat format)
    {
        auto fmt = table();

        for (; fmt->pixFormat != AkVideoCaps::Format_none; fmt++)
            if (fmt->pixFormat == format)
                return fmt;

        return fmt;
    }

    static inline const Av1PixFormatTable *byAv1Format(aom_img_fmt_t format,
                                                       size_t depth,
                                                       bool monochrome)
    {
        auto fmt = table();

        for (; fmt->pixFormat != AkVideoCaps::Format_none; fmt++)
            if (fmt->av1Format == format
                && fmt->depth == depth
                && fmt->monochrome == monochrome)
                return fmt;

        return fmt;
    }
};

class VideoEncoderAv1ElementPrivate
{
    public:
        VideoEncoderAv1Element *self;
        AkVideoConverter m_videoConverter;
        AkCompressedVideoCaps m_outputCaps;
        VideoEncoderAv1Element::ErrorResilientFlag m_errorResilient {VideoEncoderAv1Element::ErrorResilientFlag_NoFlags};
        int m_speed {11};
        VideoEncoderAv1Element::Usage m_usage {VideoEncoderAv1Element::Usage_RealTime};
        bool m_lossless {false};
        VideoEncoderAv1Element::TuneContent m_tuneContent {VideoEncoderAv1Element::TuneContent_Default};
        AkCompressedVideoPackets m_headers;
        aom_codec_iface_t *m_interface {nullptr};
        aom_codec_ctx m_encoder;
        aom_image_t m_frame;
        QMutex m_mutex;
        qint64 m_id {0};
        int m_index {0};
        bool m_initialized {false};
        bool m_paused {false};
        qint64 m_encodedTimePts {0};
        AkElementPtr m_fpsControl {akPluginManager->create<AkElement>("VideoFilter/FpsControl")};

        explicit VideoEncoderAv1ElementPrivate(VideoEncoderAv1Element *self);
        ~VideoEncoderAv1ElementPrivate();
        bool init();
        void uninit();
        void updateHeaders();
        void updateOutputCaps(const AkVideoCaps &inputCaps);
        static void printError(aom_codec_err_t error,
                               const aom_codec_ctx_t *codecContext=nullptr);
        void encodeFrame(const AkVideoPacket &src);
        void sendFrame(const aom_codec_cx_pkt_t *aomPacket) const;
        unsigned int aomLevel(const AkVideoCaps &caps) const;
};

VideoEncoderAv1Element::VideoEncoderAv1Element():
    AkVideoEncoder()
{
    this->d = new VideoEncoderAv1ElementPrivate(this);
    this->setCodec(this->codecs().value(0));
}

VideoEncoderAv1Element::~VideoEncoderAv1Element()
{
    this->d->uninit();
    delete this->d;
}

QStringList VideoEncoderAv1Element::codecs() const
{
    return {"libaom"};
}

AkVideoEncoderCodecID VideoEncoderAv1Element::codecID(const QString &codec) const
{
    return codec == this->codecs().first()?
                AkCompressedVideoCaps::VideoCodecID_av1:
                AkCompressedVideoCaps::VideoCodecID_unknown;
}

QString VideoEncoderAv1Element::codecDescription(const QString &codec) const
{
    return codec == this->codecs().first()?
                QStringLiteral("AV1 (libaom)"):
                QString();
}

AkCompressedVideoCaps VideoEncoderAv1Element::outputCaps() const
{
    return this->d->m_outputCaps;
}

AkCompressedPackets VideoEncoderAv1Element::headers() const
{
    AkCompressedPackets packets;

    for (auto &header: this->d->m_headers)
        packets << header;

    return packets;
}

qint64 VideoEncoderAv1Element::encodedTimePts() const
{
    return this->d->m_encodedTimePts;
}

VideoEncoderAv1Element::ErrorResilientFlag VideoEncoderAv1Element::errorResilient() const
{
    return this->d->m_errorResilient;
}

int VideoEncoderAv1Element::speed() const
{
    return this->d->m_speed;
}

VideoEncoderAv1Element::Usage VideoEncoderAv1Element::usage() const
{
    return this->d->m_usage;
}

bool VideoEncoderAv1Element::lossless() const
{
    return this->d->m_lossless;
}

VideoEncoderAv1Element::TuneContent VideoEncoderAv1Element::tuneContent() const
{
    return this->d->m_tuneContent;
}

QString VideoEncoderAv1Element::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/VideoEncoderAv1/share/qml/main.qml");
}

void VideoEncoderAv1Element::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("VideoEncoderAv1", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket VideoEncoderAv1Element::iVideoStream(const AkVideoPacket &packet)
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

void VideoEncoderAv1Element::setErrorResilient(ErrorResilientFlag errorResilient)
{
    if (errorResilient == this->d->m_errorResilient)
        return;

    this->d->m_errorResilient = errorResilient;
    emit this->errorResilientChanged(errorResilient);
}

void VideoEncoderAv1Element::setSpeed(int speed)
{
    if (speed == this->d->m_speed)
        return;

    this->d->m_speed = speed;
    emit this->speedChanged(speed);
}

void VideoEncoderAv1Element::setUsage(Usage usage)
{
    if (usage == this->d->m_usage)
        return;

    this->d->m_usage = usage;
    emit this->usageChanged(usage);
}

void VideoEncoderAv1Element::setLossless(bool lossless)
{
    if (lossless == this->d->m_lossless)
        return;

    this->d->m_lossless = lossless;
    emit this->losslessChanged(lossless);
}

void VideoEncoderAv1Element::setTuneContent(TuneContent tuneContent)
{
    if (tuneContent == this->d->m_tuneContent)
        return;

    this->d->m_tuneContent = tuneContent;
    emit this->tuneContentChanged(tuneContent);
}

void VideoEncoderAv1Element::resetErrorResilient()
{
    this->setErrorResilient(VideoEncoderAv1Element::ErrorResilientFlag_NoFlags);
}

void VideoEncoderAv1Element::resetSpeed()
{
    this->setSpeed(11);
}

void VideoEncoderAv1Element::resetUsage()
{
    this->setUsage(Usage_RealTime);
}

void VideoEncoderAv1Element::resetLossless()
{
    this->setLossless(false);
}

void VideoEncoderAv1Element::resetTuneContent()
{
    this->setTuneContent(TuneContent_Default);
}

void VideoEncoderAv1Element::resetOptions()
{
    AkVideoEncoder::resetOptions();
    this->resetErrorResilient();
    this->resetSpeed();
    this->resetUsage();
    this->resetLossless();
    this->resetTuneContent();
}

bool VideoEncoderAv1Element::setState(ElementState state)
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

VideoEncoderAv1ElementPrivate::VideoEncoderAv1ElementPrivate(VideoEncoderAv1Element *self):
    self(self)
{
    this->m_interface = aom_codec_av1_cx();
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

VideoEncoderAv1ElementPrivate::~VideoEncoderAv1ElementPrivate()
{

}

bool VideoEncoderAv1ElementPrivate::init()
{
    this->uninit();

    if (!this->m_interface) {
        qCritical() << "AV1 Codec interface was not initialized.";

        return false;
    }

    auto inputCaps = self->inputCaps();

    if (!inputCaps) {
        qCritical() << "Invalid input format.";

        return false;
    }

    auto eqFormat =
            Av1PixFormatTable::byPixFormat(this->m_videoConverter.outputCaps().format());

    aom_codec_enc_cfg_t codecConfigs;
    memset(&codecConfigs, 0, sizeof(aom_codec_enc_cfg));
    auto result =
            aom_codec_enc_config_default(this->m_interface,
                                         &codecConfigs,
                                         static_cast<unsigned int>(this->m_usage));

    if (result != AOM_CODEC_OK) {
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
    codecConfigs.rc_end_usage = AOM_CBR;
    codecConfigs.rc_target_bitrate = self->bitrate() / 1000;
    codecConfigs.g_bit_depth = aom_bit_depth(eqFormat->depth);
    codecConfigs.g_input_bit_depth = eqFormat->depth;
    codecConfigs.monochrome = eqFormat->monochrome;
    codecConfigs.g_error_resilient = this->m_errorResilient;
    codecConfigs.g_pass = AOM_RC_ONE_PASS;
    codecConfigs.kf_max_dist =
            qMax(self->gop() * this->m_videoConverter.outputCaps().fps().num()
                 / (1000 * this->m_videoConverter.outputCaps().fps().den()), 1);

    memset(&this->m_encoder, 0, sizeof(aom_codec_ctx));
    result = aom_codec_enc_init(&this->m_encoder,
                                this->m_interface,
                                &codecConfigs,
                                eqFormat->flags);

    if (result != AOM_CODEC_OK) {
        printError(result, &this->m_encoder);

        return false;
    }

    int speed = qBound(0, this->m_speed, 11);
    aom_codec_control(&this->m_encoder, AOME_SET_CPUUSED, speed);
    auto level = this->aomLevel(this->m_videoConverter.outputCaps());
    aom_codec_control(&this->m_encoder,
                      AV1E_SET_TARGET_SEQ_LEVEL_IDX,
                      static_cast<unsigned int>(level));
    aom_codec_control(&this->m_encoder,
                      AV1E_SET_LOSSLESS,
                      static_cast<unsigned int>(this->m_lossless));

    int tune = AOM_CONTENT_DEFAULT;

    switch (this->m_tuneContent) {
    case VideoEncoderAv1Element::TuneContent_Screen:
        tune = AOM_CONTENT_SCREEN;
        break;
    case VideoEncoderAv1Element::TuneContent_Film:
        tune = AOM_CONTENT_FILM;
        break;
    default:
        break;
    }

    aom_codec_control(&this->m_encoder, AV1E_SET_TUNE_CONTENT, tune);
    memset(&this->m_frame, 0, sizeof(aom_image_t));

    if (!aom_img_alloc(&this->m_frame,
                       eqFormat->av1Format,
                       this->m_videoConverter.outputCaps().width(),
                       this->m_videoConverter.outputCaps().height(),
                       1)) {
        qCritical() << "Failed to allocate the input frame";
        aom_codec_destroy(&this->m_encoder);

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

void VideoEncoderAv1ElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;
    aom_codec_iter_t iter = nullptr;

    for (;;) {
        auto packet = aom_codec_get_cx_data(&this->m_encoder, &iter);

        if (!packet)
            break;

        if (packet->kind != AOM_CODEC_CX_FRAME_PKT)
            continue;

        this->sendFrame(packet);
    }

    aom_img_free(&this->m_frame);
    aom_codec_destroy(&this->m_encoder);

    if (this->m_fpsControl)
        QMetaObject::invokeMethod(this->m_fpsControl.data(),
                                  "restart",
                                  Qt::DirectConnection);

    this->m_paused = false;
}

void VideoEncoderAv1ElementPrivate::updateHeaders()
{
    auto headers = aom_codec_get_global_headers(&this->m_encoder);

    if (!headers)
        return;

    AkCompressedVideoPacket headerPacket(this->m_outputCaps,
                                         headers->sz);
    memcpy(headerPacket.data(),
           headers->buf,
           headerPacket.size());
    headerPacket.setTimeBase(this->m_outputCaps.rawCaps().fps().invert());
    headerPacket.setFlags(AkCompressedVideoPacket::VideoPacketTypeFlag_Header);
    this->m_headers = {headerPacket};
    emit self->headersChanged(self->headers());
    free(headers->buf);
    free(headers);
}

void VideoEncoderAv1ElementPrivate::updateOutputCaps(const AkVideoCaps &inputCaps)
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

void VideoEncoderAv1ElementPrivate::printError(aom_codec_err_t error,
                                               const aom_codec_ctx_t *codecContext)
{
    if (codecContext) {
        auto errorStr = aom_codec_error_detail(codecContext);

        if (QString(errorStr).isEmpty())
            qCritical() << aom_codec_err_to_string(error);
        else
            qCritical() << errorStr;
    } else {
        qCritical() << aom_codec_err_to_string(error);
    }
}

void VideoEncoderAv1ElementPrivate::encodeFrame(const AkVideoPacket &src)
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

    auto result = aom_codec_encode(&this->m_encoder,
                                   &this->m_frame,
                                   src.pts(),
                                   src.duration(),
                                   0);

    if (result != AOM_CODEC_OK)
        this->printError(result, &this->m_encoder);

    aom_codec_iter_t iter = nullptr;

    for (;;) {
        auto aomPacket = aom_codec_get_cx_data(&this->m_encoder, &iter);

        if (!aomPacket)
            break;

        if (aomPacket->kind != AOM_CODEC_CX_FRAME_PKT)
            continue;

        this->sendFrame(aomPacket);
    }

    this->m_encodedTimePts = src.pts() + src.duration();
    emit self->encodedTimePtsChanged(this->m_encodedTimePts);
}

void VideoEncoderAv1ElementPrivate::sendFrame(const aom_codec_cx_pkt_t *aomPacket) const
{
    AkCompressedVideoPacket packet(this->m_outputCaps,
                                   aomPacket->data.frame.sz);
    memcpy(packet.data(), aomPacket->data.frame.buf, packet.size());
    packet.setFlags(aomPacket->data.frame.flags & AOM_FRAME_IS_KEY?
                        AkCompressedVideoPacket::VideoPacketTypeFlag_KeyFrame:
                        AkCompressedVideoPacket::VideoPacketTypeFlag_None);
    packet.setPts(aomPacket->data.frame.pts);
    packet.setDts(aomPacket->data.frame.pts);
    packet.setDuration(aomPacket->data.frame.duration);
    packet.setTimeBase({this->m_encoder.config.enc->g_timebase.num,
                        this->m_encoder.config.enc->g_timebase.den});
    packet.setId(this->m_id);
    packet.setIndex(this->m_index);

    emit self->oStream(packet);
}

unsigned int VideoEncoderAv1ElementPrivate::aomLevel(const AkVideoCaps &caps) const
{
    // https://aomediacodec.github.io/av1-spec/#levels

    struct AomLevelsDef
    {
        quint64 maxLumaPictureSize;
        int maxWidth;
        int maxHeight;
        quint64 lumaSampleRate;
        quint64 maxBitrate;
    };
    static const AomLevelsDef aomLevels[] = {
        {147456L  , 2048 , 1152, 4423680L   , 5529600L   },
        {278784L  , 2816 , 1584, 8363520L   , 10454400L  },
        {665856L  , 4352 , 2448, 19975680L  , 24969600L  },
        {1065024L , 5504 , 3096, 31950720L  , 39938400L  },
        {2359296L , 6144 , 3456, 70778880L  , 77856768L  },
        {2359296L , 6144 , 3456, 141557760L , 155713536L },
        {8912896L , 8192 , 4352, 267386880L , 273715200L },
        {8912896L , 8192 , 4352, 534773760L , 547430400L },
        {8912896L , 8192 , 4352, 1069547520L, 1094860800L},
        {8912896L , 8192 , 4352, 1069547520L, 1176502272L},
        {35651584L, 16384, 8704, 1069547520L, 1176502272L},
        {35651584L, 16384, 8704, 2139095040L, 2189721600L},
        {35651584L, 16384, 8704, 4278190080L, 4379443200L},
        {35651584L, 16384, 8704, 4278190080L, 4706009088L},
        {0L       , 0    , 0   , 0L         , 0L         },
    };

    quint64 lumaPictureSize = caps.width() * caps.height();
    quint64 lumaSampleRate = qRound64(lumaPictureSize * caps.fps().value());
    int bitrate = self->bitrate();
    unsigned int i = 0;

    for (auto level = aomLevels; level->maxLumaPictureSize; ++level, ++i)
        if (level->maxLumaPictureSize >= lumaPictureSize
            && level->maxWidth >= caps.width()
            && level->maxHeight >= caps.height()
            && level->lumaSampleRate >= lumaSampleRate
            && level->maxBitrate >= bitrate) {
            return i;
        }

    return 0;
}

#include "moc_videoencoderav1element.cpp"
