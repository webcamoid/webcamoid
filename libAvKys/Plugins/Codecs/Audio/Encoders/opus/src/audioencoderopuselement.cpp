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

#include <QCoreApplication>
#include <QMutex>
#include <QQmlContext>
#include <QThread>
#include <QVariant>
#include <QtEndian>
#include <akfrac.h>
#include <akpacket.h>
#include <akaudiocaps.h>
#include <akcompressedaudiocaps.h>
#include <akaudioconverter.h>
#include <akaudiopacket.h>
#include <akcompressedaudiopacket.h>
#include <akpluginmanager.h>
#include <iak/akelement.h>
#include <opus.h>

#include "audioencoderopuselement.h"

class AudioEncoderOpusElementPrivate
{
    public:
        AudioEncoderOpusElement *self;
        AudioEncoderOpusElement::ApplicationType m_applicationType {AudioEncoderOpusElement::ApplicationType_Audio};
        AkCompressedAudioCaps m_outputCaps;
        AkCompressedAudioPackets m_headers;
        OpusEncoder *m_encoder {nullptr};
        QMutex m_mutex;
        qint64 m_id {0};
        int m_index {0};
        bool m_initialized {false};
        bool m_paused {false};
        qint64 m_pts {0};
        qint64 m_encodedTimePts {0};
        AkElementPtr m_fillAudioGaps {akPluginManager->create<AkElement>("AudioFilter/FillAudioGaps")};

        explicit AudioEncoderOpusElementPrivate(AudioEncoderOpusElement *self);
        ~AudioEncoderOpusElementPrivate();
        static int opusApplication(AudioEncoderOpusElement::ApplicationType applicationType);
        static int nearestSampleRate(int rate);
        bool init();
        void uninit();
        void updateHeaders();
        void updateOutputCaps(const AkAudioCaps &inputCaps);
        void encodeFrame(const AkAudioPacket &src);
        void sendFrame(const QByteArray &data, opus_int32 writtenBytes);
};

AudioEncoderOpusElement::AudioEncoderOpusElement():
    AkAudioEncoder()
{
    this->d = new AudioEncoderOpusElementPrivate(this);
}

AudioEncoderOpusElement::~AudioEncoderOpusElement()
{
    this->d->uninit();
    delete this->d;
}

AkAudioEncoderCodecID AudioEncoderOpusElement::codec() const
{
    return AkCompressedAudioCaps::AudioCodecID_opus;
}

AkCompressedAudioCaps AudioEncoderOpusElement::outputCaps() const
{
    return this->d->m_outputCaps;
}

AkCompressedPackets AudioEncoderOpusElement::headers() const
{
    AkCompressedPackets packets;

    for (auto &header: this->d->m_headers)
        packets << header;

    return packets;
}

qint64 AudioEncoderOpusElement::encodedTimePts() const
{
    return this->d->m_encodedTimePts;
}

AudioEncoderOpusElement::ApplicationType AudioEncoderOpusElement::applicationType() const
{
    return this->d->m_applicationType;
}

QString AudioEncoderOpusElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/AudioEncoderOpus/share/qml/main.qml");
}

void AudioEncoderOpusElement::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("AudioEncoderOpus", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket AudioEncoderOpusElement::iAudioStream(const AkAudioPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (this->d->m_paused
        || !this->d->m_initialized
        || !this->d->m_fillAudioGaps)
        return {};

    this->d->m_fillAudioGaps->iStream(packet);

    return {};
}

void AudioEncoderOpusElement::setApplicationType(ApplicationType applicationType)
{
    if (applicationType == this->d->m_applicationType)
        return;

    this->d->m_applicationType = applicationType;
    emit this->applicationTypeChanged(applicationType);
}

void AudioEncoderOpusElement::resetApplicationType()
{
    this->setApplicationType(ApplicationType_Audio);
}

bool AudioEncoderOpusElement::setState(ElementState state)
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

AudioEncoderOpusElementPrivate::AudioEncoderOpusElementPrivate(AudioEncoderOpusElement *self):
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

AudioEncoderOpusElementPrivate::~AudioEncoderOpusElementPrivate()
{

}

int AudioEncoderOpusElementPrivate::opusApplication(AudioEncoderOpusElement::ApplicationType applicationType)
{
    struct AppType
    {
        AudioEncoderOpusElement::ApplicationType type;
        int opusType;
    };

    static const AppType appTypes[] = {
        {AudioEncoderOpusElement::ApplicationType_Voip              , OPUS_APPLICATION_VOIP               },
        {AudioEncoderOpusElement::ApplicationType_Audio             , OPUS_APPLICATION_AUDIO              },
        {AudioEncoderOpusElement::ApplicationType_RestrictedLowdelay, OPUS_APPLICATION_RESTRICTED_LOWDELAY},
        {AudioEncoderOpusElement::ApplicationType_Unknown           , 0                                   },
    };

    for (auto appType = appTypes; appType->type != AudioEncoderOpusElement::ApplicationType_Unknown; ++appType)
        if (appType->type == applicationType)
            return appType->opusType;

    return OPUS_APPLICATION_AUDIO;
}

int AudioEncoderOpusElementPrivate::nearestSampleRate(int rate)
{
   static const int supportedSampleRates[] = {
       8000,
       12000,
       16000,
       24000,
       48000,
       0
   };

   int nearest = rate;
   quint64 q = std::numeric_limits<quint64>::max();

   for (auto srate = supportedSampleRates; *srate; ++srate) {
       quint64 k = qAbs(*srate - rate);

       if (k < q) {
           nearest = *srate;
           q = k;
       }
   }

   return nearest;
}

bool AudioEncoderOpusElementPrivate::init()
{
    this->uninit();

    auto inputCaps = self->inputCaps();

    if (!inputCaps) {
        qCritical() << "Invalid input format.";

        return false;
    }

    int error = 0;
    this->m_encoder =
            opus_encoder_create(this->m_outputCaps.rawCaps().rate(),
                                this->m_outputCaps.rawCaps().channels(),
                                opusApplication(this->m_applicationType),
                                &error);

    if (error < 0) {
        qCritical() << opus_strerror(error);

        return false;
    }

    auto bitrate = qBound(6000, self->bitrate(), 510000);
    error = opus_encoder_ctl(this->m_encoder, OPUS_SET_BITRATE(bitrate));

    if (error < 0) {
        qCritical() << opus_strerror(error);
        opus_encoder_destroy(this->m_encoder);

        return false;
    }

    this->updateHeaders();

    if (this->m_fillAudioGaps) {
        this->m_fillAudioGaps->setProperty("fillGaps", self->fillGaps());
        this->m_fillAudioGaps->setProperty("outputSamples",
                                           this->m_outputCaps.rawCaps().rate() / 50);
        this->m_fillAudioGaps->setState(AkElement::ElementStatePlaying);
    }

    this->m_pts = 0;
    this->m_encodedTimePts = 0;
    this->m_initialized = true;

    return true;
}

void AudioEncoderOpusElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;

    if (this->m_fillAudioGaps)
        this->m_fillAudioGaps->setState(AkElement::ElementStateNull);

    if (this->m_encoder) {
        opus_encoder_destroy(this->m_encoder);
        this->m_encoder = nullptr;
    }

    this->m_paused = false;
}

