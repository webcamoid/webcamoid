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

#include <QJniObject>
#include <QMutex>
#include <QThread>
#include <QVariant>
#include <akfrac.h>
#include <akaudiocaps.h>
#include <akaudiopacket.h>
#include <akcompressedaudiocaps.h>
#include <akcompressedaudiopacket.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <iak/akelement.h>
#include <media/NdkMediaCodec.h>

#include "audioencoderndkmediaelement.h"

#define BITRATE_MODE_CQ  0
#define BITRATE_MODE_VBR 1
#define BITRATE_MODE_CBR 2

#define ENCODING_PCM_16BIT 0x2
#define ENCODING_PCM_8BIT  0x3
#define ENCODING_PCM_FLOAT 0x4

#define CHANNEL_MASK_MONO                  0x2
#define CHANNEL_MASK_FRONT_LEFT            0x4
#define CHANNEL_MASK_FRONT_RIGHT           0x8
#define CHANNEL_MASK_FRONT_CENTER          0x10
#define CHANNEL_MASK_LOW_FREQUENCY         0x20
#define CHANNEL_MASK_BACK_LEFT             0x40
#define CHANNEL_MASK_BACK_RIGHT            0x80
#define CHANNEL_MASK_FRONT_LEFT_OF_CENTER  0x100
#define CHANNEL_MASK_FRONT_RIGHT_OF_CENTER 0x200
#define CHANNEL_MASK_BACK_CENTER           0x400
#define CHANNEL_MASK_SIDE_LEFT             0x800
#define CHANNEL_MASK_SIDE_RIGHT            0x1000

#define FRAME_SIZE 1024
#define PROCESSING_TIMEOUT 3000

#define AudioCodecID_amvorbis AkCompressedAudioCaps::AudioCodecID(AK_MAKE_FOURCC(0xA, 'V', 'O', 'R'))
#define AudioCodecID_amopus   AkCompressedAudioCaps::AudioCodecID(AK_MAKE_FOURCC(0xA, 'O', 'P', 'U'))
#define AudioCodecID_amaac    AkCompressedAudioCaps::AudioCodecID(AK_MAKE_FOURCC(0xA, 'A', 'A', 'C'))
#define AudioCodecID_ammp3    AkCompressedAudioCaps::AudioCodecID(AK_MAKE_FOURCC(0xA, 'M', 'P', '3'))

struct NDKMediaCodecs
{
    AkAudioEncoderCodecID codecID;
    const char *mimeType;
    const char *name;
    const char *description;

    static inline const NDKMediaCodecs *table()
    {
        static const NDKMediaCodecs ndkmediaAudioEncCodecsTable[] = {
            {AudioCodecID_amvorbis                      , "audio/vorbis"   , "vorbis", "Vorbis"},
            {AudioCodecID_amopus                        , "audio/opus"     , "opus"  , "Opus"  },
            {AudioCodecID_amaac                         , "audio/mp4a-latm", "aac"   , "AAC"   },
            {AudioCodecID_ammp3                         , "audio/mpeg"     , "mp3"   , "MP3"   },
            {AkCompressedAudioCaps::AudioCodecID_unknown, ""               , ""      , ""      },
        };

        return ndkmediaAudioEncCodecsTable;
    }

    static inline QStringList codecs()
    {
        QStringList codecs;

        for (auto codec = table();
             codec->codecID != AkCompressedAudioCaps::AudioCodecID_unknown;
             codec++) {
            codecs << codec->name;
        }

        return codecs;
    }

    static inline const NDKMediaCodecs *byCodecID(AkAudioEncoderCodecID codecID)
    {
        auto codec = table();

        for (;
             codec->codecID != AkCompressedAudioCaps::AudioCodecID_unknown;
             codec++) {
            if (codec->codecID == codecID)
                return codec;
        }

        return codec;
    }

    static inline const NDKMediaCodecs *byName(const QString &name)
    {
        auto codec = table();

        for (;
             codec->codecID != AkCompressedAudioCaps::AudioCodecID_unknown;
             codec++) {
            if (codec->name == name)
                return codec;
        }

        return codec;
    }

