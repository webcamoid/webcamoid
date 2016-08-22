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

#include <QProcess>

#include "cameraout.h"

#define MAX_CAMERAS 64

#ifndef ROOT_METHOD
#define ROOT_METHOD "su"
#endif

typedef QMap<CameraOut::RootMethod, QString> RootMethodMap;

inline RootMethodMap initRootMethodMap()
{
    RootMethodMap rootMethodToStr;
    rootMethodToStr[CameraOut::RootMethodSu] = "su";
    rootMethodToStr[CameraOut::RootMethodSudo] = "sudo";

    return rootMethodToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(RootMethodMap, rootMethodToStr, (initRootMethodMap()))

typedef QMap<AkVideoCaps::PixelFormat, quint32> V4l2PixFmtMap;

inline V4l2PixFmtMap initV4l2PixFmtMap()
{
    V4l2PixFmtMap ffToV4L2;

    // RGB formats
    ffToV4L2[AkVideoCaps::Format_rgb444le] = V4L2_PIX_FMT_RGB444;
    ffToV4L2[AkVideoCaps::Format_rgb555le] = V4L2_PIX_FMT_RGB555;
    ffToV4L2[AkVideoCaps::Format_rgb565le] = V4L2_PIX_FMT_RGB565;
    ffToV4L2[AkVideoCaps::Format_rgb555be] = V4L2_PIX_FMT_RGB555X;
    ffToV4L2[AkVideoCaps::Format_rgb565be] = V4L2_PIX_FMT_RGB565X;
    ffToV4L2[AkVideoCaps::Format_bgr24] = V4L2_PIX_FMT_BGR24;
    ffToV4L2[AkVideoCaps::Format_rgb24] = V4L2_PIX_FMT_RGB24;
    ffToV4L2[AkVideoCaps::Format_bgr0] = V4L2_PIX_FMT_BGR32;
    ffToV4L2[AkVideoCaps::Format_0rgb] = V4L2_PIX_FMT_RGB32;

    // Grey formats
    ffToV4L2[AkVideoCaps::Format_gray] = V4L2_PIX_FMT_GREY;
    ffToV4L2[AkVideoCaps::Format_gray16le] = V4L2_PIX_FMT_Y16;

    // Luminance+Chrominance formats
    ffToV4L2[AkVideoCaps::Format_yuv410p] = V4L2_PIX_FMT_YVU410;
    ffToV4L2[AkVideoCaps::Format_yuv420p] = V4L2_PIX_FMT_YVU420;
    ffToV4L2[AkVideoCaps::Format_yuyv422] = V4L2_PIX_FMT_YUYV;
    ffToV4L2[AkVideoCaps::Format_yuv422p] = V4L2_PIX_FMT_YYUV;
    ffToV4L2[AkVideoCaps::Format_uyvy422] = V4L2_PIX_FMT_UYVY;
    ffToV4L2[AkVideoCaps::Format_yuv422p] = V4L2_PIX_FMT_VYUY;
    ffToV4L2[AkVideoCaps::Format_yuv422p] = V4L2_PIX_FMT_YUV422P;
    ffToV4L2[AkVideoCaps::Format_yuv411p] = V4L2_PIX_FMT_YUV411P;
    ffToV4L2[AkVideoCaps::Format_yuv411p] = V4L2_PIX_FMT_Y41P;
    ffToV4L2[AkVideoCaps::Format_yuv410p] = V4L2_PIX_FMT_YUV410;
    ffToV4L2[AkVideoCaps::Format_yuv420p] = V4L2_PIX_FMT_YUV420;

    // two planes -- one Y, one Cr + Cb interleaved
    ffToV4L2[AkVideoCaps::Format_nv12] = V4L2_PIX_FMT_NV12;
    ffToV4L2[AkVideoCaps::Format_nv21] = V4L2_PIX_FMT_NV21;
    ffToV4L2[AkVideoCaps::Format_nv16] = V4L2_PIX_FMT_NV16;

    // Bayer formats
    ffToV4L2[AkVideoCaps::Format_bayer_bggr8] = V4L2_PIX_FMT_SBGGR8;
    ffToV4L2[AkVideoCaps::Format_bayer_gbrg8] = V4L2_PIX_FMT_SGBRG8;
    ffToV4L2[AkVideoCaps::Format_bayer_grbg8] = V4L2_PIX_FMT_SGRBG8;
    ffToV4L2[AkVideoCaps::Format_bayer_rggb8] = V4L2_PIX_FMT_SRGGB8;

    // 10bit raw bayer, expanded to 16 bits
    ffToV4L2[AkVideoCaps::Format_bayer_bggr16le] = V4L2_PIX_FMT_SBGGR16;

    return ffToV4L2;
}

Q_GLOBAL_STATIC_WITH_ARGS(V4l2PixFmtMap, ffToV4L2, (initV4l2PixFmtMap()))

CameraOut::CameraOut(): QObject()
{
    this->m_streamIndex = -1;
    this->m_fd = -1;
    this->m_passwordTimeout = 2500;
    this->m_rootMethod = rootMethodToStr->key(ROOT_METHOD, RootMethodSu);
    this->m_webcams = this->webcams();
    this->m_fsWatcher = new QFileSystemWatcher(QStringList() << "/dev");
    this->m_fsWatcher->setParent(this);

    QObject::connect(this->m_fsWatcher,
                     &QFileSystemWatcher::directoryChanged,
                     this,
                     &CameraOut::onDirectoryChanged);
}

CameraOut::~CameraOut()
{
    delete this->m_fsWatcher;
}

QString CameraOut::driverPath() const
{
    return this->m_driverPath;
}

QStringList CameraOut::webcams() const
{
    QDir devicesDir("/dev");

    QStringList devices = devicesDir.entryList(QStringList() << "video*",
                                               QDir::System
                                               | QDir::Readable
                                               | QDir::Writable
                                               | QDir::NoSymLinks
                                               | QDir::NoDotAndDotDot
                                               | QDir::CaseSensitive,
                                               QDir::Name);

    QStringList webcams;
    QFile device;
    v4l2_capability capability;
    memset(&capability, 0, sizeof(v4l2_capability));

    foreach (QString devicePath, devices) {
        device.setFileName(devicesDir.absoluteFilePath(devicePath));

        if (device.open(QIODevice::ReadWrite)) {
            this->xioctl(device.handle(), VIDIOC_QUERYCAP, &capability);

            if (capability.capabilities & V4L2_CAP_VIDEO_OUTPUT)
                webcams << device.fileName();

            device.close();
        }
    }

    return webcams;
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
    if (webcam.isEmpty())
        return QString();

    QFile device;
    v4l2_capability capability;
    memset(&capability, 0, sizeof(v4l2_capability));

    device.setFileName(webcam);

    if (device.open(QIODevice::ReadWrite)) {
        this->xioctl(device.handle(), VIDIOC_QUERYCAP, &capability);

        if (capability.capabilities & V4L2_CAP_VIDEO_OUTPUT)
            return QString(reinterpret_cast<const char *>(capability.card));

        device.close();
    }

    return QString();
}

void CameraOut::writeFrame(const AkPacket &frame)
{
    if (write(this->m_fd,
              frame.buffer().constData(),
              size_t(frame.buffer().size())) < 0)
        qDebug() << "Error writing frame";
}

int CameraOut::maxCameras() const
{
    QString modules = QString("/lib/modules/%1/modules.dep")
                      .arg(QSysInfo::kernelVersion());

    QFile file(modules);

    if (!file.open(QIODevice::ReadOnly | QIODevice::ReadOnly))
        return 0;

    forever {
        QByteArray line = file.readLine();

        if (line.isEmpty())
            break;

        QString module = QFileInfo(line.left(line.indexOf(':'))).baseName();

        if (module == "v4l2loopback") {
            file.close();

            return MAX_CAMERAS;
        }
    }

    file.close();

    return 0;
}

bool CameraOut::needRoot() const
{
    return true;
}

int CameraOut::passwordTimeout() const
{
    return this->m_passwordTimeout;
}

QString CameraOut::rootMethod() const
{
    return rootMethodToStr->value(this->m_rootMethod);
}

QString CameraOut::createWebcam(const QString &description,
                                const QString &password) const
{
    if (password.isEmpty())
        return QString();

    QStringList webcams = this->webcams();
    QStringList webcamDescriptions;
    QStringList webcamIds;

    foreach (QString webcam, webcams) {
        webcamDescriptions << this->description(webcam);
        int id = webcam.indexOf(QRegExp("[0-9]+"));
        webcamIds << webcam.mid(id);
    }

    int id = 0;

    for (; QFileInfo::exists(QString("/dev/video%1").arg(id)); id++) {
    }

    QString deviceDescription;

    if (description.isEmpty())
        deviceDescription = QString(tr("Virtual Camera %1")).arg(id);
    else
        deviceDescription = description;

    webcamDescriptions << deviceDescription;
    webcamIds << QString("%1").arg(id);

    this->sudo("rmmod", {"v4l2loopback"}, password);

    if (!this->sudo("modprobe",
                    {"v4l2loopback",
                     QString("video_nr=%1").arg(webcamIds.join(',')),
                     QString("card_label=%1").arg(webcamDescriptions.join(','))},
                    password))
        return QString();

    QStringList curWebcams = this->webcams();

    if (curWebcams != webcams)
        emit this->webcamsChanged(curWebcams);

    return QString("/dev/video%1").arg(id);
}

bool CameraOut::changeDescription(const QString &webcam,
                                  const QString &description,
                                  const QString &password) const
{
    if (password.isEmpty())
        return false;

    if (!QRegExp("/dev/video[0-9]+").exactMatch(webcam))
        return false;

    QStringList webcams = this->webcams();

    if (webcams.isEmpty()
        || !webcams.contains(webcam))
        return false;

    QStringList webcamDescriptions;
    QStringList webcamIds;

    foreach (QString webcam, webcams) {
        webcamDescriptions << this->description(webcam);
        int id = webcam.indexOf(QRegExp("[0-9]+"));
        webcamIds << webcam.mid(id);
    }

    int id = webcam.indexOf(QRegExp("[0-9]+"));
    bool ok = false;
    id = webcam.mid(id).toInt(&ok);

    if (!ok)
        return false;

    QString deviceDescription;

    if (description.isEmpty())
        deviceDescription = QString(tr("Virtual Camera %1")).arg(id);
    else
        deviceDescription = description;

    int index = webcamIds.indexOf(QString("%1").arg(id));

    if (index < 0)
        return false;

    webcamDescriptions[index] = deviceDescription;
    this->sudo("rmmod", {"v4l2loopback"}, password);

    if (!this->sudo("modprobe",
                    {"v4l2loopback",
                     QString("video_nr=%1").arg(webcamIds.join(',')),
                     QString("card_label=%1").arg(webcamDescriptions.join(','))},
                    password))
        return false;

    QStringList curWebcams = this->webcams();

    if (curWebcams != webcams)
        emit this->webcamsChanged(curWebcams);

    return true;
}

bool CameraOut::removeWebcam(const QString &webcam,
                             const QString &password) const
{
    if (password.isEmpty())
        return false;

    if (!QRegExp("/dev/video[0-9]+").exactMatch(webcam))
        return false;

    QStringList webcams = this->webcams();

    if (webcams.isEmpty()
        || !webcams.contains(webcam))
        return false;

    QStringList webcamDescriptions;
    QStringList webcamIds;

    foreach (QString webcam, webcams) {
        webcamDescriptions << this->description(webcam);
        int id = webcam.indexOf(QRegExp("[0-9]+"));
        webcamIds << webcam.mid(id);
    }

    int id = webcam.indexOf(QRegExp("[0-9]+"));
    bool ok = false;
    id = webcam.mid(id).toInt(&ok);

    if (!ok)
        return false;

    int index = webcamIds.indexOf(QString("%1").arg(id));

    if (index < 0)
        return false;

    webcamDescriptions.removeAt(index);
    webcamIds.removeAt(index);

    this->sudo("rmmod", {"v4l2loopback"}, password);

    if (!webcamIds.isEmpty()) {
        if (!this->sudo("modprobe",
                        {"v4l2loopback",
                         QString("video_nr=%1").arg(webcamIds.join(',')),
                         QString("card_label=%1").arg(webcamDescriptions.join(','))},
                        password))
            return false;
    }

    QStringList curWebcams = this->webcams();

    if (curWebcams != webcams)
        emit this->webcamsChanged(curWebcams);

    return true;
}

bool CameraOut::removeAllWebcams(const QString &password) const
{
    if (password.isEmpty())
        return false;

    QStringList webcams = this->webcams();

    if (webcams.isEmpty())
        return false;

    this->sudo("rmmod", {"v4l2loopback"}, password);

    QStringList curWebcams = this->webcams();

    if (curWebcams != webcams)
        emit this->webcamsChanged(curWebcams);

    return true;
}

bool CameraOut::sudo(const QString &command,
                     const QStringList &argumments,
                     const QString &password) const
{
    if (password.isEmpty())
        return false;

    QProcess echo;
    QProcess su;

    echo.setStandardOutputProcess(&su);

    switch (this->m_rootMethod) {
        case RootMethodSu: {
            QStringList args;

            foreach (QString arg, argumments)
                args << arg.replace(" ", "\\ ");

            echo.start("echo", {password});
            su.start("su", {"-c", command + " " + args.join(" ")});

            break;
        }
        case RootMethodSudo: {
            echo.start("echo", {password});
            su.start("sudo", QStringList{"-S", command} << argumments);

            break;
        }
    }

    su.setProcessChannelMode(QProcess::ForwardedChannels);
    echo.waitForStarted();

    if (!su.waitForFinished(this->m_passwordTimeout)) {
        su.kill();
        echo.waitForFinished();

        return false;
    }

    echo.waitForFinished();

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

bool CameraOut::init(int streamIndex, const AkCaps &caps)
{
    this->m_fd = open(this->m_device.toStdString().c_str(), O_RDWR | O_NONBLOCK);

    if (this->m_fd < 0) {
        emit this->error(QString("Unable to open V4L2 device %1").arg(this->m_device));

        return false;
    }

    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    if (this->xioctl(this->m_fd, VIDIOC_G_FMT, &fmt) < 0) {
        emit this->error("Can't read default format");
        close(this->m_fd);
        this->m_fd = -1;

        return false;
    }

    AkVideoCaps videoCaps(caps);
    fmt.fmt.pix.width = __u32(videoCaps.width());
    fmt.fmt.pix.height = __u32(videoCaps.height());
    fmt.fmt.pix.pixelformat = ffToV4L2->value(videoCaps.format());
    fmt.fmt.pix.sizeimage = __u32(videoCaps.pictureSize());

    if (this->xioctl(this->m_fd, VIDIOC_S_FMT, &fmt) < 0) {
        emit this->error("Can't set format");
        close(this->m_fd);
        this->m_fd = -1;

        return false;
    }

    this->m_streamIndex = streamIndex;
    this->m_caps = caps;

    return true;
}

void CameraOut::uninit()
{
    close(this->m_fd);
    this->m_fd = -1;
}

void CameraOut::setDriverPath(const QString &driverPath)
{
    if (this->m_driverPath == driverPath)
        return;

    this->m_driverPath = driverPath;
    emit this->driverPathChanged(driverPath);
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

void CameraOut::setRootMethod(const QString &rootMethod)
{
    RootMethod methodEnum = rootMethodToStr->key(rootMethod, RootMethodSu);

    if (this->m_rootMethod == methodEnum)
        return;

    this->m_rootMethod = methodEnum;
    emit this->rootMethodChanged(rootMethod);
}

void CameraOut::resetDriverPath()
{
    this->setDriverPath("");
}

void CameraOut::resetDevice()
{
    this->setDevice("");
}

void CameraOut::resetPasswordTimeout()
{
    this->setPasswordTimeout(2500);
}

void CameraOut::resetRootMethod()
{
    this->setRootMethod(ROOT_METHOD);
}

void CameraOut::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path)

    QStringList webcams = this->webcams();

    if (webcams != this->m_webcams) {
        emit this->webcamsChanged(webcams);

        this->m_webcams = webcams;
    }
}
