/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#include <akelement.h>

#include "audiodeviceglobals.h"

AudioDeviceGlobals::AudioDeviceGlobals(QObject *parent):
    QObject(parent)
{
    this->m_preferredLibrary = QStringList {
#ifdef Q_OS_WIN32
        "wasapi",
        "qtaudio"
#elif defined(Q_OS_OSX)
        "coreaudio",
        "pulseaudio",
        "jack",
        "qtaudio"
#else
        "pulseaudio",
        "alsa",
        "oss",
        "jack",
        "qtaudio"
#endif
    };

    this->resetAudioLib();
}

QString AudioDeviceGlobals::audioLib() const
{
    return this->m_audioLib;
}

void AudioDeviceGlobals::setAudioLib(const QString &audioLib)
{
    if (this->m_audioLib == audioLib)
        return;

    this->m_audioLib = audioLib;
    emit this->audioLibChanged(audioLib);
}

void AudioDeviceGlobals::resetAudioLib()
{
    auto subModules = AkElement::listSubModules("AudioDevice");

    for (auto &framework: this->m_preferredLibrary)
        if (subModules.contains(framework)) {
            this->setAudioLib(framework);

            return;
        }

    if (this->m_audioLib.isEmpty() && !subModules.isEmpty())
        this->setAudioLib(subModules.first());
    else
        this->setAudioLib("");
}
