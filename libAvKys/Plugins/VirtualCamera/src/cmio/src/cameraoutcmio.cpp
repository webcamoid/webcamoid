/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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
#include <QFileInfo>
#include <QDir>
#include <akvideocaps.h>
#include <akvideopacket.h>

#include "cameraoutcmio.h"
#include "ipcbridge.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/image/videoframe.h"

#define MAX_CAMERAS 64

class CameraOutCMIOPrivate
{
    public:
        QDir m_applicationDir;
        QStringList m_webcams;
        QString m_curDevice;
        AkVCam::IpcBridge m_ipcBridge;
        int m_streamIndex;

        CameraOutCMIOPrivate();
        QString convertToAbsolute(const QString &path) const;
};

CameraOutCMIO::CameraOutCMIO(QObject *parent):
    CameraOut(parent)
{
    this->d = new CameraOutCMIOPrivate;
    QObject::connect(this,
                     &CameraOut::driverPathsChanged,
                     this,
                     &CameraOutCMIO::updateDriverPaths);
    this->resetDriverPaths();
    this->d->m_ipcBridge.registerPeer(false);
}

CameraOutCMIO::~CameraOutCMIO()
{
    this->d->m_ipcBridge.unregisterPeer();
    delete this->d;
}

QStringList CameraOutCMIO::webcams() const
{
    QStringList webcams;

    for (auto &device: this->d->m_ipcBridge.listDevices())
        webcams << QString::fromStdString(device);

    return webcams;
}

int CameraOutCMIO::streamIndex() const
{
    return this->d->m_streamIndex;
}

QString CameraOutCMIO::description(const QString &webcam) const
{
    for (auto &device: this->d->m_ipcBridge.listDevices()) {
        auto deviceId = QString::fromStdString(device);

        if (deviceId == webcam)
            return QString::fromStdString(this->d->m_ipcBridge.description(device));
    }

    return {};
}

void CameraOutCMIO::writeFrame(const AkPacket &frame)
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

int CameraOutCMIO::maxCameras() const
{
    return MAX_CAMERAS;
}

QString CameraOutCMIO::createWebcam(const QString &description,
                                    const QString &password)
{
    Q_UNUSED(password)

    auto webcams = this->webcams();
    AkVideoCaps caps(this->m_caps);
    auto webcam =
            this->d->m_ipcBridge.deviceCreate(description.isEmpty()?
                                                  "AvKys Virtual Camera":
                                                  description.toStdString(),
                                              {{AkVCam::PixelFormatRGB32,
                                                640, 480,
                                                {caps.fps().value()}}});

    if (webcam.size() < 1)
        return {};

    auto curWebcams = this->webcams();

    if (curWebcams != webcams)
        emit this->webcamsChanged(curWebcams);

    return QString::fromStdString(webcam);
}

bool CameraOutCMIO::changeDescription(const QString &webcam,
                                      const QString &description,
                                      const QString &password)
{
    Q_UNUSED(password)

    QStringList webcams = this->webcams();

    if (!webcams.contains(webcam))
        return false;

    bool result =
            this->d->m_ipcBridge.changeDescription(webcam.toStdString(),
                                                   description.toStdString());

    auto curWebcams = this->webcams();

    if (curWebcams != webcams)
        emit this->webcamsChanged(curWebcams);

    return result;
}

bool CameraOutCMIO::removeWebcam(const QString &webcam,
                                 const QString &password)
{
    Q_UNUSED(password)

    QStringList webcams = this->webcams();

    if (!webcams.contains(webcam))
        return false;

    this->d->m_ipcBridge.deviceDestroy(webcam.toStdString());
    emit this->webcamsChanged({});

    return true;
}

bool CameraOutCMIO::removeAllWebcams(const QString &password)
{
    Q_UNUSED(password)

    this->d->m_ipcBridge.destroyAllDevices();
    emit this->webcamsChanged({});

    return true;
}

bool CameraOutCMIO::init(int streamIndex)
{
    if (!this->d->m_ipcBridge.deviceStart(this->m_device.toStdString()))
        return false;

    this->d->m_streamIndex = streamIndex;
    this->d->m_curDevice = this->m_device;

    return true;
}

void CameraOutCMIO::uninit()
{
    if (this->d->m_curDevice.isEmpty())
        return;

    this->d->m_ipcBridge.deviceStop(this->d->m_curDevice.toStdString());
    this->d->m_streamIndex = -1;
    this->d->m_curDevice.clear();
}

void CameraOutCMIO::resetDriverPaths()
{
    QStringList driverPaths {
        DATAROOTDIR,
        this->d->convertToAbsolute(QString("../share")),
        this->d->convertToAbsolute(QString("../Resources"))
    };

    this->setDriverPaths(driverPaths);
}

void CameraOutCMIO::updateDriverPaths(const QStringList &driverPaths)
{
    std::vector<std::string> paths;

    for (auto &path: driverPaths)
        paths.push_back(path.toStdString());

    this->d->m_ipcBridge.setDriverPaths(paths);
}

CameraOutCMIOPrivate::CameraOutCMIOPrivate():
    m_streamIndex(-1)
{
    this->m_applicationDir.setPath(QCoreApplication::applicationDirPath());
}

QString CameraOutCMIOPrivate::convertToAbsolute(const QString &path) const
{
    if (!QDir::isRelativePath(path))
        return QDir::cleanPath(path);

    QString absPath = this->m_applicationDir.absoluteFilePath(path);

    return QDir::cleanPath(absPath);
}

#include "moc_cameraoutcmio.cpp"