    static inline const NDKMediaCodecs *byMimeType(const QString &mimeType)
    {
        auto codec = table();

        for (;
             codec->codecID != AkCompressedAudioCaps::AudioCodecID_unknown;
             codec++) {
            if (codec->mimeType == mimeType)
                return codec;
        }

        return codec;
    }

    static inline bool containsMimeType(const QString &mimeType)
    {
        auto codec = table();

        for (;
             codec->codecID != AkCompressedAudioCaps::AudioCodecID_unknown;
             codec++) {
            if (codec->mimeType == mimeType)
                return true;
        }

        return false;
    }
};

struct CodecInfo
{
    QString name;
    QString description;
    QString ndkName;
    AkAudioEncoderCodecID codecID;
    QString mimeType;
    QVector<int32_t> formats;
};

struct SampleFormatsTable
{
    int32_t ndkFormat;
    AkAudioCaps::SampleFormat format;

    inline static const SampleFormatsTable *table()
    {
        static const SampleFormatsTable ndkmediaEncoderSampleFormatsTable[] {
            {ENCODING_PCM_16BIT, AkAudioCaps::SampleFormat_s16 },

#if __ANDROID_API__ >= 28
            {ENCODING_PCM_FLOAT, AkAudioCaps::SampleFormat_flt },
            {ENCODING_PCM_8BIT , AkAudioCaps::SampleFormat_u8  },
#endif

            {0                 , AkAudioCaps::SampleFormat_none},
        };

        return ndkmediaEncoderSampleFormatsTable;
    }

    inline static QString ndkFormatToString(int32_t format)
    {
        static const struct
        {
            int32_t ndkFormat;
            const char *str;
        } ndkAudioEncoderColorFormats[] {
            {ENCODING_PCM_16BIT, "16bit"},

#if __ANDROID_API__ >= 28
            {ENCODING_PCM_FLOAT, "float"},
            {ENCODING_PCM_8BIT , "8bit" },
    #endif

            {0                 , ""     },
        };

        for (auto item = ndkAudioEncoderColorFormats; item->ndkFormat; ++item)
            if (item->ndkFormat == format)
                return {item->str};

        return {};
    }

    inline static const SampleFormatsTable *byFormat(AkAudioCaps::SampleFormat format)
    {
        auto item = table();

        for (; item->format != AkAudioCaps::SampleFormat_none; ++item)
            if (item->format == format)
                return item;

        return item;
    }

    inline static const SampleFormatsTable *byNdkFormat(int32_t format)
    {
        auto item = table();

        for (; item->format != AkAudioCaps::SampleFormat_none; ++item)
            if (item->ndkFormat == format)
                return item;

        return item;
    }

    inline static AkAudioCaps::SampleFormatList formats()
    {
        AkAudioCaps::SampleFormatList formats;

        for (auto item = table();
             item->format != AkAudioCaps::SampleFormat_none;
             ++item) {
            formats << item->format;
        }

        return formats;
    }
};

using AMediaFormatPtr = QSharedPointer<AMediaFormat>;

class AudioEncoderNDKMediaElementPrivate
{
    public:
        AudioEncoderNDKMediaElement *self;
        AkCompressedAudioCaps m_outputCaps;
        QByteArray m_headers;
        QVector<CodecInfo> m_codecs;
        AMediaCodec *m_codec {nullptr};
        AMediaFormatPtr m_inputMediaFormat;
        AMediaFormatPtr m_outputMediaFormat;
        QMutex m_mutex;
        qint64 m_id {0};
        int m_index {0};
        bool m_initialized {false};
        bool m_paused {false};
        qint64 m_encodedTimePts {0};
        AkElementPtr m_fillAudioGaps {akPluginManager->create<AkElement>("AudioFilter/FillAudioGaps")};

