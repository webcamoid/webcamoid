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

#include <QCoreApplication>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <akvideopacket.h>

#include "cameraoutcmio.h"

#define MAX_CAMERAS 1
#define VCAM_DRIVER "../Resources/AkVirtualCamera.plugin"

CameraOutCMIO::CameraOutCMIO(QObject *parent):
    CameraOut(parent)
{
    this->m_streamIndex = -1;
    QDir applicationDir(QCoreApplication::applicationDirPath());
    this->m_driverPath = applicationDir.absoluteFilePath(VCAM_DRIVER);
    this->m_ipcBridge.cleanDevices();
}

CameraOutCMIO::~CameraOutCMIO()
{
}

QStringList CameraOutCMIO::webcams() const
{
    QStringList webcams;

    for (auto &device: this->m_ipcBridge.listDevices(false))
        webcams << QString::fromStdString(device);

    return webcams;
}

int CameraOutCMIO::streamIndex() const
{
    return this->m_streamIndex;
}

QString CameraOutCMIO::description(const QString &webcam) const
{
    for (auto &device: this->m_ipcBridge.listDevices(false)) {
        auto deviceId = QString::fromStdString(device);

        if (deviceId == webcam)
            return deviceId;
    }

    return {};
}

void CameraOutCMIO::writeFrame(const AkPacket &frame)
{
    if (this->m_curDevice.isEmpty())
        return;

    AkVideoPacket videoFrame(frame);

    VideoFormat format(videoFrame.caps().fourCC(),
                       videoFrame.caps().width(),
                       videoFrame.caps().height(),
                       qRound(videoFrame.caps().fps().value()));

    this->m_ipcBridge.write(this->m_curDevice.toStdString(),
                            format,
                            videoFrame.buffer().constData());
}

int CameraOutCMIO::maxCameras() const
{
    return MAX_CAMERAS;
}

QString CameraOutCMIO::createWebcam(const QString &description,
                                    const QString &password)
{
    Q_UNUSED(password)

    if (!QFileInfo(this->m_driverPath).exists())
        return QString();

    auto webcams = this->webcams();

    if (!webcams.isEmpty())
        return QString();

    QString plugin = QFileInfo(this->m_driverPath).fileName();
    QString dstPath = "/Library/CoreMediaIO/Plug-Ins/DAL";
    QString rm = "rm -rvf " + dstPath + "/" + plugin;
    QString cp = "cp -rvf '" + this->m_driverPath + "' " + dstPath;

    if (!this->sudo(rm + ";" + cp))
        return QString();

    AkVideoCaps caps(this->m_caps);

    this->m_ipcBridge.deviceCreate({{caps.fourCC(),
                                     caps.width(), caps.height(),
                                     qRound(caps.fps().value())}},
                                   description.toStdString());

    auto curWebcams = this->webcams();

    if (curWebcams != webcams)
        emit this->webcamsChanged(curWebcams);

    return curWebcams.isEmpty()? QString(): curWebcams.first();
}

bool CameraOutCMIO::changeDescription(const QString &webcam,
                                      const QString &description,
                                      const QString &password)
{
    Q_UNUSED(password)

    QStringList webcams = this->webcams();

    if (!webcams.contains(webcam))
        return false;

    this->m_ipcBridge.setDescription(webcam.toStdString(),
                                     description.toStdString());

    emit this->webcamsChanged(webcams);

    return true;
}

bool CameraOutCMIO::removeWebcam(const QString &webcam,
                                 const QString &password)
{
    Q_UNUSED(password)

    QStringList webcams = this->webcams();

    if (!webcams.contains(webcam))
        return false;

    QString plugin = QFileInfo(this->m_driverPath).fileName();
    QString dstPath = "/Library/CoreMediaIO/Plug-Ins/DAL";
    QString rm = "rm -rvf " + dstPath + "/" + plugin;

    if (!this->sudo(rm))
        return false;

    emit this->webcamsChanged(QStringList());

    return true;
}

bool CameraOutCMIO::removeAllWebcams(const QString &password)
{
    Q_UNUSED(password)

    for (const QString &webcam: this->webcams())
        this->removeWebcam(webcam, password);

    return true;
}

bool CameraOutCMIO::sudo(const QString &command) const
{
    QProcess su;
    su.start("osascript",
             {"-e",
              "do shell script \""
              + command
              + "\" with administrator privileges"});
    su.waitForFinished(-1);

    if (su.exitCode()) {
        QByteArray outMsg = su.readAllStandardOutput();

        if (!outMsg.isEmpty())
            qDebug() << outMsg.toStdString().c_str();

        QByteArray errorMsg = su.readAllStandardError();

        if (!errorMsg.isEmpty())
            qDebug() << errorMsg.toStdString().c_str();

        return false;
    }

    return true;
}

bool CameraOutCMIO::init(int streamIndex)
{
    if (!this->m_ipcBridge.deviceStart(this->m_device.toStdString()))
        return false;

    this->m_streamIndex = streamIndex;
    this->m_curDevice = this->m_device;

    return true;
}

void CameraOutCMIO::uninit()
{
    if (this->m_curDevice.isEmpty())
        return;

    this->m_ipcBridge.deviceStop(this->m_curDevice.toStdString());
    this->m_streamIndex = -1;
    this->m_curDevice.clear();
}

void CameraOutCMIO::resetDriverPath()
{
    QDir applicationDir(QCoreApplication::applicationDirPath());
    this->setDriverPath(applicationDir.absoluteFilePath(VCAM_DRIVER));
}
