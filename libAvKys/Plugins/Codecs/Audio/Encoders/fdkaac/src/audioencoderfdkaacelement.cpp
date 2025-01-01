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

#include <QBitArray>
#include <QCoreApplication>
#include <QMutex>
#include <QQmlContext>
#include <QVariant>
#include <akfrac.h>
#include <akpacket.h>
#include <akaudiocaps.h>
#include <akcompressedaudiocaps.h>
#include <akaudiopacket.h>
#include <akcompressedaudiopacket.h>
#include <akpluginmanager.h>
#include <iak/akelement.h>
#include <fdk-aac/aacenc_lib.h>

#include "audioencoderfdkaacelement.h"

#define AAC_BITRATEMODE_CBR      0 // Constant bitrate
#define AAC_BITRATEMODE_VBR_VLBR 1 // Variable bitrate, very low bitrate
#define AAC_BITRATEMODE_VBR_LBR  2 // Variable bitrate, low bitrate
#define AAC_BITRATEMODE_VBR_MBR  3 // Variable bitrate, medium bitrate
#define AAC_BITRATEMODE_VBR_HBR  4 // Variable bitrate, high bitrate
#define AAC_BITRATEMODE_VBR_VHBR 5 // Variable bitrate, very high bitrate

class AudioEncoderFdkAacElementPrivate
{
    public:
        AudioEncoderFdkAacElement *self;
        bool m_errorResilient {false};
        AudioEncoderFdkAacElement::OutputFormat m_outputFormat {AudioEncoderFdkAacElement::OutputFormat_Raw};
        AkCompressedAudioCaps m_outputCaps;
        AkCompressedAudioPackets m_headers;
        HANDLE_AACENCODER m_encoder {nullptr};
        AACENC_InfoStruct m_info;
        AACENC_BufDesc m_outBuffer;
        AACENC_InArgs m_inArgs;
        AACENC_OutArgs m_outArgs;
        QByteArray m_packetData;
        QMutex m_mutex;
        bool m_initialized {false};
        bool m_paused {false};
        qint64 m_id {0};
        int m_index {0};
        qint64 m_pts {0};
        qint64 m_encodedTimePts {0};
        AkElementPtr m_fillAudioGaps {akPluginManager->create<AkElement>("AudioFilter/FillAudioGaps")};

        explicit AudioEncoderFdkAacElementPrivate(AudioEncoderFdkAacElement *self);
        ~AudioEncoderFdkAacElementPrivate();
        static const char *errorToString(int error);
        static int nearestSampleRate(int rate);
        static int sampleRateIndex(int rate);
        static void putBits(QBitArray &ba, qsizetype bits, quint32 value);
        static QByteArray bitsToByteArray(const QBitArray &bits);
        bool init();
        void uninit();
        void updateHeaders();
        void updateOutputCaps(const AkAudioCaps &inputCaps);
        void encodeFrame(const AkAudioPacket &src);
        void sendFrame(const QByteArray &packetData,
                       qsizetype samples,
                       qsizetype writtenBytes);
};

AudioEncoderFdkAacElement::AudioEncoderFdkAacElement():
    AkAudioEncoder()
{
    this->d = new AudioEncoderFdkAacElementPrivate(this);
    this->setCodec(this->codecs().value(0));
}

AudioEncoderFdkAacElement::~AudioEncoderFdkAacElement()
{
    this->d->uninit();
    delete this->d;
}

QStringList AudioEncoderFdkAacElement::codecs() const
{
    return {"fdkaac"};
}

AkAudioEncoderCodecID AudioEncoderFdkAacElement::codecID(const QString &codec) const
{
    return codec == this->codecs().first()?
                AkCompressedAudioCaps::AudioCodecID_aac:
                AkCompressedAudioCaps::AudioCodecID_unknown;
}

QString AudioEncoderFdkAacElement::codecDescription(const QString &codec) const
{
    return codec == this->codecs().first()?
                QStringLiteral("AAC (fdk-aac)"):
                QString();
}