        explicit AudioEncoderNDKMediaElementPrivate(AudioEncoderNDKMediaElement *self);
        ~AudioEncoderNDKMediaElementPrivate();
        static const char *errorToStr(media_status_t status);
        QString toValidName(const QString &name) const;
        bool isAvailable(const QString &mimeType) const;
        void listCodecs();
        bool init();
        void uninit();
        void updateHeaders();
        void updateOutputCaps();
        void encodeFrame(const AkAudioPacket &src);
        void sendFrame(const uint8_t *data,
                       const AMediaCodecBufferInfo &info) const;
};

AudioEncoderNDKMediaElement::AudioEncoderNDKMediaElement():
    AkAudioEncoder()
{
    this->d = new AudioEncoderNDKMediaElementPrivate(this);
    this->d->listCodecs();
    this->setCodec(NDKMediaCodecs::codecs().value(0));
}

AudioEncoderNDKMediaElement::~AudioEncoderNDKMediaElement()
{
    this->d->uninit();
    delete this->d;
}

QStringList AudioEncoderNDKMediaElement::codecs() const
{
    QStringList codecs;

    for (auto &codec: this->d->m_codecs)
        codecs << codec.name;

    return codecs;
}

AkAudioEncoderCodecID AudioEncoderNDKMediaElement::codecID(const QString &codec) const
{
    auto it = std::find_if(this->d->m_codecs.constBegin(),
                           this->d->m_codecs.constEnd(),
                           [&codec] (const CodecInfo &codecInfo) -> bool {
        return codecInfo.name == codec;
    });

    if (it == this->d->m_codecs.constEnd())
        return AkCompressedAudioCaps::AudioCodecID_unknown;

    return it->codecID;
}

QString AudioEncoderNDKMediaElement::codecDescription(const QString &codec) const
{
    auto it = std::find_if(this->d->m_codecs.constBegin(),
                           this->d->m_codecs.constEnd(),
                           [&codec] (const CodecInfo &codecInfo) -> bool {
        return codecInfo.name == codec;
    });

    if (it == this->d->m_codecs.constEnd())
        return {};

    return it->description;
}

AkCompressedAudioCaps AudioEncoderNDKMediaElement::outputCaps() const
{
    return this->d->m_outputCaps;
}

QByteArray AudioEncoderNDKMediaElement::headers() const
{
    return this->d->m_headers;
}

qint64 AudioEncoderNDKMediaElement::encodedTimePts() const
{
    return this->d->m_encodedTimePts;
}

AkPacket AudioEncoderNDKMediaElement::iAudioStream(const AkAudioPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (this->d->m_paused
        || !this->d->m_initialized
        || !this->d->m_fillAudioGaps)
        return {};

    this->d->m_fillAudioGaps->iStream(packet);

    return {};
}

bool AudioEncoderNDKMediaElement::setState(ElementState state)
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

AudioEncoderNDKMediaElementPrivate::AudioEncoderNDKMediaElementPrivate(AudioEncoderNDKMediaElement *self):
    self(self)
{
    if (this->m_fillAudioGaps)
        QObject::connect(this->m_fillAudioGaps.data(),
                         &AkElement::oStream,
                         [this] (const AkPacket &packet) {
                             this->encodeFrame(packet);
                         });

    QObject::connect(self,
                     &AkAudioEncoder::inputCapsChanged,
                     [this] (const AkAudioCaps &inputCaps) {
                        Q_UNUSED(inputCaps)

                        this->updateOutputCaps();
                     });
}

AudioEncoderNDKMediaElementPrivate::~AudioEncoderNDKMediaElementPrivate()
{

}

