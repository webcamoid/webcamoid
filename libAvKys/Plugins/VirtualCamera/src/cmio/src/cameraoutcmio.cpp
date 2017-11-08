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

#include <QSettings>
#include <QFileInfo>
#include <akvideopacket.h>

#include "cameraoutcmio.h"

#define MAX_CAMERAS 1

CameraOutCMIO::CameraOutCMIO(QObject *parent):
    CameraOut(parent)
{
    this->m_streamIndex = -1;
    this->m_passwordTimeout = 5000;
}

CameraOutCMIO::~CameraOutCMIO()
{
}

QString CameraOutCMIO::driverPath() const
{
    return this->m_driverPath;
}

QStringList CameraOutCMIO::webcams() const
{
    return {};
}

QString CameraOutCMIO::device() const
{
    return this->m_device;
}

int CameraOutCMIO::streamIndex() const
{
    return this->m_streamIndex;
}

AkCaps CameraOutCMIO::caps() const
{
    return this->m_caps;
}

QString CameraOutCMIO::description(const QString &webcam) const
{
    Q_UNUSED(webcam)

    return QString();
}
void CameraOutCMIO::writeFrame(const AkPacket &frame)
{
    Q_UNUSED(frame)
}

int CameraOutCMIO::maxCameras() const
{
    return MAX_CAMERAS;
}

bool CameraOutCMIO::needRoot() const
{
    return false;
}

int CameraOutCMIO::passwordTimeout() const
{
    return this->m_passwordTimeout;
}

QString CameraOutCMIO::rootMethod() const
{
    return this->m_rootMethod;
}

QString CameraOutCMIO::createWebcam(const QString &description,
                                     const QString &password)
{
    Q_UNUSED(description)
    Q_UNUSED(password)

    return QString();
}

bool CameraOutCMIO::changeDescription(const QString &webcam,
                                       const QString &description,
                                       const QString &password) const
{
    Q_UNUSED(webcam)
    Q_UNUSED(description)
    Q_UNUSED(password)

    return false;
}

bool CameraOutCMIO::removeWebcam(const QString &webcam,
                                  const QString &password)
{
    Q_UNUSED(webcam)
    Q_UNUSED(password)

    return false;
}

bool CameraOutCMIO::removeAllWebcams(const QString &password)
{
    Q_UNUSED(password)

    for (const QString &webcam: this->webcams())
        this->removeWebcam(webcam, password);

    return true;
}

bool CameraOutCMIO::sudo(const QString &command,
                          const QString &params,
                          const QString &dir,
                          bool hide) const
{
    Q_UNUSED(command)
    Q_UNUSED(params)
    Q_UNUSED(dir)
    Q_UNUSED(hide)

    return false;
}

bool CameraOutCMIO::init(int streamIndex, const AkCaps &caps)
{
    this->m_streamIndex = streamIndex;
    this->m_caps = caps;

    return false;
}

void CameraOutCMIO::uninit()
{
}

void CameraOutCMIO::setDriverPath(const QString &driverPath)
{
    if (this->m_driverPath == driverPath)
        return;

    this->m_driverPath = driverPath;
    emit this->driverPathChanged(driverPath);
}

void CameraOutCMIO::setDevice(const QString &device)
{
    if (this->m_device == device)
        return;

    this->m_device = device;
    emit this->deviceChanged(device);
}

void CameraOutCMIO::setPasswordTimeout(int passwordTimeout)
{
    if (this->m_passwordTimeout == passwordTimeout)
        return;

    this->m_passwordTimeout = passwordTimeout;
    emit this->passwordTimeoutChanged(passwordTimeout);
}

void CameraOutCMIO::setRootMethod(const QString &rootMethod)
{
    if (this->m_rootMethod == rootMethod)
        return;

    this->m_rootMethod = rootMethod;
    emit this->rootMethodChanged(rootMethod);
}

void CameraOutCMIO::resetDriverPath()
{
    this->setDriverPath("");
}

void CameraOutCMIO::resetDevice()
{
    this->setDevice("");
}

void CameraOutCMIO::resetPasswordTimeout()
{
    this->setPasswordTimeout(5000);
}

void CameraOutCMIO::resetRootMethod()
{
    this->setRootMethod("");
}
