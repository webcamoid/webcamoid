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

#include "audioinputelement.h"

AudioInputElement::AudioInputElement(): QbElement()
{
    this->m_input = Qb::create("MultiSrc");

    QObject::connect(this->m_input.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SIGNAL(oStream(const QbPacket &)));

    QObject::connect(this,
                     SIGNAL(stateChanged(ElementState)),
                     this->m_input.data(),
                     SLOT(setState(ElementState)));

    this->resetAudioSystem();
}

QString AudioInputElement::audioSystem()
{
    return this->m_audioSystem;
}

QStringList AudioInputElement::availableAudioSystem()
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

QVariantMap AudioInputElement::streamCaps()
{
    return this->m_input->property("streamCaps").toMap();
}

void AudioInputElement::setAudioSystem(QString audioSystem)
{
    if (audioSystem != this->m_audioSystem)
    {
        ElementState preState = this->m_input->state();
        this->m_input->setState(ElementStateNull);
        bool isValid = true;

        if (audioSystem == "pulseaudio")
            this->m_input->setProperty("location", "pulse");
        else if (audioSystem == "alsa")
            this->m_input->setProperty("location", "hw:0");
        else if (audioSystem == "oss")
            this->m_input->setProperty("location", "/dev/dsp");
        else
            isValid = false;

        if (isValid)
            this->m_input->setState(preState);
        else
            this->setState(ElementStateNull);
    }

    this->m_audioSystem = audioSystem;
}

void AudioInputElement::resetAudioSystem()
{
    QStringList audioSystems = availableAudioSystem();

    this->setAudioSystem(audioSystems.isEmpty()? "": audioSystems[0]);
}