const char *AudioEncoderNDKMediaElementPrivate::errorToStr(media_status_t status)
{
    static const struct
    {
        media_status_t status;
        const char *str;
    } audioNkmEncErrorsStr[] = {
        {AMEDIACODEC_ERROR_INSUFFICIENT_RESOURCE, "INSUFFICIENT_RESOURCE"        },
        {AMEDIACODEC_ERROR_RECLAIMED            , "ERROR_RECLAIMED"              },
        {AMEDIA_ERROR_BASE                      , "ERROR_BASE"                   },
        {AMEDIA_ERROR_UNKNOWN                   , "ERROR_UNKNOWN"                },
        {AMEDIA_ERROR_MALFORMED                 , "ERROR_MALFORMED"              },
        {AMEDIA_ERROR_UNSUPPORTED               , "ERROR_UNSUPPORTED"            },
        {AMEDIA_ERROR_INVALID_OBJECT            , "ERROR_INVALID_OBJECT"         },
        {AMEDIA_ERROR_INVALID_PARAMETER         , "ERROR_INVALID_PARAMETER"      },
        {AMEDIA_ERROR_INVALID_OPERATION         , "ERROR_INVALID_OPERATION"      },
        {AMEDIA_ERROR_END_OF_STREAM             , "ERROR_END_OF_STREAM"          },
        {AMEDIA_ERROR_IO                        , "ERROR_IO"                     },
        {AMEDIA_ERROR_WOULD_BLOCK               , "ERROR_WOULD_BLOCK"            },
        {AMEDIA_DRM_ERROR_BASE                  , "DRM_ERROR_BASE"               },
        {AMEDIA_DRM_NOT_PROVISIONED             , "DRM_NOT_PROVISIONED"          },
        {AMEDIA_DRM_RESOURCE_BUSY               , "DRM_RESOURCE_BUSY"            },
        {AMEDIA_DRM_DEVICE_REVOKED              , "DRM_DEVICE_REVOKED"           },
        {AMEDIA_DRM_SHORT_BUFFER                , "DRM_SHORT_BUFFER"             },
        {AMEDIA_DRM_SESSION_NOT_OPENED          , "DRM_SESSION_NOT_OPENED"       },
        {AMEDIA_DRM_TAMPER_DETECTED             , "DRM_TAMPER_DETECTED"          },
        {AMEDIA_DRM_VERIFY_FAILED               , "DRM_VERIFY_FAILED"            },
        {AMEDIA_DRM_NEED_KEY                    , "DRM_NEED_KEY"                 },
        {AMEDIA_DRM_LICENSE_EXPIRED             , "DRM_LICENSE_EXPIRED"          },
        {AMEDIA_IMGREADER_ERROR_BASE            , "IMGREADER_ERROR_BASE"         },
        {AMEDIA_IMGREADER_NO_BUFFER_AVAILABLE   , "IMGREADER_NO_BUFFER_AVAILABLE"},
        {AMEDIA_IMGREADER_MAX_IMAGES_ACQUIRED   , "IMGREADER_MAX_IMAGES_ACQUIRED"},
        {AMEDIA_IMGREADER_CANNOT_LOCK_IMAGE     , "IMGREADER_CANNOT_LOCK_IMAGE"  },
        {AMEDIA_IMGREADER_CANNOT_UNLOCK_IMAGE   , "IMGREADER_CANNOT_UNLOCK_IMAGE"},
        {AMEDIA_IMGREADER_IMAGE_NOT_LOCKED      , "IMGREADER_IMAGE_NOT_LOCKED"   },
        {AMEDIA_OK                              , "OK"                           },
    };

    auto errorStatus = audioNkmEncErrorsStr;

    for (; errorStatus->status != AMEDIA_OK; ++errorStatus)
        if (errorStatus->status == status)
            return errorStatus->str;

    return errorStatus->str;
}

QString AudioEncoderNDKMediaElementPrivate::toValidName(const QString &name) const
{
    QString validName;
    QString validChars =
            "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";

    for (auto &c: name)
        if (validChars.contains(c))
            validName += c;
        else
            validName += '_';

    return validName;
}

bool AudioEncoderNDKMediaElementPrivate::isAvailable(const QString &mimeType) const
{
    static struct
    {
        char mimeType[1024];
        bool isAvailable;
    } ndkmediaAudioEncAvailableCodecs[32];
    static size_t ndkmediaAudioEncAvailableCodecsSize = 0;

    for (size_t i = 0; i < ndkmediaAudioEncAvailableCodecsSize; ++i)
        if (ndkmediaAudioEncAvailableCodecs[i].mimeType == mimeType)
            return ndkmediaAudioEncAvailableCodecs[i].isAvailable;

    auto codec = AMediaCodec_createEncoderByType(mimeType.toStdString().c_str());
    bool isAvailable = false;

    if (codec) {
        isAvailable = true;
        AMediaCodec_delete(codec);
    }

    auto i = ndkmediaAudioEncAvailableCodecsSize++;
    qstrncpy(ndkmediaAudioEncAvailableCodecs[i].mimeType,
            mimeType.toStdString().c_str(),
            1024);
    ndkmediaAudioEncAvailableCodecs[i].isAvailable = isAvailable;

    return isAvailable;
}

