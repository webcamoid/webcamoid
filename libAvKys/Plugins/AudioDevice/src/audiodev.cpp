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

#include "audiodev.h"

AudioDev::AudioDev(QObject *parent):
    QObject(parent)
{
}

AudioDev::~AudioDev()
{
}

QString AudioDev::error() const
{
    return QString();
}

QString AudioDev::defaultInput()
{
    return QString();
}

QString AudioDev::defaultOutput()
{
    return QString();
}

QStringList AudioDev::inputs()
{
    return QStringList();
}

QStringList AudioDev::outputs()
{
    return QStringList();
}

QString AudioDev::description(const QString &device)
{
    Q_UNUSED(device)

    return QString();
}

AkAudioCaps AudioDev::preferredFormat(const QString &device)
{
    Q_UNUSED(device)

    return AkAudioCaps();
}

bool AudioDev::init(const QString &device, const AkAudioCaps &caps)
{
    Q_UNUSED(device)
    Q_UNUSED(caps)

    return false;
}

QByteArray AudioDev::read(int samples)
{
    Q_UNUSED(samples)

    return QByteArray();
}

bool AudioDev::write(const AkAudioPacket &packet)
{
    Q_UNUSED(packet)

    return false;
}

bool AudioDev::uninit()
{
    return true;
}
