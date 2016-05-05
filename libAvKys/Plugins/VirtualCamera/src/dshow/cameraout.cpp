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

#include "cameraout.h"

CameraOut::CameraOut(): QObject()
{
    this->m_streamIndex = -1;
    this->m_passwordTimeout = 5000;
}

QStringList CameraOut::webcams() const
{
    return QStringList();
}

QString CameraOut::device() const
{
    return this->m_device;
}

int CameraOut::streamIndex() const
{
    return this->m_streamIndex;
}

AkCaps CameraOut::caps() const
{
    return this->m_caps;
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

bool CameraOut::isAvailable() const
{
    return false;
}

bool CameraOut::needRoot() const
{
    return false;
}

int CameraOut::passwordTimeout() const
{
    return this->m_passwordTimeout;
}

QString CameraOut::createWebcam(const QString &description,
                              const QString &password) const
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
                              const QString &password) const
{
    Q_UNUSED(webcam)
    Q_UNUSED(password)

    return false;
}

bool CameraOut::removeAllWebcams(const QString &password) const
{
    Q_UNUSED(password)

    return false;
}

bool CameraOut::init(int streamIndex, const AkCaps &caps)
{
    this->m_streamIndex = streamIndex;
    this->m_caps = caps;

    return false;
}

void CameraOut::uninit()
{
}

void CameraOut::setDevice(const QString &device)
{
    if (this->m_device == device)
        return;

    this->m_device = device;
    emit this->deviceChanged(device);
}

void CameraOut::setPasswordTimeout(int passwordTimeout)
{
    if (this->m_passwordTimeout == passwordTimeout)
        return;

    this->m_passwordTimeout = passwordTimeout;
    emit this->passwordTimeoutChanged(passwordTimeout);
}

void CameraOut::resetDevice()
{
    this->setDevice("");
}

void CameraOut::resetPasswordTimeout()
{
    this->setPasswordTimeout(5000);
}