void AudioEncoderNDKMediaElementPrivate::listCodecs()
{
    QJniEnvironment env;

    // Create MediaCodecList with ALL_CODECS
    auto allCodecsConst =
            QJniObject::getStaticField<jint>("android/media/MediaCodecList",
                                             "ALL_CODECS");

    QJniObject mediaCodecList("android/media/MediaCodecList",
                              "(I)V",
                              allCodecsConst);

    if (!mediaCodecList.isValid()) {
        qWarning() << "Failed to create MediaCodecList";

        return;
    }

    // Get MediaCodecInfo[]
    auto codecInfos =
            mediaCodecList.callObjectMethod("getCodecInfos",
                                            "()[Landroid/media/MediaCodecInfo;");

    if (!codecInfos.isValid()) {
        qWarning() << "Failed to get codec info";

        return;
    }

    qInfo() << "Listing available audio codecs:";

    auto codecArray = static_cast<jobjectArray>(codecInfos.object());
    auto numCodecs = env->GetArrayLength(codecArray);

    for (jsize i = 0; i < numCodecs; ++i) {
        QJniObject codecInfo(env->GetObjectArrayElement(codecArray, i));

        if (!codecInfo.isValid())
            continue;

        // Only list encoders
        if (!codecInfo.callMethod<jboolean>("isEncoder", "()Z"))
            continue;

        // Read the codec name
        auto codecName =
                codecInfo.callObjectMethod("getName", "()Ljava/lang/String;");

        // Read the supported mime types
        auto mimeTypes = codecInfo.callObjectMethod("getSupportedTypes",
                                                    "()[Ljava/lang/String;");

        if (!mimeTypes.isValid())
            continue;

        auto types = static_cast<jobjectArray>(mimeTypes.object());
        auto typesLength = env->GetArrayLength(types);

        for (jsize j = 0; j < typesLength; ++j) {
            QJniObject mimeType(env->GetObjectArrayElement(types, j));

            if (!mimeType.isValid())
                continue;

            auto mimeTypeStr = mimeType.toString();
            auto codec = NDKMediaCodecs::byMimeType(mimeTypeStr);

            if (codec->codecID == AkCompressedAudioCaps::AudioCodecID_unknown)
                continue;

            // Read capabilities
            auto capabilities =
                    codecInfo.callObjectMethod("getCapabilitiesForType",
                                               "(Ljava/lang/String;)"
                                               "Landroid/media/MediaCodecInfo$CodecCapabilities;",
                                               mimeType.object());

            if (!capabilities.isValid())
                continue;

            auto cname = codecName.toString();
            this->m_codecs << CodecInfo {QString("%1_%2").arg(codec->name).arg(this->toValidName(cname)),
                                         QString("%1 (%2)").arg(codec->description).arg(cname),
                                         cname,
                                         codec->codecID,
                                         mimeTypeStr,
                                         QVector<int32_t> {ENCODING_PCM_16BIT}};

            qInfo() << "Codec name:" << this->m_codecs.last().name;
            qInfo() << "Codec description:" << this->m_codecs.last().description;
            qInfo() << "Native codec name:" << this->m_codecs.last().ndkName;
            qInfo() << "Codec ID:" << this->m_codecs.last().codecID;
            qInfo() << "Mime type:" << this->m_codecs.last().mimeType;

            qInfo() << "Supported sample formats:";

            for (auto &fmt: this->m_codecs.last().formats)
                qInfo() << "    "
                        << SampleFormatsTable::byNdkFormat(fmt)->format
                        << "("
                        << SampleFormatsTable::ndkFormatToString(fmt)
                        << ")";

            qInfo() << "";
        }
    }
}