void AudioEncoderOpusElementPrivate::updateHeaders()
{
    // https://wiki.xiph.org/OggOpus

    QByteArray oggOpusHeader;
    QDataStream ds(&oggOpusHeader, QIODeviceBase::WriteOnly);
    ds.writeRawData("OpusHead", 8); // Magic signature
    ds << quint8(1);  // Version number
    ds << quint8(this->m_outputCaps.rawCaps().channels()); // Channels
    opus_int32 preSkip = 0;
    opus_encoder_ctl(this->m_encoder, OPUS_GET_LOOKAHEAD(&preSkip));
    ds << quint16_le(48000 * preSkip / this->m_outputCaps.rawCaps().rate()); // Pre-skip
    ds << quint32_le(this->m_outputCaps.rawCaps().rate());
    ds << qint16_le(0); // Output gain
    ds << quint8(0); /* Channel mapping family
                      * (only mono and stereo are
                      *  supported)
                      */

    AkCompressedAudioPacket headerPacket(this->m_outputCaps,
                                         oggOpusHeader.size());
    memcpy(headerPacket.data(),
           oggOpusHeader.constData(),
           headerPacket.size());
    headerPacket.setTimeBase({1, this->m_outputCaps.rawCaps().rate()});
    headerPacket.setFlags(AkCompressedAudioPacket::AudioPacketTypeFlag_Header);
    this->m_headers = {headerPacket};
    emit self->headersChanged(self->headers());
}

void AudioEncoderOpusElementPrivate::updateOutputCaps(const AkAudioCaps &inputCaps)
{
    if (!inputCaps) {
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
    AkCompressedAudioCaps outputCaps(self->codec(), rawCaps);

    if (this->m_fillAudioGaps)
        this->m_fillAudioGaps->setProperty("outputCaps",
                                           QVariant::fromValue(rawCaps));

    if (this->m_outputCaps == outputCaps)
        return;

    this->m_outputCaps = outputCaps;
    emit self->outputCapsChanged(outputCaps);
}

void AudioEncoderOpusElementPrivate::encodeFrame(const AkAudioPacket &src)
{
    this->m_id = src.id();
    this->m_index = src.index();

    QByteArray packetData(2 * src.size(), 0);
    auto writtenBytes =
            opus_encode(this->m_encoder,
                        reinterpret_cast<const opus_int16 *>(src.constData()),
                        src.samples(),
                        reinterpret_cast<unsigned char *>(packetData.data()),
                        packetData.size());

    if (writtenBytes < 0) {
        qCritical() << opus_strerror(writtenBytes);

        return;
    } else if (writtenBytes > 0) {
        this->sendFrame(packetData, writtenBytes);
    }

    this->m_encodedTimePts += src.samples();
    emit self->encodedTimePtsChanged(this->m_encodedTimePts);
}

void AudioEncoderOpusElementPrivate::sendFrame(const QByteArray &data,
                                               opus_int32 writtenBytes)
{
    auto samples =
            opus_packet_get_nb_samples(reinterpret_cast<const unsigned char *>(data.constData()),
                                                                               writtenBytes,
                                                                               this->m_outputCaps.rawCaps().rate());

    AkCompressedAudioPacket packet(this->m_outputCaps, writtenBytes);
    memcpy(packet.data(), data.constData(), packet.size());
    packet.setPts(this->m_pts);
    packet.setDts(this->m_pts);
    packet.setDuration(samples);
    packet.setTimeBase({1, this->m_outputCaps.rawCaps().rate()});
    packet.setId(this->m_id);
    packet.setIndex(this->m_index);

    emit self->oStream(packet);
    this->m_pts += samples;
}

#include "moc_audioencoderopuselement.cpp"
