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
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QDomDocument>
#include <akvideocaps.h>
#include <akvideopacket.h>

#include "cameraoutcmio.h"
#include "ipcbridge.h"
#include "../Assistant/src/assistantglobals.h"
#include "VCamUtils/src/image/videoframe.h"

#define MAX_CAMERAS 1

Q_GLOBAL_STATIC_WITH_ARGS(QString,
                          akVCamDriver,
                          (QString("../Resources/%1.plugin").arg(CMIO_PLUGIN_NAME)))

class CameraOutCMIOPrivate
{
    public:
        QStringList m_webcams;
        QString m_curDevice;
        AkVCam::IpcBridge m_ipcBridge;
        int m_streamIndex;

        CameraOutCMIOPrivate():
            m_streamIndex(-1)
        {

        }

        inline bool sudo(const QString &command) const;
        inline QString readDaemonPlist() const;
};

CameraOutCMIO::CameraOutCMIO(QObject *parent):
    CameraOut(parent)
{
    this->d = new CameraOutCMIOPrivate;
    QDir applicationDir(QCoreApplication::applicationDirPath());
    this->m_driverPath = applicationDir.absoluteFilePath(*akVCamDriver());

    auto daemon =
            QString("%1/%2.plist")
            .arg(CMIO_DAEMONS_PATH)
            .arg(AKVCAM_ASSISTANT_NAME)
            .replace("~", QDir::homePath());

    if (QFileInfo::exists(daemon))
        this->d->m_ipcBridge.registerEndPoint(false);
}

CameraOutCMIO::~CameraOutCMIO()
{
    this->d->m_ipcBridge.unregisterEndPoint();
    delete this->d;
}

QStringList CameraOutCMIO::webcams() const
{
    QStringList webcams;

    for (auto &device: this->d->m_ipcBridge.listDevices(false))
        webcams << QString::fromStdString(device);

    return webcams;
}

int CameraOutCMIO::streamIndex() const
{
    return this->d->m_streamIndex;
}

QString CameraOutCMIO::description(const QString &webcam) const
{
    for (auto &device: this->d->m_ipcBridge.listDevices(false)) {
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
                                                  videoFrame.buffer().size()));
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
    QString dstPath = CMIO_PLUGINS_DAL_PATH;
    QString pluginInstallPath = dstPath + "/" + plugin;

    if (!QFileInfo(pluginInstallPath).exists()) {
        QString cp = "cp -rvf '" + this->m_driverPath + "' " + dstPath;

        if (!this->d->sudo(cp))
            return QString();
    }

    auto daemonPlist = this->d->readDaemonPlist();
    auto daemonPlistFile = QFileInfo(daemonPlist).fileName();
    auto daemonsPath = QString(CMIO_DAEMONS_PATH).replace("~", QDir::homePath());
    auto dstDaemonsPath = QDir(daemonsPath).absoluteFilePath(daemonPlistFile);

    if (!QFileInfo(dstDaemonsPath).exists()) {
        QDir().mkpath(daemonsPath);
        QFile::copy(daemonPlist, dstDaemonsPath);
    }

    AkVideoCaps caps(this->m_caps);

    this->d->m_ipcBridge.deviceCreate(description.isEmpty()?
                                          "AvKys Virtual Camera":
                                          description.toStdString(),
                                      {{caps.fourCC(),
                                        caps.width(), caps.height(),
                                        {caps.fps().value()}}});

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

    this->d->m_ipcBridge.deviceDestroy(webcam.toStdString());
    AkVideoCaps caps(this->m_caps);

    this->d->m_ipcBridge.deviceCreate(description.isEmpty()?
                                          "AvKys Virtual Camera":
                                          description.toStdString(),
                                      {{caps.fourCC(),
                                        caps.width(), caps.height(),
                                        {caps.fps().value()}}});

    auto curWebcams = this->webcams();

    if (curWebcams != webcams)
        emit this->webcamsChanged(curWebcams);

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
    QString dstPath = CMIO_PLUGINS_DAL_PATH;
    QString rm = "rm -rvf " + dstPath + "/" + plugin;

    if (!this->d->sudo(rm))
        return false;

    QDir(QString(CMIO_DAEMONS_PATH).replace("~", QDir::homePath()))
            .remove(QString("%1.plist").arg(AKVCAM_ASSISTANT_NAME));
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