bool AudioEncoderNDKMediaElementPrivate::init()
{
    this->uninit();

    this->m_outputMediaFormat = {};
    auto inputCaps = self->inputCaps();
    qInfo() << "Starting the NDK audio encoder";

    if (!inputCaps) {
        qCritical() << "Invalid input format.";

        return false;
    }

    auto it = std::find_if(this->m_codecs.constBegin(),
                           this->m_codecs.constEnd(),
                           [this] (const CodecInfo &codecInfo) -> bool {
        return codecInfo.name == self->codec();
    });

    if (it == this->m_codecs.constEnd()) {
        qCritical() << "Codec not found:" << self->codec();

        return false;
    }

    this->m_codec =
            AMediaCodec_createCodecByName(it->ndkName.toStdString().c_str());

    if (!this->m_codec) {
        qCritical() << "Encoder not found";

        return false;
    }

    this->m_inputMediaFormat =
            AMediaFormatPtr(AMediaFormat_new(),
                            [] (AMediaFormat *mediaFormat) {
        AMediaFormat_delete(mediaFormat);
    });
    AMediaFormat_setString(this->m_inputMediaFormat.data(),
                           AMEDIAFORMAT_KEY_MIME,
                           it->mimeType.toStdString().c_str());
    AMediaFormat_setInt32(this->m_inputMediaFormat.data(),
                          AMEDIAFORMAT_KEY_BIT_RATE,
                          self->bitrate());
    AMediaFormat_setString(this->m_inputMediaFormat.data(),
                           AMEDIAFORMAT_KEY_LANGUAGE,
                           "und");

#if __ANDROID_API__ >= 28
    AMediaFormat_setInt32(this->m_inputMediaFormat.data(),
                          AMEDIAFORMAT_KEY_PCM_ENCODING,
                          SampleFormatsTable::byFormat(this->m_outputCaps.rawCaps().format())->ndkFormat);
#endif

    int32_t channelMask = this->m_outputCaps.rawCaps().channels() < 1?
                          CHANNEL_MASK_MONO:
                          CHANNEL_MASK_FRONT_LEFT | CHANNEL_MASK_FRONT_RIGHT;
    AMediaFormat_setInt32(this->m_inputMediaFormat.data(),
                          AMEDIAFORMAT_KEY_CHANNEL_MASK,
                          channelMask);
    AMediaFormat_setInt32(this->m_inputMediaFormat.data(),
                          AMEDIAFORMAT_KEY_CHANNEL_COUNT,
                          this->m_outputCaps.rawCaps().channels());
    AMediaFormat_setInt32(this->m_inputMediaFormat.data(),
                          AMEDIAFORMAT_KEY_SAMPLE_RATE,
                          this->m_outputCaps.rawCaps().rate());

#if __ANDROID_API__ >= 28
    AMediaFormat_setInt32(this->m_inputMediaFormat.data(),
                          AMEDIAFORMAT_KEY_BITRATE_MODE,
                          BITRATE_MODE_CBR);
#endif

    auto result =
            AMediaCodec_configure(this->m_codec,
                                  this->m_inputMediaFormat.data(),
                                  nullptr,
                                  nullptr,
                                  AMEDIACODEC_CONFIGURE_FLAG_ENCODE);

    if (result != AMEDIA_OK) {
        qCritical() << "Encoder configuration failed:" << errorToStr(result);

        return false;
    }

    result = AMediaCodec_start(this->m_codec);

    if (result != AMEDIA_OK) {
        qCritical() << "Failed to start the encoding:" << errorToStr(result);

        return false;
    }

    this->m_outputMediaFormat =
            AMediaFormatPtr(AMediaCodec_getOutputFormat(this->m_codec),
                            [] (AMediaFormat *mediaFormat) {
        AMediaFormat_delete(mediaFormat);
    });
    this->updateHeaders();

    if (this->m_fillAudioGaps) {
        this->m_fillAudioGaps->setProperty("fillGaps", self->fillGaps());
        this->m_fillAudioGaps->setProperty("outputSamples",
                                           FRAME_SIZE);
        this->m_fillAudioGaps->setState(AkElement::ElementStatePlaying);
    }

    this->m_encodedTimePts = 0;
    this->m_initialized = true;
    qInfo() << "NDK audio encoder started";

    return true;
}

void AudioEncoderNDKMediaElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;

    if (this->m_fillAudioGaps)
        this->m_fillAudioGaps->setState(AkElement::ElementStateNull);

    if (this->m_codec) {
        auto bufferIndex =
                AMediaCodec_dequeueInputBuffer(this->m_codec, PROCESSING_TIMEOUT);

        if (bufferIndex >= 0) {
            AMediaCodec_queueInputBuffer(this->m_codec,
                                         size_t(bufferIndex),
                                         0,
                                         0,
                                         0,
                                         AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
        }

        bool eos = false;

        while (!eos) {
            AMediaCodecBufferInfo info;
            memset(&info, 0, sizeof(AMediaCodecBufferInfo));
            auto bufferIndex = AMediaCodec_dequeueOutputBuffer(this->m_codec,
                                                               &info,
                                                               PROCESSING_TIMEOUT);

            if (bufferIndex < 0)
                break;

            if (info.flags & AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG)
                continue;

            if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM)
                eos = true;

            size_t bufferSize = 0;
            auto data = AMediaCodec_getOutputBuffer(this->m_codec,
                                                    size_t(bufferIndex),
                                                    &bufferSize);
            this->sendFrame(data, info);
            AMediaCodec_releaseOutputBuffer(this->m_codec,
                                            size_t(bufferIndex),
                                            info.size > 0);
        }

        AMediaCodec_stop(this->m_codec);
    }

    this->m_inputMediaFormat = {};

    if (this->m_codec) {
        AMediaCodec_delete(this->m_codec);
        this->m_codec = nullptr;
    }

    this->m_paused = false;
    this->m_outputMediaFormat = {};
}

void AudioEncoderNDKMediaElementPrivate::updateHeaders()
{
    auto mfptr = qsizetype(this->m_outputMediaFormat.data());
    QByteArray headers(reinterpret_cast<char *>(&mfptr), sizeof(qsizetype));

    if (this->m_headers == headers)
        return;

    this->m_headers = headers;
    emit self->headersChanged(headers);
}

void AudioEncoderNDKMediaElementPrivate::updateOutputCaps()
{
    auto inputCaps = self->inputCaps();

    if (!inputCaps) {
        if (!this->m_outputCaps)
            return;

        this->m_outputCaps = AkCompressedAudioCaps();
        emit self->outputCapsChanged(this->m_outputCaps);

        return;
    }

    auto codecID = self->codecID(self->codec());

    if (codecID == AkCompressedAudioCaps::AudioCodecID_unknown) {
        if (!this->m_outputCaps)
            return;

        this->m_outputCaps = AkCompressedAudioCaps();
        emit self->outputCapsChanged(this->m_outputCaps);

        return;
    }

    auto it = std::find_if(this->m_codecs.constBegin(),
                           this->m_codecs.constEnd(),
                           [this] (const CodecInfo &codec) -> bool {
        return codec.name == self->codec();
    });

    if (it == this->m_codecs.constEnd()) {
        if (!this->m_outputCaps)
            return;

        this->m_outputCaps = AkCompressedAudioCaps();
        emit self->outputCapsChanged(this->m_outputCaps);

        return;
    }

    auto supportedFormats = SampleFormatsTable::formats();
    AkAudioCaps::SampleFormat format =
            supportedFormats.contains(inputCaps.format())?
                inputCaps.format():
                AkAudioCaps::SampleFormat_s16;
    int channels = qBound(1, inputCaps.channels(), 2);
    int rate = inputCaps.rate();
    AkAudioCaps rawCaps(format,
                        AkAudioCaps::defaultChannelLayout(channels),
                        false,
                        rate);
    AkCompressedAudioCaps outputCaps(codecID, rawCaps);

    if (this->m_fillAudioGaps)
        this->m_fillAudioGaps->setProperty("outputCaps",
                                           QVariant::fromValue(rawCaps));

    if (this->m_outputCaps == outputCaps)
        return;

    this->m_outputCaps = outputCaps;
    emit self->outputCapsChanged(outputCaps);
}