AkCompressedAudioCaps AudioEncoderFdkAacElement::outputCaps() const
{
    return this->d->m_outputCaps;
}

AkCompressedPackets AudioEncoderFdkAacElement::headers() const
{
    AkCompressedPackets packets;

    for (auto &header: this->d->m_headers)
        packets << header;

    return packets;
}

qint64 AudioEncoderFdkAacElement::encodedTimePts() const
{
    return this->d->m_encodedTimePts;
}

bool AudioEncoderFdkAacElement::errorResilient() const
{
    return this->d->m_errorResilient;
}

AudioEncoderFdkAacElement::OutputFormat AudioEncoderFdkAacElement::outputFormat() const
{
    return this->d->m_outputFormat;
}

QString AudioEncoderFdkAacElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/AudioEncoderFdkAac/share/qml/main.qml");
}

void AudioEncoderFdkAacElement::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("AudioEncoderFdkAac", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket AudioEncoderFdkAacElement::iAudioStream(const AkAudioPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (this->d->m_paused
        || !this->d->m_initialized
        || !this->d->m_fillAudioGaps)
        return {};

    this->d->m_fillAudioGaps->iStream(packet);

    return {};
}

void AudioEncoderFdkAacElement::setErrorResilient(bool errorResilient)
{
    if (errorResilient == this->d->m_errorResilient)
        return;

    this->d->m_errorResilient = errorResilient;
    emit this->errorResilientChanged(errorResilient);
}

void AudioEncoderFdkAacElement::setOutputFormat(OutputFormat outputFormat)
{
    if (outputFormat == this->d->m_outputFormat)
        return;

    this->d->m_outputFormat = outputFormat;
    emit this->outputFormatChanged(outputFormat);
}

void AudioEncoderFdkAacElement::resetErrorResilient()
{
    this->setErrorResilient(false);
}

void AudioEncoderFdkAacElement::resetOutputFormat()
{
    this->setOutputFormat(OutputFormat_Raw);
}

bool AudioEncoderFdkAacElement::setState(ElementState state)
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

AudioEncoderFdkAacElementPrivate::AudioEncoderFdkAacElementPrivate(AudioEncoderFdkAacElement *self):
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
                         this->updateOutputCaps(inputCaps);
                     });
}

AudioEncoderFdkAacElementPrivate::~AudioEncoderFdkAacElementPrivate()
{

}

const char *AudioEncoderFdkAacElementPrivate::errorToString(int error)
{
    static const struct ErrorCodesStr
    {
        AACENC_ERROR code;
        const char *str;
    } fdkaacEncErrorCodes[] = {
        {AACENC_INVALID_HANDLE       , "Invalid handle"                      },
        {AACENC_MEMORY_ERROR         , "Memory allocation failed"            },
        {AACENC_UNSUPPORTED_PARAMETER, "Unsupported parameter"               },
        {AACENC_INVALID_CONFIG       , "Invalid Configuration"               },
        {AACENC_INIT_ERROR           , "Error in initialization"             },
        {AACENC_INIT_AAC_ERROR       , "Error initializing AAC library"      },
        {AACENC_INIT_SBR_ERROR       , "Error initializing SBR library"      },
        {AACENC_INIT_TP_ERROR        , "Error initializing Transport library"},
        {AACENC_INIT_META_ERROR      , "Error initializing Meta data library"},
        {AACENC_INIT_MPS_ERROR       , "Error initializing MPS library"      },
        {AACENC_ENCODE_ERROR         , "Unexpected error"                    },
        {AACENC_ENCODE_EOF           , "End of file"                         },
        {AACENC_OK                   , "No error"                            },
    };

    auto ec = fdkaacEncErrorCodes;

    for (; ec->code != AACENC_OK; ++ec)
        if (ec->code == error)
            return ec->str;

    if (ec->code == AACENC_OK)
        return ec->str;

    static char fdkaacEncErrorStr[1024];
    snprintf(fdkaacEncErrorStr, 1024, "%d", error);

    return fdkaacEncErrorStr;
}

