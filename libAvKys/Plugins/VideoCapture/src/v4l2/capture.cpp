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

#include "capture.h"

typedef QMap<v4l2_ctrl_type, QString> V4l2CtrlTypeMap;

inline V4l2CtrlTypeMap initV4l2CtrlTypeMap()
{
    V4l2CtrlTypeMap ctrlTypeToStr;

    // V4L2 controls
    ctrlTypeToStr[V4L2_CTRL_TYPE_INTEGER] = "integer";
    ctrlTypeToStr[V4L2_CTRL_TYPE_BOOLEAN] = "boolean";
    ctrlTypeToStr[V4L2_CTRL_TYPE_MENU] = "menu";
    ctrlTypeToStr[V4L2_CTRL_TYPE_BUTTON] = "button";
    ctrlTypeToStr[V4L2_CTRL_TYPE_INTEGER64] = "integer64";
    ctrlTypeToStr[V4L2_CTRL_TYPE_CTRL_CLASS] = "ctrlClass";
    ctrlTypeToStr[V4L2_CTRL_TYPE_STRING] = "string";
    ctrlTypeToStr[V4L2_CTRL_TYPE_BITMASK] = "bitmask";
    ctrlTypeToStr[V4L2_CTRL_TYPE_INTEGER_MENU] = "integerMenu";

    return ctrlTypeToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(V4l2CtrlTypeMap, ctrlTypeToStr, (initV4l2CtrlTypeMap()))

typedef QMap<Capture::IoMethod, QString> IoMethodMap;

inline IoMethodMap initIoMethodMap()
{
    IoMethodMap ioMethodToStr;
    ioMethodToStr[Capture::IoMethodReadWrite] = "readWrite";
    ioMethodToStr[Capture::IoMethodMemoryMap] = "memoryMap";
    ioMethodToStr[Capture::IoMethodUserPointer] = "userPointer";

    return ioMethodToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(IoMethodMap, ioMethodToStr, (initIoMethodMap()))

Capture::Capture(): QObject()
{
    this->m_id = -1;
    this->m_ioMethod = IoMethodUnknown;
    this->m_nBuffers = 32;
    this->m_webcams = this->webcams();
    this->m_device = this->m_webcams.value(0, "");
    this->m_fsWatcher = new QFileSystemWatcher(QStringList() << "/dev", this);
    this->m_fsWatcher->addPaths(this->m_webcams);

    QObject::connect(this->m_fsWatcher,
                     &QFileSystemWatcher::directoryChanged,
                     this,
                     &Capture::onDirectoryChanged);
    QObject::connect(this->m_fsWatcher,
                     &QFileSystemWatcher::fileChanged,
                     this,
                     &Capture::onFileChanged);
}

Capture::~Capture()
{
    delete this->m_fsWatcher;
}

QStringList Capture::webcams() const
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
            if (this->xioctl(device.handle(), VIDIOC_QUERYCAP, &capability) >= 0
                && capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
                v4l2_format fmt;
                memset(&fmt, 0, sizeof(v4l2_format));
                fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

                if (this->xioctl(device.handle(), VIDIOC_G_FMT, &fmt) >= 0)
                    webcams << device.fileName();
            }

            device.close();
        }
    }

    return webcams;
}

QString Capture::device() const
{
    return this->m_device;
}

QList<int> Capture::streams() const
{
    if (!this->m_streams.isEmpty())
        return this->m_streams;

    QFile device(this->m_device);

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return QList<int>();

    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (this->xioctl(device.handle(), VIDIOC_G_FMT, &fmt) < 0) {
        qDebug() << "VideoCapture: Can't get default input format.";
        device.close();

        return QList<int>();
    }

    AkCaps videoCaps;
    videoCaps.setMimeType("video/unknown");
    videoCaps.setProperty("fourcc", this->fourccToStr(fmt.fmt.pix.pixelformat));
    videoCaps.setProperty("width", fmt.fmt.pix.width);
    videoCaps.setProperty("height", fmt.fmt.pix.height);
    videoCaps.setProperty("fps", this->fps(device.handle()).toString());

    device.close();

    QVariantList supportedCaps = this->caps(this->m_device);
    int stream = -1;

    for (int i = 0; i < supportedCaps.count(); i++)
        if (supportedCaps[i].value<AkCaps>() == videoCaps) {
            stream = i;

            break;
        }

    if (stream < 0)
        return QList<int>();

    QList<int> streams;
    streams << stream;

    return streams;
}

