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

#include <QDebug>
#include <QCoreApplication>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <akvideocaps.h>
#include <akvideopacket.h>
#include <strmif.h>
#include <initguid.h>
#include <uuids.h>
#include <vfwmsgs.h>

#include "cameraoutdshow.h"
#include "ipcbridge.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/image/videoframe.h"

#define MAX_CAMERAS 64
#define VCAM_DRIVER "VirtualCameraSource.dll"

class CameraOutDShowPrivate
{
    public:
        QStringList m_webcams;
        QString m_curDevice;
        AkVCam::IpcBridge m_ipcBridge;
        int m_streamIndex;

        CameraOutDShowPrivate():
            m_streamIndex(-1)
        {
        }
};

CameraOutDShow::CameraOutDShow(QObject *parent):
    CameraOut(parent)
{
    this->d = new CameraOutDShowPrivate;
    QDir applicationDir(QCoreApplication::applicationDirPath());
    this->m_driverPath = applicationDir.absoluteFilePath(VCAM_DRIVER);
}

CameraOutDShow::~CameraOutDShow()
{
    this->d->m_ipcBridge.unregisterPeer();
    delete this->d;
}

QStringList CameraOutDShow::webcams() const
{
    QStringList webcams;

    for (auto &device: this->d->m_ipcBridge.listDevices(true))
        webcams << QString::fromStdString(device);

    return webcams;
}

int CameraOutDShow::streamIndex() const
{
    return this->d->m_streamIndex;
}

QString CameraOutDShow::description(const QString &webcam) const
{
    for (auto &device: this->d->m_ipcBridge.listDevices(false)) {
        auto deviceId = QString::fromStdString(device);

        if (deviceId == webcam)
            return QString::fromStdString(this->d->m_ipcBridge.description(device));
    }

    return {};
}

void CameraOutDShow::writeFrame(const AkPacket &frame)
{
    if (this->d->m_curDevice.isEmpty())
        return;

    AkVideoPacket videoFrame(frame);
    AkVCam::VideoFormat format(videoFrame.caps().fourCC(),
                               videoFrame.caps().width(),
                               videoFrame.caps().height(),
                               {videoFrame.caps().fps().value()});

    this->d->m_ipcBridge.write(this->d->m_curDevice.toStdString(),
                               AkVCam::VideoFrame(format,
                                                  reinterpret_cast<const uint8_t *>(videoFrame.buffer().constData()),
                                                  size_t(videoFrame.buffer().size())));
}

int CameraOutDShow::maxCameras() const
{
    return MAX_CAMERAS;
}

QString CameraOutDShow::createWebcam(const QString &description,
                                     const QString &password)
{
    Q_UNUSED(password)

    if (!QFileInfo(this->m_driverPath).exists())
        return QString();

    QStringList webcams = this->webcams();

    if (!webcams.isEmpty())
        return QString();
/*
    QString reg =
            QString("HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\%1\\Instance\\%2")
            .arg(this->d->iidToString(CLSID_VideoInputDeviceCategory))
            .arg(this->d->iidToString(CLSID_VirtualCameraSource));

    QString desc = description.isEmpty()?
                       QString::fromWCharArray(FILTER_NAME):
                       description;

    QString params =
            QString("/c \"regsvr32 \"%1\" && reg add %2 /v FriendlyName /d \"%3\" /f\"")
            .arg(this->m_driverPath)
            .arg(reg)
            .arg(desc);

    if (!this->d->sudo("cmd", params, "", true))
        return QString();
*/
    QStringList curWebcams = this->webcams();

    if (curWebcams != webcams)
        emit this->webcamsChanged(curWebcams);

    return curWebcams.isEmpty()? QString(): curWebcams.first();
}

bool CameraOutDShow::changeDescription(const QString &webcam,
                                       const QString &description,
                                       const QString &password)
{
    Q_UNUSED(password)

    QStringList webcams = this->webcams();

    if (!webcams.contains(webcam))
        return false;
/*
    QString reg =
            QString("HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\%1\\Instance\\%2")
            .arg(this->d->iidToString(CLSID_VideoInputDeviceCategory))
            .arg(this->d->iidToString(CLSID_VirtualCameraSource));

    QString desc = description.isEmpty()?
                       QString::fromWCharArray(FILTER_NAME):
                       description;

    QString params =
            QString("add %1 /v FriendlyName /d \"%2\" /f")
            .arg(reg)
            .arg(desc);

    if (!this->d->sudo("reg", params))
        return false;
*/
    emit this->webcamsChanged(webcams);

    return true;
}

bool CameraOutDShow::removeWebcam(const QString &webcam,
                                  const QString &password)
{
    Q_UNUSED(password)

    QStringList webcams = this->webcams();

    if (!webcams.contains(webcam))
        return false;
/*
    QString reg =
            QString("HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\%1\\InprocServer32")
            .arg(this->d->iidToString(CLSID_VirtualCameraSource));

    QSettings settings(reg, QSettings::NativeFormat);

    QString params =
            QString("/u \"%1\"")
            .arg(settings.value(".").toString());

    if (!this->d->sudo("regsvr32", params))
        return false;
*/
    emit this->webcamsChanged(QStringList());

    return true;
}

bool CameraOutDShow::removeAllWebcams(const QString &password)
{
    Q_UNUSED(password)

    for (const QString &webcam: this->webcams())
        this->removeWebcam(webcam, password);

    return true;
}

bool CameraOutDShow::init(int streamIndex)
{
    if (!this->d->m_ipcBridge.deviceStart(this->m_device.toStdString()))
        return false;

    this->d->m_streamIndex = streamIndex;
    this->d->m_curDevice = this->m_device;

    return true;
}

void CameraOutDShow::uninit()
{
    if (this->d->m_curDevice.isEmpty())
        return;

    this->d->m_ipcBridge.deviceStop(this->d->m_curDevice.toStdString());
    this->d->m_streamIndex = -1;
    this->d->m_curDevice.clear();
}

void CameraOutDShow::resetDriverPath()
{
    QDir applicationDir(QCoreApplication::applicationDirPath());
    this->setDriverPath(applicationDir.absoluteFilePath(VCAM_DRIVER));
}

#include "moc_cameraoutdshow.cpp"