int AudioEncoderFdkAacElementPrivate::nearestSampleRate(int rate)
{
    static const int fdkaacEncSupportedSampleRates[] = {
        8000,
        11025,
        12000,
        16000,
        22050,
        24000,
        32000,
        44100,
        48000,
        64000,
        88200,
        96000,
        0
    };

    int nearest = rate;
    quint64 q = std::numeric_limits<quint64>::max();

    for (auto srate = fdkaacEncSupportedSampleRates; *srate; ++srate) {
        quint64 k = qAbs(*srate - rate);

        if (k < q) {
            nearest = *srate;
            q = k;
        }
    }

    return nearest;
}

int AudioEncoderFdkAacElementPrivate::sampleRateIndex(int rate)
{
    static const int fdkaacEncSampleRateIndex[] = {
        96000,
        88200,
        64000,
        48000,
        44100,
        32000,
        24000,
        22050,
        16000,
        12000,
        11025,
        8000,
        7350,
        0
    };

    for (int i = 0; fdkaacEncSampleRateIndex[i]; ++i)
        if (fdkaacEncSampleRateIndex[i] == rate)
            return i;

    return 15;
}

void AudioEncoderFdkAacElementPrivate::putBits(QBitArray &ba,
                                             qsizetype bits,
                                             quint32 value)
{
    ba.resize(ba.size() + bits);

    for (qsizetype i = 0; i < bits; ++i)
        ba[ba.size() - i - 1] = (value >> i) & 0x1;
}

QByteArray AudioEncoderFdkAacElementPrivate::bitsToByteArray(const QBitArray &bits)
{
    QByteArray bytes((bits.size() + 7) / 8, 0);

    for (int i = 0; i < bits.size(); ++i)
        bytes[i / 8] |= (bits[i]? 1: 0) << (7 - (i % 8));

    return bytes;
}

bool AudioEncoderFdkAacElementPrivate::init()
{
    this->uninit();

    auto inputCaps = self->inputCaps();

    if (!inputCaps) {
        qCritical() << "Invalid input format.";

        return false;
    }

    auto result = aacEncOpen(&this->m_encoder, 0, inputCaps.channels());

    if (result != AACENC_OK) {
        qCritical() << "Failed to open encoder:" << errorToString(result);

        return false;
    }

    auto rate = UINT(nearestSampleRate(inputCaps.rate()));

    struct ParamValue
    {
        AACENC_PARAM param;
        const char *name;
        UINT value;
    } paramValues[] = {
        {AACENC_AOT              , "AACENC_AOT"           , AOT_AAC_LC                },
        {AACENC_BITRATE          , "AACENC_BITRATE"       , UINT(self->bitrate())     },
        {AACENC_BITRATEMODE      , "AACENC_BITRATEMODE"   , AAC_BITRATEMODE_CBR       },
        {AACENC_SAMPLERATE       , "AACENC_SAMPLERATE"    , rate                      },
        {AACENC_CHANNELMODE      , "AACENC_CHANNELMODE"   , UINT(inputCaps.channels())},
        {AACENC_TRANSMUX         , "AACENC_TRANSMUX"      , UINT(this->m_outputFormat)},
        {AACENC_PROTECTION       , "AACENC_PROTECTION"    , this->m_errorResilient    },
        {AACENC_NONE             , ""                     , 0                         },
    };
    auto param = paramValues;

    for (; param->param != AACENC_NONE; ++param) {
        result = aacEncoder_SetParam(this->m_encoder, param->param, param->value);

        if (result != AACENC_OK) {
            qCritical() << "Failed to set" << param->name << "parameter:" << errorToString(result);

            return false;
        }
    }

    result = aacEncEncode(this->m_encoder, nullptr, nullptr, nullptr, nullptr);

    if (result != AACENC_OK) {
        qCritical() << "Unable to initialize the encoder:" << errorToString(result);

        return false;
    }

    result = aacEncInfo(this->m_encoder, &this->m_info);

    if (result != AACENC_OK) {
        qCritical() << "Unable to get encoder info:" << errorToString(result);

        return false;
    }

    this->m_packetData =
            QByteArray(this->m_info.maxOutBufBytes, Qt::Uninitialized);

    memset(&this->m_outBuffer, 0, sizeof(AACENC_BufDesc));
    this->m_outBuffer.numBufs = 1;
    this->m_outBuffer.bufs = new void *[] {this->m_packetData.data()};
    this->m_outBuffer.bufferIdentifiers = new INT[] {OUT_BITSTREAM_DATA};
    this->m_outBuffer.bufSizes = new INT[] {INT(this->m_packetData.size())};
    this->m_outBuffer.bufElSizes = new INT[] {1};

    memset(&this->m_inArgs, 0, sizeof(AACENC_InArgs));
    this->m_inArgs.numInSamples = this->m_info.inputChannels * this->m_info.frameLength;

    memset(&this->m_outArgs, 0, sizeof(AACENC_OutArgs));

    this->updateHeaders();

    if (this->m_fillAudioGaps) {
        this->m_fillAudioGaps->setProperty("fillGaps", self->fillGaps());
        this->m_fillAudioGaps->setProperty("outputSamples",
                                           this->m_info.frameLength);
        this->m_fillAudioGaps->setState(AkElement::ElementStatePlaying);
    }

    this->m_pts = 0;
    this->m_encodedTimePts = 0;
    this->m_initialized = true;

    return true;
}

void AudioEncoderFdkAacElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;

    if (this->m_fillAudioGaps)
        this->m_fillAudioGaps->setState(AkElement::ElementStateNull);

    if (this->m_outBuffer.bufs) {
        delete [] this->m_outBuffer.bufs;
        this->m_outBuffer.bufs = nullptr;
    }

    if (this->m_outBuffer.bufferIdentifiers) {
        delete [] this->m_outBuffer.bufferIdentifiers;
        this->m_outBuffer.bufferIdentifiers = nullptr;
    }

    if (this->m_outBuffer.bufSizes) {
        delete [] this->m_outBuffer.bufSizes;
        this->m_outBuffer.bufSizes = nullptr;
    }

    if (this->m_outBuffer.bufElSizes) {
        delete [] this->m_outBuffer.bufElSizes;
        this->m_outBuffer.bufElSizes = nullptr;
    }

    if (this->m_encoder) {
        aacEncClose(&this->m_encoder);
        this->m_encoder = nullptr;
    }

    this->m_packetData = {};
    this->m_paused = false;
}

void AudioEncoderFdkAacElementPrivate::updateHeaders()
{
    // https://wiki.multimedia.cx/index.php/MPEG-4_Audio
    // https://csclub.uwaterloo.ca/~pbarfuss/ISO14496-3-2009.pdf
    // https://learn.microsoft.com/es-es/windows/win32/medfound/aac-decoder

    QBitArray audioSpecificConfig;

    // Set audio specific config
    auto aacObjectType = aacEncoder_GetParam(this->m_encoder, AACENC_AOT);
    putBits(audioSpecificConfig, 5, aacObjectType);
    auto sri = sampleRateIndex(this->m_outputCaps.rawCaps().rate());
    putBits(audioSpecificConfig, 4, sri);

    if (sri >= 15)
        putBits(audioSpecificConfig, 24, this->m_outputCaps.rawCaps().rate());

    putBits(audioSpecificConfig, 4, this->m_outputCaps.rawCaps().channels());

    // Set GASpecificConfig
    putBits(audioSpecificConfig, 1, 0); //frame length - 1024 samples
    putBits(audioSpecificConfig, 1, 0); //does not depend on core coder
    putBits(audioSpecificConfig, 1, 0); //is not extension

    // Disable SBR
    putBits(audioSpecificConfig, 11, 0x2b7);
    putBits(audioSpecificConfig, 5, 5);
    putBits(audioSpecificConfig, 1, 0);

    audioSpecificConfig.resize(32 * 8);
    auto header = bitsToByteArray(audioSpecificConfig);

    AkCompressedAudioPacket headerPacket(this->m_outputCaps,
                                         header.size());
    memcpy(headerPacket.data(),
           header.constData(),
           headerPacket.size());
    headerPacket.setTimeBase({1, this->m_outputCaps.rawCaps().rate()});
    headerPacket.setFlags(AkCompressedAudioPacket::AudioPacketTypeFlag_Header);
    this->m_headers = {headerPacket};
    emit self->headersChanged(self->headers());
}