bool CameraOutCMIOPrivate::sudo(const QString &command) const
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

QString CameraOutCMIOPrivate::readDaemonPlist() const
{
    QFile daemonFile(":/VirtualCameraCMIO/daemon.plist");
    daemonFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QDomDocument daemonXml;
    daemonXml.setContent(&daemonFile);
    auto daemonDict = daemonXml.documentElement().firstChildElement();
    auto keys = daemonDict.childNodes();
    auto assistantPath =
            QString("%1/%2.plugin/Contents/Resources/%3")
                .arg(CMIO_PLUGINS_DAL_PATH)
                .arg(CMIO_PLUGIN_NAME)
                .arg(CMIO_PLUGIN_ASSISTANT_NAME);

    for (int i = 0; i < keys.size(); i++) {
        if (keys.item(i).nodeName() == "key") {
            auto keyElement = keys.item(i).toElement();
            auto valueElement = keyElement.nextSiblingElement();

            if (keyElement.text() == "Label") {
                if (valueElement.childNodes().isEmpty())
                    valueElement.appendChild(daemonXml.createTextNode(AKVCAM_ASSISTANT_NAME));
                else
                    valueElement.replaceChild(daemonXml.createTextNode(AKVCAM_ASSISTANT_NAME),
                                              valueElement.firstChild());
            } else if (keyElement.text() == "ProgramArguments") {
                auto stringChilds = valueElement.elementsByTagName("string");

                for (int i = 0; i < stringChilds.size(); i++) {
                    auto item = stringChilds.item(i);

                    if (item.childNodes().isEmpty())
                        item.appendChild(daemonXml.createTextNode(assistantPath));
                    else
                        item.replaceChild(daemonXml.createTextNode(assistantPath),
                                          item.firstChild());
                }
            } else if (keyElement.text() == "MachServices") {
                auto stringChilds = valueElement.elementsByTagName("key");

                for (int i = 0; i < stringChilds.size(); i++) {
                    auto item = stringChilds.item(i);

                    if (item.childNodes().isEmpty())
                        item.appendChild(daemonXml.createTextNode(AKVCAM_ASSISTANT_NAME));
                    else
                        item.replaceChild(daemonXml.createTextNode(AKVCAM_ASSISTANT_NAME),
                                          item.firstChild());
                }
            }
        }
    }

#ifdef QT_DEBUG
    auto daemonLog = QString("/tmp/%1.log").arg(AKVCAM_ASSISTANT_NAME);

    // StandardOutPath key
    auto keyStandardOutPath = daemonXml.createElement("key");
    daemonDict.appendChild(keyStandardOutPath);
    keyStandardOutPath.appendChild(daemonXml.createTextNode("StandardOutPath"));

    // StandardOutPath value
    auto valueStandardOutPath = daemonXml.createElement("string");
    daemonDict.appendChild(valueStandardOutPath);
    valueStandardOutPath.appendChild(daemonXml.createTextNode(daemonLog));

    // StandardErrorPath key
    auto keyStandardErrorPath = daemonXml.createElement("key");
    daemonDict.appendChild(keyStandardErrorPath);
    keyStandardErrorPath.appendChild(daemonXml.createTextNode("StandardErrorPath"));

    // StandardErrorPath value
    auto valueStandardErrorPath = daemonXml.createElement("string");
    daemonDict.appendChild(valueStandardErrorPath);
    valueStandardErrorPath.appendChild(daemonXml.createTextNode(daemonLog));
#endif

    auto tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QFile plistFile(tempDir + "/" + AKVCAM_ASSISTANT_NAME + ".plist");
    plistFile.open(QIODevice::ReadWrite | QIODevice::Text);
    plistFile.write(daemonXml.toString().toUtf8());
    plistFile.close();

    return plistFile.fileName();
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

void CameraOutCMIO::resetDriverPath()
{
    QDir applicationDir(QCoreApplication::applicationDirPath());
    this->setDriverPath(applicationDir.absoluteFilePath(*akVCamDriver()));
}

#include "moc_cameraoutcmio.cpp"
