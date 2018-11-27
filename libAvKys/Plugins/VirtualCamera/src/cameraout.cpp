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

#include <akpacket.h>

#include "cameraout.h"

CameraOut::CameraOut(QObject *parent):
    QObject(parent)
{
}

CameraOut::~CameraOut()
{
}

QStringList CameraOut::driverPaths() const
{
    return this->m_driverPaths;
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
    return -1;
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

int CameraOut::maxCameras() const
{
    return 0;
}

QStringList CameraOut::availableRootMethods() const
{
    return {};
}

QString CameraOut::rootMethod() const
{
    return {};
}

QString CameraOut::createWebcam(const QString &description)
{
    Q_UNUSED(description)

    return QString();
}

bool CameraOut::changeDescription(const QString &webcam,
                                  const QString &description)
{
    Q_UNUSED(webcam)
    Q_UNUSED(description)

    return false;
}

bool CameraOut::removeWebcam(const QString &webcam)
{
    Q_UNUSED(webcam)

    return false;
}

bool CameraOut::removeAllWebcams()
{
    return true;
}

bool CameraOut::init(int streamIndex)
{
    Q_UNUSED(streamIndex)

    return false;
}

void CameraOut::uninit()
{
}

bool CameraOut::setDriverPaths(const QStringList &driverPaths)
{
    if (this->m_driverPaths == driverPaths)
        return false;

    this->m_driverPaths = driverPaths;
    emit this->driverPathsChanged(this->m_driverPaths);

    return true;
}

bool CameraOut::addDriverPath(const QString &driverPath)
{
    if (driverPath.isEmpty() || this->m_driverPaths.contains(driverPath))
        return false;

    this->m_driverPaths << driverPath;
    emit this->driverPathsChanged(this->m_driverPaths);

    return true;
}

bool CameraOut::addDriverPaths(const QStringList &driverPaths)
{
    bool added = false;

    for (auto path: driverPaths)
        if (!path.isEmpty() && !this->m_driverPaths.contains(path)) {
            this->m_driverPaths << path;
            added = true;
        }

    if (added)
        emit this->driverPathsChanged(this->m_driverPaths);

    return added;
}

bool CameraOut::removeDriverPath(const QString &driverPath)
{
    if (driverPath.isEmpty() || !this->m_driverPaths.contains(driverPath))
        return false;

    this->m_driverPaths.removeAll(driverPath);
    emit this->driverPathsChanged(this->m_driverPaths);

    return true;
}

bool CameraOut::removeDriverPaths(const QStringList &driverPaths)
{
    bool removed = false;

    for (auto path: driverPaths)
        if (!path.isEmpty() && this->m_driverPaths.contains(path)) {
            this->m_driverPaths.removeAll(path);
            removed = true;
        }

    if (removed)
        emit this->driverPathsChanged(this->m_driverPaths);

    return removed;
}

void CameraOut::setDevice(const QString &device)
{
    if (this->m_device == device)
        return;

    this->m_device = device;
    emit this->deviceChanged(this->m_device);
}

void CameraOut::setCaps(const AkCaps &caps)
{
    if (this->m_caps == caps)
        return;

    this->m_caps = caps;
    emit this->capsChanged(this->m_caps);
}

void CameraOut::setRootMethod(const QString &rootMethod)
{
    Q_UNUSED(rootMethod)
}

void CameraOut::resetDriverPaths()
{
    if (this->m_driverPaths.isEmpty())
        return

    this->m_driverPaths.clear();
    emit this->driverPathsChanged(this->m_driverPaths);
}

void CameraOut::resetDevice()
{
    this->setDevice("");
}

void CameraOut::resetCaps()
{
    this->setCaps(AkCaps());
}

void CameraOut::resetRootMethod()
{
    this->setRootMethod("");
}

#include "moc_cameraout.cpp"
