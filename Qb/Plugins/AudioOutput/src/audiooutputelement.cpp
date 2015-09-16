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

    QObject::connect(this,
                     SIGNAL(stateChanged(QbElement::ElementState)),
                     this->m_convert.data(),
                     SLOT(setState(QbElement::ElementState)));
}

AudioOutputElement::~AudioOutputElement()
{
    this->uninit();
}

bool AudioOutputElement::init()
{
    this->m_mutex.lock();

    QString caps("audio/x-raw,"
                 "format=s16,"
                 "bps=2,"
                 "channels=2,"
                 "rate=44100,"
                 "layout=stereo,"
                 "align=0");

    this->m_convert->setProperty("caps", caps);
    bool result = this->m_audioOut.init(QbAudioCaps::SampleFormat_s16, 2, 44100);
    this->m_mutex.unlock();

    return result;
}

void AudioOutputElement::uninit()
{
    this->m_mutex.lock();
    this->m_audioOut.uninit();
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

QbPacket AudioOutputElement::iStream(const QbAudioPacket &packet)
{
    this->m_mutex.lock();
    QbPacket iPacket = this->m_convert->iStream(packet.toPacket());
    QByteArray frame(iPacket.buffer().data(), iPacket.bufferSize());
    this->m_audioOut.write(frame);
    this->m_mutex.unlock();

    return QbPacket();
}
