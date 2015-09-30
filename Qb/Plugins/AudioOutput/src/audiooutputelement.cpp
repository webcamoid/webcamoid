/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "audiooutputelement.h"

AudioOutputElement::AudioOutputElement(): QbElement()
{
    this->m_convert = QbElement::create("ACapsConvert");
    this->m_caps = QString("audio/x-raw,"
                           "format=s16,"
                           "bps=2,"
                           "channels=2,"
                           "rate=44100,"
                           "layout=stereo,"
                           "align=0");

    QObject::connect(this,
                     &AudioOutputElement::stateChanged,
                     this->m_convert.data(),
                     &QbElement::setState);
}

AudioOutputElement::~AudioOutputElement()
{
    this->uninit();
}

QbCaps AudioOutputElement::caps() const
{
    return this->m_caps;
}

bool AudioOutputElement::init()
{
    this->m_mutex.lock();
    QbAudioCaps caps(this->m_caps);
    this->m_convert->setProperty("caps", this->m_caps.toString());
    bool result = this->m_audioDevice.init(AudioDevice::DeviceModePlayback,
                                           caps.format(),
                                           caps.channels(),
                                           caps.rate());
    this->m_mutex.unlock();

    return result;
}

void AudioOutputElement::uninit()
{
    this->m_mutex.lock();
    this->m_audioDevice.uninit();
    this->m_mutex.unlock();
}

void AudioOutputElement::stateChange(QbElement::ElementState from,
                                     QbElement::ElementState to)
{
    if (from == QbElement::ElementStateNull
        && to == QbElement::ElementStatePaused)
        this->init();
    else if (from == QbElement::ElementStatePaused
             && to == QbElement::ElementStateNull)
        this->uninit();
}

void AudioOutputElement::setCaps(const QbCaps &caps)
{
    if (this->m_caps == caps)
        return;

    this->m_caps = caps;
    emit this->capsChanged(caps);
}

void AudioOutputElement::resetCaps()
{
    QString caps("audio/x-raw,"
                 "format=s16,"
                 "bps=2,"
                 "channels=2,"
                 "rate=44100,"
                 "layout=stereo,"
                 "align=0");

    this->setCaps(caps);
}

QbPacket AudioOutputElement::iStream(const QbAudioPacket &packet)
{
    this->m_mutex.lock();
    QbPacket iPacket = this->m_convert->iStream(packet.toPacket());

    QByteArray frame(iPacket.buffer().data(), iPacket.bufferSize());
    this->m_audioDevice.write(frame);
    this->m_mutex.unlock();

    return QbPacket();
}
