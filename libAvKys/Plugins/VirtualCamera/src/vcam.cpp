/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

#include "vcam.h"

VCam::VCam(QObject *parent):
    QObject(parent)
{
}

QString VCam::error() const
{
    return {};
}

bool VCam::isInstalled() const
{
    return false;
}

QString VCam::installedVersion() const
{
    return {};
}

QStringList VCam::webcams() const
{
    return {};
}

QString VCam::device() const
{
    return {};
}

QString VCam::description(const QString &webcam) const
{
    Q_UNUSED(webcam)

    return {};
}

QList<AkVideoCaps::PixelFormat> VCam::supportedOutputPixelFormats() const
{
    return {};
}

AkVideoCaps::PixelFormat VCam::defaultOutputPixelFormat() const
{
    return AkVideoCaps::Format_none;
}

AkVideoCapsList VCam::caps(const QString &webcam) const
{
    Q_UNUSED(webcam)

    return {};
}

AkVideoCaps VCam::currentCaps() const
{
    return {};
}

QVariantList VCam::controls() const
{
    return {};
}

bool VCam::setControls(const QVariantMap &controls)
{
    Q_UNUSED(controls)

    return false;
}

QList<quint64> VCam::clientsPids() const
{
    return {};
}

QString VCam::clientExe(quint64 pid) const
{
    Q_UNUSED(pid)

    return {};
}

QString VCam::picture() const
{
    return {};
}

QString VCam::rootMethod() const
{
    return {};
}

QStringList VCam::availableRootMethods() const
{
    return {};
}

QString VCam::deviceCreate(const QString &description,
                           const AkVideoCapsList &caps)
{
    Q_UNUSED(description)
    Q_UNUSED(caps)

    return {};
}

bool VCam::deviceEdit(const QString &deviceId,
                const QString &description,
                const AkVideoCapsList &caps)
{
    Q_UNUSED(deviceId)
    Q_UNUSED(description)
    Q_UNUSED(caps)

    return false;
}

bool VCam::changeDescription(const QString &deviceId,
                       const QString &description)
{
    Q_UNUSED(deviceId)
    Q_UNUSED(description)

    return false;
}

bool VCam::deviceDestroy(const QString &deviceId)
{
    Q_UNUSED(deviceId)

    return false;
}

bool VCam::destroyAllDevices()
{
    return false;
}

bool VCam::init()
{
    return false;
}

void VCam::uninit()
{
}

bool VCam::write(const AkVideoPacket &packet)
{
    Q_UNUSED(packet)

    return false;
}

bool VCam::applyPicture()
{
    return false;
}

void VCam::setDevice(const QString &device)
{
    Q_UNUSED(device)
}

void VCam::setCurrentCaps(const AkVideoCaps &currentCaps)
{
    Q_UNUSED(currentCaps)
}

void VCam::setPicture(const QString &picture)
{
    Q_UNUSED(picture)
}

void VCam::setRootMethod(const QString &rootMethod)
{
    Q_UNUSED(rootMethod)
}

void VCam::resetDevice()
{
    this->setDevice({});
}

void VCam::resetCurrentCaps()
{
    this->setCurrentCaps({});
}

void VCam::resetPicture()
{
    this->setPicture({});
}

void VCam::resetRootMethod()
{
    this->setRootMethod({});
}

#include "moc_vcam.cpp"
