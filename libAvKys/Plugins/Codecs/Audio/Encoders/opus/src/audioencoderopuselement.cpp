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
#include <opus.h>

#include "audioencoderopuselement.h"

class AudioEncoderOpusElementPrivate
{
    public:
        AudioEncoderOpusElement *self;
        AkAudioConverter m_audioConverter;
        AudioEncoderOpusElement::ApplicationType m_applicationType {AudioEncoderOpusElement::ApplicationType_Audio};
        AkCompressedAudioCaps m_outputCaps;
        AkCompressedAudioPackets m_headers;
        OpusEncoder *m_encoder {nullptr};
        QByteArray m_audioBuffer;
        QMutex m_mutex;
        bool m_initialized {false};
        qint64 m_pts {0};

        explicit AudioEncoderOpusElementPrivate(AudioEncoderOpusElement *self);
        ~AudioEncoderOpusElementPrivate();
        static int opusApplication(AudioEncoderOpusElement::ApplicationType applicationType);
        static int nearestSampleRate(int rate);
        QByteArray readBuffer(int *readSamples);
        bool init();
        void uninit();
        void updateHeaders();
        void updateOutputCaps(const AkAudioCaps &inputCaps);
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

    if (!this->d->m_initialized)
        return {};

    auto src = this->d->m_audioConverter.convert(packet);

    if (!src)
        return {};

    qsizetype bufferSize =
            src.caps().bps() * src.caps().channels() * src.samples() / 8;
    this->d->m_audioBuffer += QByteArray(src.constData(),
                                         qMin<qsizetype>(bufferSize, src.size()));

    forever {
        int samples = 0;
        auto buffer = this->d->readBuffer(&samples);

        if (buffer.isEmpty())
            break;

        QByteArray packetData(2 * buffer.size(), 0);
        auto writtenBytes = opus_encode(this->d->m_encoder,
                                        reinterpret_cast<const opus_int16 *>(buffer.constData()),
                                        samples,
                                        reinterpret_cast<unsigned char *>(packetData.data()),
                                        packetData.size());

        if (writtenBytes < 0) {
            qCritical() << opus_strerror(writtenBytes);

            continue;
        } else if (writtenBytes > 0) {
            AkCompressedAudioPacket packet(this->d->m_outputCaps, writtenBytes);
            memcpy(packet.data(), packetData.constData(), packet.size());
            packet.setPts(this->d->m_pts);
            packet.setDts(this->d->m_pts);
            packet.setDuration(samples);
            packet.setTimeBase({1, this->d->m_outputCaps.rate()});
            packet.setId(src.id());
            packet.setIndex(src.index());

            emit this->oStream(packet);

            this->d->m_pts += samples;
        }
    }

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

AudioEncoderOpusElementPrivate::AudioEncoderOpusElementPrivate(AudioEncoderOpusElement *self):
    self(self)
{
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

QByteArray AudioEncoderOpusElementPrivate::readBuffer(int *readSamples)
{
    struct SampleDuration
    {
        int num;
        int den;
    };
    static const SampleDuration durations[] = {
        {5 , 2},
        {5 , 1},
        {10, 1},
        {20, 1},
        {40, 1},
        {60, 1},
        {0 , 0},
    };
    static qsizetype nDurations = 0;

    if (nDurations < 1)
        for (auto duration = durations; duration->num; ++duration)
            nDurations++;

    auto caps = this->m_audioConverter.outputCaps();
    auto channels = caps.channels();
    auto rate = caps.rate();
    auto bps = caps.bps();
    auto totalSamples =
            8 * this->m_audioBuffer.size() / (bps * channels);

    for (qsizetype i = nDurations - 1; i >= 0; --i) {
        auto &duration = durations[i];
        auto maxSamples = duration.num * rate / (1000 * duration.den);

        if (totalSamples >= maxSamples) {
            auto readBytes = bps * channels * maxSamples / 8;
            auto buffer = this->m_audioBuffer.mid(0, readBytes);
            this->m_audioBuffer.remove(0, readBytes);

            if (readSamples)
                *readSamples = maxSamples;

            return buffer;
        }
    }

    if (readSamples)
        *readSamples = 0;

    return {};
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
            opus_encoder_create(this->m_outputCaps.rate(),
                                this->m_outputCaps.channels(),
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

    this->m_audioConverter.reset();
    this->updateHeaders();
    this->m_audioBuffer.clear();
    this->m_pts = 0;
    this->m_initialized = true;

    return true;
}

void AudioEncoderOpusElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;

    if (this->m_encoder) {
        opus_encoder_destroy(this->m_encoder);
        this->m_encoder = nullptr;
    }

    this->m_audioBuffer.clear();
}

void AudioEncoderOpusElementPrivate::updateHeaders()
{
    // https://wiki.xiph.org/OggOpus

    QByteArray oggOpusHeader;
    QDataStream ds(&oggOpusHeader, QIODeviceBase::WriteOnly);
    ds.writeRawData("OpusHead", 8); // Magic signature
    ds << quint8(1);  // Version number
    ds << quint8(this->m_outputCaps.channels()); // Channels
    opus_int32 preSkip = 0;
    opus_encoder_ctl(this->m_encoder, OPUS_GET_LOOKAHEAD(&preSkip));
    ds << quint16_le(48000 * preSkip / this->m_outputCaps.rate()); // Pre-skip
    ds << quint32_le(this->m_outputCaps.rate());
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
    headerPacket.setTimeBase({1, this->m_outputCaps.rate()});
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
    this->m_audioConverter.setOutputCaps({AkAudioCaps::SampleFormat_s16,
                                          AkAudioCaps::defaultChannelLayout(channels),
                                          false,
                                          inputCaps.rate()});
    AkCompressedAudioCaps outputCaps(self->codec(),
                                     this->m_audioConverter.outputCaps().bps(),
                                     this->m_audioConverter.outputCaps().channels(),
                                     this->m_audioConverter.outputCaps().rate());

    if (this->m_outputCaps == outputCaps)
        return;

    this->m_outputCaps = outputCaps;
    emit self->outputCapsChanged(outputCaps);
}

#include "moc_audioencoderopuselement.cpp"
