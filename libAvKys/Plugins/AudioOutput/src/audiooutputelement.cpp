/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include "audiooutputelement.h"

AudioOutputElement::AudioOutputElement(): AkElement()
{
    this->m_caps = QString("audio/x-raw,"
                           "format=s16,"
                           "bps=2,"
                           "channels=2,"
                           "rate=44100,"
                           "layout=stereo,"
                           "align=0");
}

AudioOutputElement::~AudioOutputElement()
{
    this->uninit();
}

AkCaps AudioOutputElement::caps() const
{
    return this->m_caps;
}

bool AudioOutputElement::init()
{
    this->m_mutex.lock();
    AkAudioCaps caps(this->m_caps);
    this->m_convert = AkElement::create("ACapsConvert");

    QObject::connect(this,
                     SIGNAL(stateChanged(AkElement::ElementState)),
                     this->m_convert.data(),
                     SLOT(setState(AkElement::ElementState)));

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

void AudioOutputElement::stateChange(AkElement::ElementState from,
                                     AkElement::ElementState to)
{
    if (from == AkElement::ElementStateNull
        && to == AkElement::ElementStatePaused)
        this->init();
    else if (from == AkElement::ElementStatePaused
             && to == AkElement::ElementStateNull)
        this->uninit();
}

void AudioOutputElement::setCaps(const AkCaps &caps)
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

AkPacket AudioOutputElement::iStream(const AkAudioPacket &packet)
{
    this->m_mutex.lock();

    if (this->m_convert) {
        AkPacket iPacket = this->m_convert->iStream(packet.toPacket());
        this->m_audioDevice.write(iPacket.buffer());
    }

    this->m_mutex.unlock();

    return AkPacket();
}