QList<int> Capture::listTracks(const QString &mimeType)
{
    if (mimeType != "video/x-raw"
        || !mimeType.isEmpty())
        return QList<int>();

    QVariantList caps = this->caps(this->m_device);
    QList<int> streams;

    for (int i = 0; i < caps.count(); i++)
        streams << i;

    return streams;
}

QString Capture::ioMethod() const
{
    return ioMethodToStr->value(this->m_ioMethod, "any");
}

int Capture::nBuffers() const
{
    return this->m_nBuffers;
}

QString Capture::description(const QString &webcam) const
{
    if (webcam.isEmpty())
        return QString();

    QFile device;
    v4l2_capability capability;
    memset(&capability, 0, sizeof(v4l2_capability));

    device.setFileName(webcam);

    if (device.open(QIODevice::ReadWrite)) {
        this->xioctl(device.handle(), VIDIOC_QUERYCAP, &capability);

        if (capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)
            return QString(reinterpret_cast<const char *>(capability.card));

        device.close();
    }

    return QString();
}

QVariantList Capture::capsFps(int fd, const struct v4l2_fmtdesc &format, __u32 width, __u32 height) const
{
    QVariantList caps;

    struct v4l2_frmivalenum frmival;
    memset(&frmival, 0, sizeof(frmival));
    frmival.pixel_format = format.pixelformat;
    frmival.width = width;
    frmival.height = height;

    for (frmival.index = 0;
         this->xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) >= 0;
         frmival.index++) {
        if (!frmival.discrete.numerator
            || !frmival.discrete.denominator)
            continue;

        AkCaps videoCaps;
        videoCaps.setMimeType("video/unknown");
        videoCaps.setProperty("fourcc", this->fourccToStr(format.pixelformat));
        videoCaps.setProperty("width", width);
        videoCaps.setProperty("height", height);
        AkFrac fps;

        if (frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE)
            fps = AkFrac(frmival.discrete.denominator, frmival.discrete.numerator);
        else
            fps = AkFrac(frmival.stepwise.min.denominator, frmival.stepwise.max.numerator);

        videoCaps.setProperty("fps", fps.toString());
        caps << QVariant::fromValue(videoCaps);
    }

    return caps;
}

QVariantList Capture::caps(const QString &webcam) const
{
    QFile device(webcam);
    QVariantList caps;

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return caps;

    QList<v4l2_buf_type> bufType;

    bufType << V4L2_BUF_TYPE_VIDEO_CAPTURE
            << V4L2_BUF_TYPE_VIDEO_OUTPUT
            << V4L2_BUF_TYPE_VIDEO_OVERLAY;

    foreach (v4l2_buf_type type, bufType) {
        v4l2_fmtdesc fmt;
        memset(&fmt, 0, sizeof(v4l2_fmtdesc));
        fmt.type = type;

        for (fmt.index = 0;
             this->xioctl(device.handle(), VIDIOC_ENUM_FMT, &fmt) >= 0;
             fmt.index++) {
            v4l2_frmsizeenum frmsize;
            memset(&frmsize, 0, sizeof(v4l2_frmsizeenum));
            frmsize.pixel_format = fmt.pixelformat;

            for (frmsize.index = 0;
                 this->xioctl(device.handle(), VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0;
                 frmsize.index++) {
                if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                    caps << this->capsFps(device.handle(),
                                          fmt,
                                          frmsize.discrete.width,
                                          frmsize.discrete.height);
                } else {/*
                    for (uint height = frmsize.stepwise.min_height;
                         height < frmsize.stepwise.max_height;
                         height += frmsize.stepwise.step_height)
                        for (uint width = frmsize.stepwise.min_width;
                             width < frmsize.stepwise.max_width;
                             width += frmsize.stepwise.step_width) {
                            caps << this->capsFps(device.handle(),
                                                  fmt,
                                                  width,
                                                  height);
                        }*/
                }
            }
        }
    }

    device.close();

    return caps;
}

