/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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

#include "platform/capturewin.h"

Capture::Capture()
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

QString Capture::ioMethod() const
{
    return QString();
}

int Capture::nBuffers() const
{
    return 0;
}

bool Capture::isCompressed() const
{
    return false;
}

QString Capture::caps() const
{
    return QString();
}

QString Capture::description(const QString &webcam) const
{
    Q_UNUSED(webcam)

    return QString();
}

QVariantList Capture::availableSizes(const QString &webcam) const
{
    Q_UNUSED(webcam)

    return QVariantList();
}

QSize Capture::size(const QString &webcam) const
{
    Q_UNUSED(webcam)

    return QSize();
}

bool Capture::setSize(const QString &webcam, const QSize &size) const
{
    Q_UNUSED(webcam)
    Q_UNUSED(size)

    return false;
}

bool Capture::resetSize(const QString &webcam) const
{
    Q_UNUSED(webcam)

    return false;
}

QVariantList Capture::controls(const QString &webcam) const
{
    Q_UNUSED(webcam)

    return QVariantList();
}

bool Capture::setControls(const QString &webcam, const QVariantMap &controls) const
{
    Q_UNUSED(webcam)
    Q_UNUSED(controls)

    return false;
}

bool Capture::resetControls(const QString &webcam) const
{
    Q_UNUSED(webcam)

    return false;
}

QbPacket Capture::readFrame() const
{
    return QbPacket();
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
    Q_UNUSED(device);
}

void Capture::setIoMethod(const QString &ioMethod)
{
    Q_UNUSED(ioMethod);
}

void Capture::setNBuffers(int nBuffers)
{
    Q_UNUSED(nBuffers);
}

void Capture::resetDevice()
{

}

void Capture::resetIoMethod()
{

}

void Capture::resetNBuffers()
{

}

void Capture::reset(const QString &webcam="") const
{
    Q_UNUSED(webcam)
}