void AudioEncoderNDKMediaElementPrivate::encodeFrame(const AkAudioPacket &src)
{
    this->m_id = src.id();
    this->m_index = src.index();

    // Write the current frame.
    auto bufferIndex =
            AMediaCodec_dequeueInputBuffer(this->m_codec, PROCESSING_TIMEOUT);

    if (bufferIndex >= 0) {
        size_t bufferSize = 0;
        auto buffer = AMediaCodec_getInputBuffer(this->m_codec,
                                                 size_t(bufferIndex),
                                                 &bufferSize);
        memcpy(buffer,
               src.constData(),
               qMin(src.size(), bufferSize));
        uint64_t presentationTimeUs =
                qRound64(1e6 * src.pts() * src.timeBase().value());

        AMediaCodec_queueInputBuffer(this->m_codec,
                                     size_t(bufferIndex),
                                     0,
                                     bufferSize,
                                     presentationTimeUs,
                                     0);
    }

    forever {
        AMediaCodecBufferInfo info;
        memset(&info, 0, sizeof(AMediaCodecBufferInfo));
        auto bufferIndex = AMediaCodec_dequeueOutputBuffer(this->m_codec,
                                                           &info,
                                                           PROCESSING_TIMEOUT);

        if (bufferIndex < 0)
            break;

        if (info.flags & AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG){
            qDebug() << "Audio codec media format changed";
            this->m_outputMediaFormat =
                    AMediaFormatPtr(AMediaCodec_getOutputFormat(this->m_codec),
                                    [] (AMediaFormat *mediaFormat) {
                AMediaFormat_delete(mediaFormat);
            });
            this->updateHeaders();
            AMediaCodec_releaseOutputBuffer(this->m_codec,
                                            size_t(bufferIndex),
                                            false);

            continue;
        }

        if (!this->m_outputMediaFormat) {
            this->m_outputMediaFormat =
                    AMediaFormatPtr(AMediaCodec_getOutputFormat(this->m_codec),
                                    [] (AMediaFormat *mediaFormat) {
                AMediaFormat_delete(mediaFormat);
            });
            this->updateHeaders();
        }

        size_t bufferSize = 0;
        auto data = AMediaCodec_getOutputBuffer(this->m_codec,
                                                size_t(bufferIndex),
                                                &bufferSize);
        this->sendFrame(data, info);
        AMediaCodec_releaseOutputBuffer(this->m_codec,
                                        size_t(bufferIndex),
                                        info.size > 0);
    }

    this->m_encodedTimePts = src.pts() + src.duration();
    emit self->encodedTimePtsChanged(this->m_encodedTimePts);
}

void AudioEncoderNDKMediaElementPrivate::sendFrame(const uint8_t *data,
                                                   const AMediaCodecBufferInfo &info) const
{
    AkCompressedAudioPacket packet(this->m_outputCaps, info.size);
    memcpy(packet.data(), data, packet.size());
    packet.setFlags(info.flags & 1?
                        AkCompressedAudioPacket::AudioPacketTypeFlag_KeyFrame:
                        AkCompressedAudioPacket::AudioPacketTypeFlag_None);
    qint64 pts = qRound64(info.presentationTimeUs
                          * this->m_outputCaps.rawCaps().rate()
                          / 1e6);
    packet.setPts(pts);
    packet.setDts(pts);
    packet.setDuration(FRAME_SIZE);
    packet.setTimeBase({1, this->m_outputCaps.rawCaps().rate()});
    packet.setId(this->m_id);
    packet.setIndex(this->m_index);
    packet.setExtraData({reinterpret_cast<const char *>(&info),
                         sizeof(AMediaCodecBufferInfo)});

    emit self->oStream(packet);
}

#include "moc_audioencoderndkmediaelement.cpp"