QString Capture::capsDescription(const AkCaps &caps) const
{
    if (caps.mimeType() != "video/unknown")
        return QString();

    return QString("%1, %2x%3 %4 fps")
                .arg(caps.property("fourcc").toString())
                .arg(caps.property("width").toString())
                .arg(caps.property("height").toString())
                .arg(caps.property("fps").toString());
}

QVariantList Capture::imageControls() const
{
    return this->controls(this->m_device, V4L2_CTRL_CLASS_USER);
}

bool Capture::setImageControls(const QVariantMap &imageControls) const
{
    QVariantMap imageControlsDiff;

    foreach (QVariant control, this->imageControls()) {
        QVariantList params = control.toList();
        QString ctrlName = params[0].toString();

        if (imageControls.contains(ctrlName)
            && imageControls[ctrlName] != params[6]) {
            imageControlsDiff[ctrlName] = imageControls[ctrlName];
        }
    }

    if (imageControlsDiff.isEmpty())
        return false;

    QFile device(this->m_device);

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return false;

    QMap<QString, uint> ctrl2id = this->findControls(device.handle(), V4L2_CTRL_CLASS_USER);
    QVector<v4l2_ext_control> mpegCtrls;
    QVector<v4l2_ext_control> userCtrls;

    foreach (QString control, imageControlsDiff.keys()) {
        v4l2_ext_control ctrl;
        ctrl.id = ctrl2id[control];
        ctrl.value = imageControlsDiff[control].toInt();

        if (V4L2_CTRL_ID2CLASS(ctrl.id) == V4L2_CTRL_CLASS_MPEG)
            mpegCtrls << ctrl;
        else
            userCtrls << ctrl;
    }

    foreach (v4l2_ext_control user_ctrl, userCtrls) {
        v4l2_control ctrl;
        memset(&ctrl, 0, sizeof(v4l2_control));
        ctrl.id = user_ctrl.id;
        ctrl.value = user_ctrl.value;
        this->xioctl(device.handle(), VIDIOC_S_CTRL, &ctrl);
    }

    if (!mpegCtrls.isEmpty()) {
        v4l2_ext_controls ctrls;
        memset(&ctrls, 0, sizeof(v4l2_ext_control));
        ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
        ctrls.count = __u32(mpegCtrls.size());
        ctrls.controls = &mpegCtrls[0];
        this->xioctl(device.handle(), VIDIOC_S_EXT_CTRLS, &ctrls);
    }

    device.close();

    emit this->imageControlsChanged(imageControlsDiff);

    return true;
}

