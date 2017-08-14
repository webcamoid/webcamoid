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

#include "cameraout.h"

CameraOut::CameraOut(QObject *parent):
    QObject(parent)
{
}

CameraOut::~CameraOut()
{
}

QString CameraOut::driverPath() const
{
    return QString();
}

QStringList CameraOut::webcams() const
{
    return QStringList();
}

QString CameraOut::device() const
{
    return QString();
}

int CameraOut::streamIndex() const
{
    return -1;
}

AkCaps CameraOut::caps() const
{
    return AkCaps();
}

QString CameraOut::description(const QString &webcam) const
{
    Q_UNUSED(webcam)

    return QString();
}

void CameraOut::writeFrame(const AkPacket &frame)
{
    Q_UNUSED(frame)
}

int CameraOut::maxCameras() const
{
    return 0;
}

bool CameraOut::needRoot() const
{
    return false;
}

int CameraOut::passwordTimeout() const
{
    return 2500;
}

QString CameraOut::rootMethod() const
{
    return QString();
}

QString CameraOut::createWebcam(const QString &description,
                                const QString &password)
{
    Q_UNUSED(description)
    Q_UNUSED(password)

    return QString();
}

bool CameraOut::changeDescription(const QString &webcam,
                                  const QString &description,
                                  const QString &password) const
{
    Q_UNUSED(webcam)
    Q_UNUSED(description)
    Q_UNUSED(password)

    return false;
}

bool CameraOut::removeWebcam(const QString &webcam,
                             const QString &password)
{
    Q_UNUSED(webcam)
    Q_UNUSED(password)

    return false;
}

bool CameraOut::removeAllWebcams(const QString &password)
{
    Q_UNUSED(password)

    return true;
}

bool CameraOut::init(int streamIndex, const AkCaps &caps)
{
    Q_UNUSED(streamIndex)
    Q_UNUSED(caps)

    return false;
}

void CameraOut::uninit()
{
}

void CameraOut::setDriverPath(const QString &driverPath)
{
    Q_UNUSED(driverPath)
}

void CameraOut::setDevice(const QString &device)
{
    Q_UNUSED(device)
}

void CameraOut::setPasswordTimeout(int passwordTimeout)
{
    Q_UNUSED(passwordTimeout)
}

void CameraOut::setRootMethod(const QString &rootMethod)
{
    Q_UNUSED(rootMethod)
}

void CameraOut::resetDriverPath()
{
}

void CameraOut::resetDevice()
{
}

void CameraOut::resetPasswordTimeout()
{
}

void CameraOut::resetRootMethod()
{
}
