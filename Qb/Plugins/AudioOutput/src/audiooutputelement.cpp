/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "audiooutputelement.h"

AudioOutputElement::AudioOutputElement(): QbElement()
{
    this->m_output = Qb::create("MultiSink");

    QObject::connect(this->m_output.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SIGNAL(oStream(const QbPacket &)));

    QObject::connect(this,
                     SIGNAL(stateChanged(ElementState)),
                     this->m_output.data(),
                     SLOT(setState(ElementState)));

    this->resetAudioSystem();
}

QString AudioOutputElement::audioSystem()
{
    return this->m_audioSystem;
}

QStringList AudioOutputElement::availableAudioSystem()
{
    QStringList audioSystems;

    if (QFileInfo("/usr/bin/pulseaudio").exists())
        audioSystems << "pulseaudio";

    if (QFileInfo("/proc/asound/version").exists())
        audioSystems << "alsa";

    if (QFileInfo("/dev/dsp").exists())
        audioSystems << "oss";

    return audioSystems;
}

void AudioOutputElement::setAudioSystem(QString audioSystem)
{
    this->m_audioSystem = audioSystem;
}

void AudioOutputElement::resetAudioSystem()
{
    QStringList audioSystems = availableAudioSystem();

    this->setAudioSystem(audioSystems.isEmpty()? "": audioSystems[0]);
}

void AudioOutputElement::iStream(const QbPacket &packet)
{
    this->m_output->iStream(packet);
}
