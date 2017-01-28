/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include "capture.h"

Capture::Capture(QObject *parent):
    QObject(parent)
{
}

Capture::~Capture()
{
}

QStringList Capture::webcams() const
{
    return QStringList();
}

QString Capture::device() const
{
    return QString();
}

QList<int> Capture::streams() const
{
    return QList<int>();
}

QList<int> Capture::listTracks(const QString &mimeType)
{
    Q_UNUSED(mimeType);

    return QList<int>();
}

QString Capture::ioMethod() const
{
    return QString();
}

int Capture::nBuffers() const
{
    return 0;
}

QString Capture::description(const QString &webcam) const
{
    Q_UNUSED(webcam)

    return QString();
}

QVariantList Capture::caps(const QString &webcam) const
{
    Q_UNUSED(webcam)

    return QVariantList();
}

QString Capture::capsDescription(const AkCaps &caps) const
{
    Q_UNUSED(caps)

    return QString();
}

QVariantList Capture::imageControls() const
{
    return QVariantList();
}

bool Capture::setImageControls(const QVariantMap &imageControls)
{
    Q_UNUSED(imageControls)

    return false;
}

bool Capture::resetImageControls()
{
    return false;
}

QVariantList Capture::cameraControls() const
{
    return QVariantList();
}

bool Capture::setCameraControls(const QVariantMap &cameraControls)
{
    Q_UNUSED(cameraControls)

    return false;
}

bool Capture::resetCameraControls()
{
    return false;
}

AkPacket Capture::readFrame()
{
    return AkPacket();
}

bool Capture::init()
{
    return false;
}

void Capture::uninit()
{
}

void Capture::setDevice(const QString &device)
{
    Q_UNUSED(device)
}

void Capture::setStreams(const QList<int> &streams)
{
    Q_UNUSED(streams)
}

void Capture::setIoMethod(const QString &ioMethod)
{
    Q_UNUSED(ioMethod)
}

void Capture::setNBuffers(int nBuffers)
{
    Q_UNUSED(nBuffers)
}

void Capture::resetDevice()
{
}

void Capture::resetStreams()
{
}

void Capture::resetIoMethod()
{
}

void Capture::resetNBuffers()
{
}

void Capture::reset()
{
}