void AudioEncoderFdkAacElementPrivate::updateOutputCaps(const AkAudioCaps &inputCaps)
{
    if (!inputCaps) {
        if (!this->m_outputCaps)
            return;

        this->m_outputCaps = {};
        emit self->outputCapsChanged({});

        return;
    }

    auto codecID = self->codecID(self->codec());

    if (codecID == AkCompressedAudioCaps::AudioCodecID_unknown) {
        if (!this->m_outputCaps)
            return;

        this->m_outputCaps = {};
        emit self->outputCapsChanged({});

        return;
    }

    int channels = qBound(1, inputCaps.channels(), 2);
    int rate = nearestSampleRate(inputCaps.rate());
    AkAudioCaps rawCaps(AkAudioCaps::SampleFormat_s16,
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

void AudioEncoderFdkAacElementPrivate::encodeFrame(const AkAudioPacket &src)
{
    if (!src)
        return;

    this->m_id = src.id();
    this->m_index = src.index();

    AACENC_BufDesc inBuffer;
    memset(&inBuffer, 0, sizeof(AACENC_BufDesc));
    inBuffer.numBufs = 1;
    inBuffer.bufs = new void *[] {const_cast<char *>(src.constData())};
    inBuffer.bufferIdentifiers = new INT[] {IN_AUDIO_DATA};
    inBuffer.bufSizes = new INT[] {INT(src.size())};
    inBuffer.bufElSizes = new INT[] {src.caps().bps() / 8};
    auto result = aacEncEncode(this->m_encoder,
                               &inBuffer,
                               &this->m_outBuffer,
                               &this->m_inArgs,
                               &this->m_outArgs);
    delete [] inBuffer.bufs;
    delete [] inBuffer.bufferIdentifiers;
    delete [] inBuffer.bufSizes;
    delete [] inBuffer.bufElSizes;

    if (result != AACENC_OK) {
        qCritical() << "Failed encoding the samples:" << errorToString(result);

        return;
    } else if (this->m_outArgs.numOutBytes > 0) {
        this->sendFrame(this->m_packetData,
                        src.samples(),
                        this->m_outArgs.numOutBytes);
    }

    this->m_encodedTimePts += src.samples();
    emit self->encodedTimePtsChanged(this->m_encodedTimePts);
}

void AudioEncoderFdkAacElementPrivate::sendFrame(const QByteArray &packetData,
                                               qsizetype samples,
                                               qsizetype writtenBytes)
{
    AkCompressedAudioPacket packet(this->m_outputCaps, writtenBytes);
    memcpy(packet.data(), packetData.constData(), packet.size());
    packet.setPts(this->m_pts);
    packet.setDts(this->m_pts);
    packet.setDuration(samples);
    packet.setTimeBase({1, this->m_outputCaps.rawCaps().rate()});
    packet.setId(this->m_id);
    packet.setIndex(this->m_index);

    emit self->oStream(packet);

    this->m_pts += samples;
}

#include "moc_audioencoderfdkaacelement.cpp"