bool Capture::resetImageControls() const
{
    QVariantMap controls;

    foreach (QVariant control, this->imageControls()) {
        QVariantList params = control.toList();

        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setImageControls(controls);
}

QVariantList Capture::cameraControls() const
{
    return this->controls(this->m_device, V4L2_CTRL_CLASS_CAMERA);
}

bool Capture::setCameraControls(const QVariantMap &cameraControls) const
{
    QVariantMap cameraControlsDiff;

    foreach (QVariant control, this->cameraControls()) {
        QVariantList params = control.toList();
        QString ctrlName = params[0].toString();

        if (cameraControls.contains(ctrlName)
            && cameraControls[ctrlName] != params[6]) {
            cameraControlsDiff[ctrlName] = cameraControls[ctrlName];
        }
    }

    if (cameraControlsDiff.isEmpty())
        return false;

    QFile device(this->m_device);

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return false;

    QMap<QString, uint> ctrl2id = this->findControls(device.handle(), V4L2_CTRL_CLASS_CAMERA);
    QVector<v4l2_ext_control> mpegCtrls;
    QVector<v4l2_ext_control> userCtrls;

    foreach (QString control, cameraControlsDiff.keys()) {
        v4l2_ext_control ctrl;
        ctrl.id = ctrl2id[control];
        ctrl.value = cameraControlsDiff[control].toInt();

        if (V4L2_CTRL_ID2CLASS(ctrl.id) == V4L2_CTRL_CLASS_MPEG)
            mpegCtrls << ctrl;
        else
            userCtrls << ctrl;
    }

    foreach (v4l2_ext_control user_ctrl, userCtrls) {
        v4l2_control ctrl;
        memset(&ctrl, 0, sizeof(v4l2_control));
        ctrl.id = user_ctrl.id;
        ctrl.value = user_ctrl.value;
        this->xioctl(device.handle(), VIDIOC_S_CTRL, &ctrl);
    }

    if (!mpegCtrls.isEmpty()) {
        v4l2_ext_controls ctrls;
        memset(&ctrls, 0, sizeof(v4l2_ext_control));
        ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
        ctrls.count = __u32(mpegCtrls.size());
        ctrls.controls = &mpegCtrls[0];
        this->xioctl(device.handle(), VIDIOC_S_EXT_CTRLS, &ctrls);
    }

    device.close();

    emit this->cameraControlsChanged(cameraControlsDiff);

    return true;
}

bool Capture::resetCameraControls() const
{
    QVariantMap controls;

    foreach (QVariant control, this->cameraControls()) {
        QVariantList params = control.toList();

        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(controls);
}

AkPacket Capture::readFrame()
{
    if (this->m_buffers.isEmpty())
        return AkPacket();

    if (!this->m_deviceFile.isOpen())
        return AkPacket();

    if (this->m_ioMethod == IoMethodReadWrite) {
        if (read(this->m_deviceFile.handle(), this->m_buffers[0].start, this->m_buffers[0].length) < 0)
            return AkPacket();

        timeval timestamp;
        gettimeofday(&timestamp, NULL);

        qint64 pts = qint64((timestamp.tv_sec
                             + 1e-6 * timestamp.tv_usec)
                            * this->m_fps.value());

        return this->processFrame(this->m_buffers[0].start,
                                  this->m_buffers[0].length,
                                  pts);
    } else if (this->m_ioMethod == IoMethodMemoryMap
               || this->m_ioMethod == IoMethodUserPointer) {
        v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = (this->m_ioMethod == IoMethodMemoryMap)?
                            V4L2_MEMORY_MMAP:
                            V4L2_MEMORY_USERPTR;

        if (this->xioctl(this->m_deviceFile.handle(), VIDIOC_DQBUF, &buffer) < 0)
            return AkPacket();

        if (buffer.index >= quint32(this->m_buffers.size()))
            return AkPacket();

        qint64 pts = qint64((buffer.timestamp.tv_sec
                             + 1e-6 * buffer.timestamp.tv_usec)
                            * this->m_fps.value());

        AkPacket packet = this->processFrame(this->m_buffers[int(buffer.index)].start,
                                             buffer.bytesused,
                                             pts);

        if (this->xioctl(this->m_deviceFile.handle(), VIDIOC_QBUF, &buffer) < 0)
            return AkPacket();

        return packet;
    }

    return AkPacket();
}

AkFrac Capture::fps(int fd) const
{
    AkFrac fps;
    v4l2_std_id stdId;

    if (this->xioctl(fd, VIDIOC_G_STD, &stdId) >= 0) {
        v4l2_standard standard;
        memset(&standard, 0, sizeof(standard));

        for (standard.index = 0;
             this->xioctl(fd, VIDIOC_ENUMSTD, &standard) == 0;
             standard.index++) {
            if (standard.id & stdId) {
                fps = AkFrac(standard.frameperiod.denominator,
                             standard.frameperiod.numerator);

                break;
            }
        }
    }

    v4l2_streamparm streamparm;
    memset(&streamparm, 0, sizeof(streamparm));
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (this->xioctl(fd, VIDIOC_G_PARM, &streamparm) >= 0) {
        if (streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)
            fps = AkFrac(streamparm.parm.capture.timeperframe.denominator,
                         streamparm.parm.capture.timeperframe.numerator);
    }

    return fps;
}

void Capture::setFps(int fd, const AkFrac &fps)
{
    v4l2_standard standard;
    memset(&standard, 0, sizeof(standard));

    for (standard.index = 0;
         this->xioctl(fd, VIDIOC_ENUMSTD, &standard) == 0;
         standard.index++) {
        AkFrac stdFps(standard.frameperiod.denominator,
                      standard.frameperiod.numerator);

        if (stdFps == fps) {
            this->xioctl(fd, VIDIOC_S_STD, &standard.id);

            break;
        }
    }

    v4l2_streamparm streamparm;
    memset(&streamparm, 0, sizeof(streamparm));
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (this->xioctl(fd, VIDIOC_G_PARM, &streamparm) >= 0)
        if (streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
            streamparm.parm.capture.timeperframe.numerator = __u32(fps.den());
            streamparm.parm.capture.timeperframe.denominator = __u32(fps.num());
            this->xioctl(fd, VIDIOC_S_PARM, &streamparm);
        }
}

QVariantList Capture::controls(const QString &webcam, quint32 controlClass) const
{
    QVariantList controls;

    QFile device(webcam);

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return controls;

    v4l2_queryctrl queryctrl;
    memset(&queryctrl, 0, sizeof(v4l2_queryctrl));
    queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;

    while (this->xioctl(device.handle(), VIDIOC_QUERYCTRL, &queryctrl) == 0) {
        QVariantList control = this->queryControl(device.handle(), controlClass, &queryctrl);

        if (!control.isEmpty())
            controls << QVariant(control);

        queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

    if (queryctrl.id != V4L2_CTRL_FLAG_NEXT_CTRL) {
        device.close();

        return controls;
    }

    for (__u32 id = V4L2_CID_USER_BASE; id < V4L2_CID_LASTP1; id++) {
        queryctrl.id = id;

        if (this->xioctl(device.handle(), VIDIOC_QUERYCTRL, &queryctrl) == 0) {
            QVariantList control = this->queryControl(device.handle(), controlClass, &queryctrl);

            if (!control.isEmpty())
                controls << QVariant(control);
        }
    }

    for (queryctrl.id = V4L2_CID_PRIVATE_BASE;
         this->xioctl(device.handle(), VIDIOC_QUERYCTRL, &queryctrl) == 0;
         queryctrl.id++) {
        QVariantList control = this->queryControl(device.handle(), controlClass, &queryctrl);

        if (!control.isEmpty())
            controls << QVariant(control);
    }

    device.close();

    return controls;
}

QVariantList Capture::queryControl(int handle, quint32 controlClass, v4l2_queryctrl *queryctrl) const
{
    if (queryctrl->flags & V4L2_CTRL_FLAG_DISABLED)
        return QVariantList();

    if (V4L2_CTRL_ID2CLASS(queryctrl->id) != controlClass)
        return QVariantList();

    v4l2_ext_control ext_ctrl;
    memset(&ext_ctrl, 0, sizeof(v4l2_ext_control));
    ext_ctrl.id = queryctrl->id;

    v4l2_ext_controls ctrls;
    memset(&ctrls, 0, sizeof(v4l2_ext_controls));
    ctrls.ctrl_class = V4L2_CTRL_ID2CLASS(queryctrl->id);
    ctrls.count = 1;
    ctrls.controls = &ext_ctrl;

    if (V4L2_CTRL_ID2CLASS(queryctrl->id) != V4L2_CTRL_CLASS_USER &&
        queryctrl->id < V4L2_CID_PRIVATE_BASE) {
        if (this->xioctl(handle, VIDIOC_G_EXT_CTRLS, &ctrls))
            return QVariantList();
    } else {
        v4l2_control ctrl;
        memset(&ctrl, 0, sizeof(v4l2_control));
        ctrl.id = queryctrl->id;

        if (this->xioctl(handle, VIDIOC_G_CTRL, &ctrl))
            return QVariantList();

        ext_ctrl.value = ctrl.value;
    }

    v4l2_querymenu qmenu;
    memset(&qmenu, 0, sizeof(v4l2_querymenu));
    qmenu.id = queryctrl->id;
    QStringList menu;

    if (queryctrl->type == V4L2_CTRL_TYPE_MENU)
        for (int i = 0; i < queryctrl->maximum + 1; i++) {
            qmenu.index = __u32(i);

            if (this->xioctl(handle, VIDIOC_QUERYMENU, &qmenu))
                continue;

            menu << QString(reinterpret_cast<const char *>(qmenu.name));
        }

    v4l2_ctrl_type type = static_cast<v4l2_ctrl_type>(queryctrl->type);

    return QVariantList() << QString(reinterpret_cast<const char *>(queryctrl->name))
                          << ctrlTypeToStr->value(type)
                          << queryctrl->minimum
                          << queryctrl->maximum
                          << queryctrl->step
                          << queryctrl->default_value
                          << ext_ctrl.value
                          << menu;
}

QMap<QString, quint32> Capture::findControls(int handle, quint32 controlClass) const
{
    v4l2_queryctrl qctrl;
    memset(&qctrl, 0, sizeof(v4l2_queryctrl));
    qctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    QMap<QString, quint32> controls;

    while (this->xioctl(handle, VIDIOC_QUERYCTRL, &qctrl) == 0) {
        if (!(qctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            && V4L2_CTRL_ID2CLASS(qctrl.id) == controlClass)
            controls[QString(reinterpret_cast<const char *>(qctrl.name))] = qctrl.id;

        qctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

    if (qctrl.id != V4L2_CTRL_FLAG_NEXT_CTRL)
        return controls;

    for (__u32 id = V4L2_CID_USER_BASE; id < V4L2_CID_LASTP1; id++) {
        qctrl.id = id;

        if (this->xioctl(handle, VIDIOC_QUERYCTRL, &qctrl) == 0
            && !(qctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            && V4L2_CTRL_ID2CLASS(qctrl.id) == controlClass)
            controls[QString(reinterpret_cast<const char *>(qctrl.name))] = qctrl.id;
    }

    qctrl.id = V4L2_CID_PRIVATE_BASE;

    while (this->xioctl(handle, VIDIOC_QUERYCTRL, &qctrl) == 0) {
        if (!(qctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            && V4L2_CTRL_ID2CLASS(qctrl.id) == controlClass)
            controls[QString(reinterpret_cast<const char *>(qctrl.name))] = qctrl.id;

        qctrl.id++;
    }

    return controls;
}

bool Capture::initReadWrite(quint32 bufferSize)
{
    this->m_buffers.resize(1);

    this->m_buffers[0].length = bufferSize;
    this->m_buffers[0].start = new char[bufferSize];

    if (!this->m_buffers[0].start) {
        this->m_buffers.clear();

        return false;
    }

    return true;
}

bool Capture::initMemoryMap()
{
    v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(requestBuffers));

    requestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requestBuffers.memory = V4L2_MEMORY_MMAP;
    requestBuffers.count = __u32(this->m_nBuffers);

    if (this->xioctl(this->m_deviceFile.handle(), VIDIOC_REQBUFS, &requestBuffers) < 0)
        return false;

    if (requestBuffers.count < 1)
        return false;

    this->m_buffers.resize(int(requestBuffers.count));
    bool error = false;

    for (int i = 0; i < int(requestBuffers.count); i++) {
        v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(buffer));

        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = __u32(i);

        if (this->xioctl(this->m_deviceFile.handle(), VIDIOC_QUERYBUF, &buffer) < 0) {
            error = true;

            break;
        }

        this->m_buffers[i].length = buffer.length;

        this->m_buffers[i].start = reinterpret_cast<char *>(mmap(NULL,
                                                                 buffer.length,
                                                                 PROT_READ | PROT_WRITE,
                                                                 MAP_SHARED,
                                                                 this->m_deviceFile.handle(),
                                                                 buffer.m.offset));

        if (this->m_buffers[i].start == MAP_FAILED) {
            error = true;

            break;
        }
    }

    if (error) {
        for (qint32 i = 0; i < this->m_buffers.size(); i++)
            munmap(this->m_buffers[i].start, this->m_buffers[i].length);

        this->m_buffers.clear();

        return false;
    }

    return true;
}

bool Capture::initUserPointer(quint32 bufferSize)
{
    v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(requestBuffers));

    requestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requestBuffers.memory = V4L2_MEMORY_USERPTR;
    requestBuffers.count = __u32(this->m_nBuffers);

    if (this->xioctl(this->m_deviceFile.handle(), VIDIOC_REQBUFS, &requestBuffers) < 0)
        return false;

    this->m_buffers.resize(int(requestBuffers.count));
    bool error = false;

    for (int i = 0; i < int(requestBuffers.count); i++) {
        this->m_buffers[i].length = bufferSize;
        this->m_buffers[i].start = new char[bufferSize];

        if (!this->m_buffers[i].start) {
            error = true;

            break;
        }
    }

    if (error) {
        for (qint32 i = 0; i < this->m_buffers.size(); i++)
            delete this->m_buffers[i].start;

        this->m_buffers.clear();

        return false;
    }

    return true;
}

bool Capture::startCapture()
{
    bool error = false;

    if (this->m_ioMethod == IoMethodMemoryMap) {
        for (int i = 0; i < this->m_buffers.size(); i++) {
            v4l2_buffer buffer;
            memset(&buffer, 0, sizeof(buffer));

            buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buffer.memory = V4L2_MEMORY_MMAP;
            buffer.index = __u32(i);

            if (this->xioctl(this->m_deviceFile.handle(), VIDIOC_QBUF, &buffer) < 0)
                error = true;
        }

        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (this->xioctl(this->m_deviceFile.handle(), VIDIOC_STREAMON, &type) < 0)
            error = true;
    } else if (this->m_ioMethod == IoMethodUserPointer) {
        for (int i = 0; i < this->m_buffers.size(); i++) {
            v4l2_buffer buffer;
            memset(&buffer, 0, sizeof(buffer));

            buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buffer.memory = V4L2_MEMORY_USERPTR;
            buffer.index = __u32(i);
            buffer.m.userptr = ulong(this->m_buffers[i].start);
            buffer.length = __u32(this->m_buffers[i].length);

            if (this->xioctl(this->m_deviceFile.handle(), VIDIOC_QBUF, &buffer) < 0)
                error = true;
        }

        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (this->xioctl(this->m_deviceFile.handle(), VIDIOC_STREAMON, &type) < 0)
            error = true;
    }

    if (error)
        this->uninit();

    this->m_id = Ak::id();

    return !error;
}

void Capture::stopCapture()
{
    if (this->m_ioMethod == IoMethodMemoryMap
        || this->m_ioMethod == IoMethodUserPointer) {
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        this->xioctl(this->m_deviceFile.handle(), VIDIOC_STREAMOFF, &type);
    }
}

bool Capture::init()
{
    this->m_deviceFile.setFileName(this->m_device);

    if (!this->m_deviceFile.open(QIODevice::ReadWrite))
        return false;

    v4l2_capability capabilities;
    memset(&capabilities, 0, sizeof(v4l2_capability));

    if (this->xioctl(this->m_deviceFile.handle(), VIDIOC_QUERYCAP, &capabilities) < 0) {
        qDebug() << "VideoCapture: Can't query capabilities.";
        this->m_deviceFile.close();

        return false;
    }

    QList<int> streams = this->streams();

    if (streams.isEmpty()) {
        qDebug() << "VideoCapture: No streams available.";
        this->m_deviceFile.close();

        return false;
    }

    QVariantList supportedCaps = this->caps(this->m_device);
    AkCaps caps = supportedCaps[streams[0]].value<AkCaps>();
    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (this->xioctl(this->m_deviceFile.handle(), VIDIOC_G_FMT, &fmt) == 0) {
        fmt.fmt.pix.pixelformat = this->strToFourCC(caps.property("fourcc").toString());
        fmt.fmt.pix.width = caps.property("width").toUInt();
        fmt.fmt.pix.height = caps.property("height").toUInt();

        if (this->xioctl(this->m_deviceFile.handle(), VIDIOC_S_FMT, &fmt) < 0) {
            qDebug() << "VideoCapture: Can't set format:" << this->fourccToStr(fmt.fmt.pix.pixelformat);
            this->m_deviceFile.close();

            return false;
        }
    }

    this->setFps(this->m_deviceFile.handle(), caps.property("fps").toString());

    if (this->xioctl(this->m_deviceFile.handle(), VIDIOC_S_FMT, &fmt) < 0) {
        qDebug() << "VideoCapture: Can't set format:" << this->fourccToStr(fmt.fmt.pix.pixelformat);
        this->m_deviceFile.close();

        return false;
    }

    this->m_caps = caps;
    this->m_fps = caps.property("fps").toString();
    this->m_timeBase = this->m_fps.invert();

    if (this->m_ioMethod == IoMethodReadWrite
        && capabilities.capabilities & V4L2_CAP_READWRITE
        && this->initReadWrite(fmt.fmt.pix.sizeimage)) {
    } else if (this->m_ioMethod == IoMethodMemoryMap
             && capabilities.capabilities & V4L2_CAP_STREAMING
             && this->initMemoryMap()) {
    } else if (this->m_ioMethod == IoMethodUserPointer
             && capabilities.capabilities & V4L2_CAP_STREAMING
             && this->initUserPointer(fmt.fmt.pix.sizeimage)) {
    } else
        this->m_ioMethod = IoMethodUnknown;

    if (this->m_ioMethod != IoMethodUnknown)
        return this->startCapture();

    if (capabilities.capabilities & V4L2_CAP_READWRITE && this->initReadWrite(fmt.fmt.pix.sizeimage))
        this->m_ioMethod = IoMethodReadWrite;
    else if (capabilities.capabilities & V4L2_CAP_STREAMING) {
        if (this->initMemoryMap())
            this->m_ioMethod = IoMethodMemoryMap;
        else if (this->initUserPointer(fmt.fmt.pix.sizeimage))
            this->m_ioMethod = IoMethodUserPointer;
        else {
            this->m_ioMethod = IoMethodUnknown;

            return false;
        }
    } else
        return false;

    return this->startCapture();
}

void Capture::uninit()
{
    this->stopCapture();

    if (!this->m_buffers.isEmpty()) {
        if (this->m_ioMethod == IoMethodReadWrite)
            delete this->m_buffers[0].start;
        else if (this->m_ioMethod == IoMethodMemoryMap)
            for (qint32 i = 0; i < this->m_buffers.size(); i++)
                munmap(this->m_buffers[i].start, this->m_buffers[i].length);
        else if (this->m_ioMethod == IoMethodUserPointer)
            for (qint32 i = 0; i < this->m_buffers.size(); i++)
                delete this->m_buffers[i].start;
    }

    this->m_deviceFile.close();
    this->m_caps.clear();
    this->m_fps = AkFrac();
    this->m_timeBase = AkFrac();
    this->m_buffers.clear();
}

void Capture::setDevice(const QString &device)
{
    if (this->m_device == device)
        return;

    this->m_device = device;
    emit this->deviceChanged(device);
}

void Capture::setStreams(const QList<int> &streams)
{
    if (streams.isEmpty())
        return;

    int stream = streams[0];

    if (stream < 0)
        return;

    QVariantList supportedCaps = this->caps(this->m_device);

    if (stream >= supportedCaps.length())
        return;

    QList<int> inputStreams;
    inputStreams << stream;

    if (this->streams() == inputStreams)
        return;

    this->m_streams = inputStreams;
    emit this->streamsChanged(inputStreams);
}

void Capture::setIoMethod(const QString &ioMethod)
{
    if (this->m_deviceFile.isOpen())
        return;

    IoMethod ioMethodEnum = ioMethodToStr->key(ioMethod, IoMethodUnknown);

    if (this->m_ioMethod == ioMethodEnum)
        return;

    this->m_ioMethod = ioMethodEnum;
    emit this->ioMethodChanged(ioMethod);
}

void Capture::setNBuffers(int nBuffers)
{
    if (this->m_nBuffers == nBuffers)
        return;

    this->m_nBuffers = nBuffers;
    emit this->nBuffersChanged(nBuffers);
}

void Capture::resetDevice()
{
    this->setDevice(this->m_webcams.value(0, ""));
}

void Capture::resetStreams()
{
    QVariantList supportedCaps = this->caps(this->m_device);
    QList<int> streams;

    if (!supportedCaps.isEmpty())
        streams << 0;

    this->setStreams(streams);
}

void Capture::resetIoMethod()
{
    this->setIoMethod("any");
}

void Capture::resetNBuffers()
{
    this->setNBuffers(32);
}

void Capture::reset()
{
    this->resetStreams();
    this->resetImageControls();
    this->resetCameraControls();
}

void Capture::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path)

    QStringList webcams = this->webcams();

    if (webcams != this->m_webcams) {
        emit this->webcamsChanged(webcams);

        this->m_fsWatcher->removePaths(this->m_webcams);
        this->m_webcams = webcams;
        this->m_fsWatcher->addPaths(webcams);
    }
}

void Capture::onFileChanged(const QString &fileName)
{
    Q_UNUSED(fileName)
}
