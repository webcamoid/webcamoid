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
#include <QVariant>
#include <akfrac.h>
#include <akpacket.h>
#include <akaudiocaps.h>
#include <akcompressedaudiocaps.h>
#include <akaudioconverter.h>
#include <akaudiopacket.h>
#include <akcompressedaudiopacket.h>
#include <lame/lame.h>

#include "audioencoderlameelement.h"

class AudioEncoderLameElementPrivate
{
    public:
        AudioEncoderLameElement *self;
        AkAudioConverter m_audioConverter;
        AkCompressedAudioCaps m_outputCaps;
        lame_global_flags *m_encoder {nullptr};
        QMutex m_mutex;
        bool m_initialized {false};
        qint64 m_id {0};
        int m_index {0};
        qint64 m_pts {0};

        explicit AudioEncoderLameElementPrivate(AudioEncoderLameElement *self);
        ~AudioEncoderLameElementPrivate();
        bool init();
        void uninit();
        void updateOutputCaps(const AkAudioCaps &inputCaps);
        void sendFrame(const QByteArray &packetData,
                       qsizetype samples,
                       qsizetype writtenBytes);
};

AudioEncoderLameElement::AudioEncoderLameElement():
    AkAudioEncoder()
{
    this->d = new AudioEncoderLameElementPrivate(this);
}

AudioEncoderLameElement::~AudioEncoderLameElement()
{
    this->d->uninit();
    delete this->d;
}

AkAudioEncoderCodecID AudioEncoderLameElement::codec() const
{
    return AkCompressedAudioCaps::AudioCodecID_mpeg3;
}

AkCompressedAudioCaps AudioEncoderLameElement::outputCaps() const
{
    return this->d->m_outputCaps;
}

QString AudioEncoderLameElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/AudioEncoderLame/share/qml/main.qml");
}

void AudioEncoderLameElement::controlInterfaceConfigure(QQmlContext *context,
                                                       const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("AudioEncoderLame", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket AudioEncoderLameElement::iAudioStream(const AkAudioPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_initialized)
        return {};

    auto src = this->d->m_audioConverter.convert(packet);

    if (!src)
        return {};

    this->d->m_id = src.id();
    this->d->m_index = src.index();

    QByteArray packetData(packet.samples()
                          * packet.caps().channels()
                          * sizeof(short int),
                          Qt::Uninitialized);
    auto writtenBytes =
            lame_encode_buffer_interleaved(this->d->m_encoder,
                                           reinterpret_cast<short int *>(const_cast<char *>(packet.constData())),
                                           packet.samples(),
                                           reinterpret_cast<unsigned char *>(packetData.data()),
                                           packetData.size());

    if (writtenBytes > 1) {
        this->d->sendFrame(packetData,
                           packet.samples(),
                           writtenBytes);
    }

    return {};
}

bool AudioEncoderLameElement::setState(ElementState state)
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

AudioEncoderLameElementPrivate::AudioEncoderLameElementPrivate(AudioEncoderLameElement *self):
    self(self)
{
    QObject::connect(self,
                     &AkAudioEncoder::inputCapsChanged,
                     [this] (const AkAudioCaps &inputCaps) {
                         this->updateOutputCaps(inputCaps);
                     });
}

AudioEncoderLameElementPrivate::~AudioEncoderLameElementPrivate()
{

}

bool AudioEncoderLameElementPrivate::init()
{
    this->uninit();

    auto inputCaps = self->inputCaps();

    if (!inputCaps) {
        qCritical() << "Invalid input format.";

        return false;
    }

    this->m_encoder = lame_init();

    if (!this->m_encoder) {
        qCritical() << "Failed initializing the encoder";

        return false;
    }

    lame_set_in_samplerate(this->m_encoder, this->m_outputCaps.rate());
    lame_set_out_samplerate(this->m_encoder, this->m_outputCaps.rate());
    lame_set_brate(this->m_encoder, self->bitrate() / 1000);
    lame_set_num_channels(this->m_encoder, this->m_outputCaps.channels());
    lame_set_mode(this->m_encoder, this->m_outputCaps.channels() < 2? MONO: STEREO);
    lame_set_bWriteVbrTag(this->m_encoder, false);

    if (lame_init_params(this->m_encoder) < 0) {
        qCritical() << "Failed initializing the parameters";

        return false;
    }

    this->m_audioConverter.reset();
    this->m_pts = 0;
    this->m_initialized = true;

    return true;
}

void AudioEncoderLameElementPrivate::uninit()
{
    QMutexLocker mutexLocker(&this->m_mutex);

    if (!this->m_initialized)
        return;

    this->m_initialized = false;

    if (!this->m_encoder)
        return;

    // Flush up to 1 second of encoded audio data
    QByteArray packetData(1000 * lame_get_brate(this->m_encoder) / 8,
                          Qt::Uninitialized);
    auto writtenBytes = lame_encode_flush(this->m_encoder,
                                          reinterpret_cast<unsigned char *>(packetData.data()),
                                          packetData.size());

    if (writtenBytes > 1)
        this->sendFrame(packetData, 0, writtenBytes);

    lame_close(this->m_encoder);
}

void AudioEncoderLameElementPrivate::updateOutputCaps(const AkAudioCaps &inputCaps)
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

void AudioEncoderLameElementPrivate::sendFrame(const QByteArray &packetData,
                                               qsizetype samples,
                                               qsizetype writtenBytes)
{
    AkCompressedAudioPacket packet(this->m_outputCaps, writtenBytes);
    memcpy(packet.data(), packetData.constData(), packet.size());
    packet.setPts(this->m_pts);
    packet.setDts(this->m_pts);
    packet.setDuration(samples);
    packet.setTimeBase({1, this->m_outputCaps.rate()});
    packet.setId(this->m_id);
    packet.setIndex(this->m_index);

    emit self->oStream(packet);

    this->m_pts += samples;
}

#include "moc_audioencoderlameelement.cpp"
