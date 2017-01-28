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

#include <QDir>

#include "capturev4l2.h"

typedef QMap<v4l2_ctrl_type, QString> V4l2CtrlTypeMap;

inline V4l2CtrlTypeMap initV4l2CtrlTypeMap()
{
    V4l2CtrlTypeMap ctrlTypeToStr = {
        // V4L2 controls
        {V4L2_CTRL_TYPE_INTEGER     , "integer"    },
        {V4L2_CTRL_TYPE_BOOLEAN     , "boolean"    },
        {V4L2_CTRL_TYPE_MENU        , "menu"       },
        {V4L2_CTRL_TYPE_BUTTON      , "button"     },
        {V4L2_CTRL_TYPE_INTEGER64   , "integer64"  },
        {V4L2_CTRL_TYPE_CTRL_CLASS  , "ctrlClass"  },
        {V4L2_CTRL_TYPE_STRING      , "string"     },
        {V4L2_CTRL_TYPE_BITMASK     , "bitmask"    },
        {V4L2_CTRL_TYPE_INTEGER_MENU, "integerMenu"}
    };

    return ctrlTypeToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(V4l2CtrlTypeMap, ctrlTypeToStr, (initV4l2CtrlTypeMap()))

typedef QMap<CaptureV4L2::IoMethod, QString> IoMethodMap;

inline IoMethodMap initIoMethodMap()
{
    IoMethodMap ioMethodToStr = {
        {CaptureV4L2::IoMethodReadWrite  , "readWrite"  },
        {CaptureV4L2::IoMethodMemoryMap  , "memoryMap"  },
        {CaptureV4L2::IoMethodUserPointer, "userPointer"}
    };

    return ioMethodToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(IoMethodMap, ioMethodToStr, (initIoMethodMap()))

CaptureV4L2::CaptureV4L2(QObject *parent):
    Capture(parent)
{
    this->m_id = -1;
    this->m_ioMethod = IoMethodUnknown;
    this->m_nBuffers = 32;
    this->m_fsWatcher = new QFileSystemWatcher({"/dev"}, this);

    QObject::connect(this->m_fsWatcher,
                     &QFileSystemWatcher::directoryChanged,
                     this,
                     &CaptureV4L2::onDirectoryChanged);
    QObject::connect(this->m_fsWatcher,
                     &QFileSystemWatcher::fileChanged,
                     this,
                     &CaptureV4L2::onFileChanged);

    this->updateDevices();
}

CaptureV4L2::~CaptureV4L2()
{
    delete this->m_fsWatcher;
}

QStringList CaptureV4L2::webcams() const
{
    return this->m_devices;
}

QString CaptureV4L2::device() const
{
    return this->m_device;
}

QList<int> CaptureV4L2::streams() const
{
    if (!this->m_streams.isEmpty())
        return this->m_streams;

    QVariantList caps = this->caps(this->m_device);

    if (caps.isEmpty())
        return QList<int>();

    return QList<int> {0};
}

QList<int> CaptureV4L2::listTracks(const QString &mimeType)
{
    if (mimeType != "video/x-raw"
        && !mimeType.isEmpty())
        return QList<int>();

    QVariantList caps = this->caps(this->m_device);
    QList<int> streams;

    for (int i = 0; i < caps.count(); i++)
        streams << i;

    return streams;
}

QString CaptureV4L2::ioMethod() const
{
    return ioMethodToStr->value(this->m_ioMethod, "any");
}

int CaptureV4L2::nBuffers() const
{
    return this->m_nBuffers;
}

QString CaptureV4L2::description(const QString &webcam) const
{
    return this->m_descriptions.value(webcam);
}

QVariantList CaptureV4L2::caps(const QString &webcam) const
{
    return this->m_devicesCaps.value(webcam);
}

QString CaptureV4L2::capsDescription(const AkCaps &caps) const
{
    if (caps.mimeType() != "video/unknown")
        return QString();

    AkFrac fps = caps.property("fps").toString();

    return QString("%1, %2x%3, %4 FPS")
                .arg(caps.property("fourcc").toString())
                .arg(caps.property("width").toString())
                .arg(caps.property("height").toString())
                .arg(qRound(fps.value()));
}

QVariantList CaptureV4L2::imageControls() const
{
    return this->m_imageControls.value(this->m_device);
}

bool CaptureV4L2::setImageControls(const QVariantMap &imageControls)
{
    QVariantMap imageControlsDiff;

    for (const QVariant &control: this->imageControls()) {
        auto params = control.toList();
        auto ctrlName = params[0].toString();

        if (imageControls.contains(ctrlName)
            && imageControls[ctrlName] != params[6]) {
            imageControlsDiff[ctrlName] = imageControls[ctrlName];
        }
    }

    if (imageControlsDiff.isEmpty())
        return false;

    int fd = -1;

    if (this->m_fd >= 0)
        fd = this->m_fd;
    else
        fd = x_open(this->m_device.toStdString().c_str(), O_RDWR | O_NONBLOCK, 0);

    if (!setControls(fd, V4L2_CTRL_CLASS_USER, imageControlsDiff))
        return false;

    if (this->m_fd < 0)
        x_close(fd);

    QVariantList controls;

    for (const auto &control: this->m_imageControls.value(this->m_device)) {
        auto controlParams = control.toList();
        auto controlName = controlParams[0].toString();

        if (imageControlsDiff.contains(controlName))
            controlParams[6] = imageControlsDiff[controlName];

        controls << QVariant(controlParams);
    }

    this->m_imageControls[this->m_device] = controls;
    emit this->imageControlsChanged(imageControlsDiff);

    return true;
}

bool CaptureV4L2::resetImageControls()
{
    QVariantMap controls;

    for (const QVariant &control: this->imageControls()) {
        QVariantList params = control.toList();

        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setImageControls(controls);
}

QVariantList CaptureV4L2::cameraControls() const
{
    return this->m_cameraControls.value(this->m_device);
}

bool CaptureV4L2::setCameraControls(const QVariantMap &cameraControls)
{
    QVariantMap cameraControlsDiff;

    for (const QVariant &control: this->cameraControls()) {
        auto params = control.toList();
        auto ctrlName = params[0].toString();

        if (cameraControls.contains(ctrlName)
            && cameraControls[ctrlName] != params[6]) {
            cameraControlsDiff[ctrlName] = cameraControls[ctrlName];
        }
    }

    if (cameraControlsDiff.isEmpty())
        return false;

    int fd = -1;

    if (this->m_fd >= 0)
        fd = this->m_fd;
    else
        fd = x_open(this->m_device.toStdString().c_str(), O_RDWR | O_NONBLOCK, 0);

    if (!setControls(fd, V4L2_CTRL_CLASS_CAMERA, cameraControlsDiff))
        return false;

    if (this->m_fd < 0)
        x_close(fd);

    QVariantList controls;

    for (const auto &control: this->m_cameraControls.value(this->m_device)) {
        auto controlParams = control.toList();
        auto controlName = controlParams[0].toString();

        if (cameraControlsDiff.contains(controlName))
            controlParams[6] = cameraControlsDiff[controlName];

        controls << QVariant(controlParams);
    }

    this->m_cameraControls[this->m_device] = controls;
    emit this->cameraControlsChanged(cameraControlsDiff);

    return true;
}

bool CaptureV4L2::resetCameraControls()
{
    QVariantMap controls;

    for (const QVariant &control: this->cameraControls()) {
        QVariantList params = control.toList();

        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(controls);
}

AkPacket CaptureV4L2::readFrame()
{
    if (this->m_buffers.isEmpty())
        return AkPacket();

    if (this->m_fd < 0)
        return AkPacket();

    if (this->m_ioMethod == IoMethodReadWrite) {
        if (x_read(this->m_fd,
                   this->m_buffers[0].start,
                   this->m_buffers[0].length) < 0)
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

        if (this->xioctl(this->m_fd,
                         VIDIOC_DQBUF,
                         &buffer) < 0)
            return AkPacket();

        if (buffer.index >= quint32(this->m_buffers.size()))
            return AkPacket();

        qint64 pts = qint64((buffer.timestamp.tv_sec
                             + 1e-6 * buffer.timestamp.tv_usec)
                            * this->m_fps.value());

        AkPacket packet =
                this->processFrame(this->m_buffers[int(buffer.index)].start,
                                   buffer.bytesused,
                                   pts);

        if (this->xioctl(this->m_fd, VIDIOC_QBUF, &buffer) < 0)
            return AkPacket();

        return packet;
    }

    return AkPacket();
}

QVariantList CaptureV4L2::capsFps(int fd,
                                  const struct v4l2_fmtdesc &format,
                                  __u32 width,
                                  __u32 height) const
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

AkFrac CaptureV4L2::fps(int fd) const
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

void CaptureV4L2::setFps(int fd, const AkFrac &fps)
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

QVariantList CaptureV4L2::controls(int fd, quint32 controlClass) const
{
    QVariantList controls;

    if (fd < 0)
        return controls;

    v4l2_queryctrl queryctrl;
    memset(&queryctrl, 0, sizeof(v4l2_queryctrl));
    queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;

    while (this->xioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
        QVariantList control = this->queryControl(fd, controlClass, &queryctrl);

        if (!control.isEmpty())
            controls << QVariant(control);

        queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

    if (queryctrl.id != V4L2_CTRL_FLAG_NEXT_CTRL)
        return controls;

    for (__u32 id = V4L2_CID_USER_BASE; id < V4L2_CID_LASTP1; id++) {
        queryctrl.id = id;

        if (this->xioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
            QVariantList control = this->queryControl(fd, controlClass, &queryctrl);

            if (!control.isEmpty())
                controls << QVariant(control);
        }
    }

    for (queryctrl.id = V4L2_CID_PRIVATE_BASE;
         this->xioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == 0;
         queryctrl.id++) {
        QVariantList control = this->queryControl(fd, controlClass, &queryctrl);

        if (!control.isEmpty())
            controls << QVariant(control);
    }

    return controls;
}

bool CaptureV4L2::setControls(int fd,
                              quint32 controlClass,
                              const QVariantMap &controls) const
{
    if (fd < 0)
        return false;

    auto ctrl2id = this->findControls(fd, controlClass);
    QVector<v4l2_ext_control> mpegCtrls;
    QVector<v4l2_ext_control> userCtrls;

    for (const QString &control: controls.keys()) {
        v4l2_ext_control ctrl;
        ctrl.id = ctrl2id[control];
        ctrl.value = controls[control].toInt();

        if (V4L2_CTRL_ID2CLASS(ctrl.id) == V4L2_CTRL_CLASS_MPEG)
            mpegCtrls << ctrl;
        else
            userCtrls << ctrl;
    }

    for (const v4l2_ext_control &user_ctrl: userCtrls) {
        v4l2_control ctrl;
        memset(&ctrl, 0, sizeof(v4l2_control));
        ctrl.id = user_ctrl.id;
        ctrl.value = user_ctrl.value;
        this->xioctl(fd, VIDIOC_S_CTRL, &ctrl);
    }

    if (!mpegCtrls.isEmpty()) {
        v4l2_ext_controls ctrls;
        memset(&ctrls, 0, sizeof(v4l2_ext_control));
        ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
        ctrls.count = __u32(mpegCtrls.size());
        ctrls.controls = &mpegCtrls[0];
        this->xioctl(fd, VIDIOC_S_EXT_CTRLS, &ctrls);
    }

    return true;
}

QVariantList CaptureV4L2::queryControl(int handle,
                                       quint32 controlClass,
                                       v4l2_queryctrl *queryctrl) const
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

    return QVariantList {
        QString(reinterpret_cast<const char *>(queryctrl->name)),
        ctrlTypeToStr->value(type),
        queryctrl->minimum,
        queryctrl->maximum,
        queryctrl->step,
        queryctrl->default_value,
        ext_ctrl.value,
        menu
    };
}

QMap<QString, quint32> CaptureV4L2::findControls(int handle,
                                                 quint32 controlClass) const
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

bool CaptureV4L2::initReadWrite(quint32 bufferSize)
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

bool CaptureV4L2::initMemoryMap()
{
    v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(requestBuffers));

    requestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requestBuffers.memory = V4L2_MEMORY_MMAP;
    requestBuffers.count = __u32(this->m_nBuffers);

    if (this->xioctl(this->m_fd, VIDIOC_REQBUFS, &requestBuffers) < 0)
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

        if (this->xioctl(this->m_fd, VIDIOC_QUERYBUF, &buffer) < 0) {
            error = true;

            break;
        }

        this->m_buffers[i].length = buffer.length;

        this->m_buffers[i].start = reinterpret_cast<char *>(x_mmap(NULL,
                                                                   buffer.length,
                                                                   PROT_READ | PROT_WRITE,
                                                                   MAP_SHARED,
                                                                   this->m_fd,
                                                                   buffer.m.offset));

        if (this->m_buffers[i].start == MAP_FAILED) {
            error = true;

            break;
        }
    }

    if (error) {
        for (qint32 i = 0; i < this->m_buffers.size(); i++)
            x_munmap(this->m_buffers[i].start, this->m_buffers[i].length);

        this->m_buffers.clear();

        return false;
    }

    return true;
}

bool CaptureV4L2::initUserPointer(quint32 bufferSize)
{
    v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(requestBuffers));

    requestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requestBuffers.memory = V4L2_MEMORY_USERPTR;
    requestBuffers.count = __u32(this->m_nBuffers);

    if (this->xioctl(this->m_fd,
                     VIDIOC_REQBUFS,
                     &requestBuffers) < 0)
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

bool CaptureV4L2::startCapture()
{
    bool error = false;

    if (this->m_ioMethod == IoMethodMemoryMap) {
        for (int i = 0; i < this->m_buffers.size(); i++) {
            v4l2_buffer buffer;
            memset(&buffer, 0, sizeof(buffer));

            buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buffer.memory = V4L2_MEMORY_MMAP;
            buffer.index = __u32(i);

            if (this->xioctl(this->m_fd, VIDIOC_QBUF, &buffer) < 0)
                error = true;
        }

        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (this->xioctl(this->m_fd, VIDIOC_STREAMON, &type) < 0)
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

            if (this->xioctl(this->m_fd, VIDIOC_QBUF, &buffer) < 0)
                error = true;
        }

        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (this->xioctl(this->m_fd, VIDIOC_STREAMON, &type) < 0)
            error = true;
    }

    if (error)
        this->uninit();

    this->m_id = Ak::id();

    return !error;
}

void CaptureV4L2::stopCapture()
{
    if (this->m_ioMethod == IoMethodMemoryMap
        || this->m_ioMethod == IoMethodUserPointer) {
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        this->xioctl(this->m_fd, VIDIOC_STREAMOFF, &type);
    }
}

bool CaptureV4L2::init()
{
    this->m_fd = x_open(this->m_device.toStdString().c_str(), O_RDWR | O_NONBLOCK, 0);

    if (this->m_fd < 0)
        return false;

    v4l2_capability capabilities;
    memset(&capabilities, 0, sizeof(v4l2_capability));

    if (this->xioctl(this->m_fd, VIDIOC_QUERYCAP, &capabilities) < 0) {
        qDebug() << "VideoCapture: Can't query capabilities.";
        x_close(this->m_fd);

        return false;
    }

    QList<int> streams = this->streams();

    if (streams.isEmpty()) {
        qDebug() << "VideoCapture: No streams available.";
        x_close(this->m_fd);

        return false;
    }

    QVariantList supportedCaps = this->caps(this->m_device);
    AkCaps caps = supportedCaps[streams[0]].value<AkCaps>();
    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (this->xioctl(this->m_fd, VIDIOC_G_FMT, &fmt) == 0) {
        fmt.fmt.pix.pixelformat = this->strToFourCC(caps.property("fourcc").toString());
        fmt.fmt.pix.width = caps.property("width").toUInt();
        fmt.fmt.pix.height = caps.property("height").toUInt();

        if (this->xioctl(this->m_fd, VIDIOC_S_FMT, &fmt) < 0) {
            qDebug() << "VideoCapture: Can't set format:"
                     << this->fourccToStr(fmt.fmt.pix.pixelformat);
            x_close(this->m_fd);

            return false;
        }
    }

    this->setFps(this->m_fd, caps.property("fps").toString());

    if (this->xioctl(this->m_fd, VIDIOC_S_FMT, &fmt) < 0) {
        qDebug() << "VideoCapture: Can't set format:"
                 << this->fourccToStr(fmt.fmt.pix.pixelformat);
        x_close(this->m_fd);

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

void CaptureV4L2::uninit()
{
    this->stopCapture();

    if (!this->m_buffers.isEmpty()) {
        if (this->m_ioMethod == IoMethodReadWrite)
            delete this->m_buffers[0].start;
        else if (this->m_ioMethod == IoMethodMemoryMap)
            for (qint32 i = 0; i < this->m_buffers.size(); i++)
                x_munmap(this->m_buffers[i].start, this->m_buffers[i].length);
        else if (this->m_ioMethod == IoMethodUserPointer)
            for (qint32 i = 0; i < this->m_buffers.size(); i++)
                delete this->m_buffers[i].start;
    }

    x_close(this->m_fd);
    this->m_caps.clear();
    this->m_fps = AkFrac();
    this->m_timeBase = AkFrac();
    this->m_buffers.clear();
}

void CaptureV4L2::setDevice(const QString &device)
{
    if (this->m_device == device)
        return;

    this->m_device = device;
    emit this->deviceChanged(device);
}

void CaptureV4L2::setStreams(const QList<int> &streams)
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

void CaptureV4L2::setIoMethod(const QString &ioMethod)
{
    if (this->m_fd >= 0)
        return;

    IoMethod ioMethodEnum = ioMethodToStr->key(ioMethod, IoMethodUnknown);

    if (this->m_ioMethod == ioMethodEnum)
        return;

    this->m_ioMethod = ioMethodEnum;
    emit this->ioMethodChanged(ioMethod);
}

void CaptureV4L2::setNBuffers(int nBuffers)
{
    if (this->m_nBuffers == nBuffers)
        return;

    this->m_nBuffers = nBuffers;
    emit this->nBuffersChanged(nBuffers);
}

void CaptureV4L2::resetDevice()
{
    this->setDevice("");
}

void CaptureV4L2::resetStreams()
{
    QVariantList supportedCaps = this->caps(this->m_device);
    QList<int> streams;

    if (!supportedCaps.isEmpty())
        streams << 0;

    this->setStreams(streams);
}

void CaptureV4L2::resetIoMethod()
{
    this->setIoMethod("any");
}

void CaptureV4L2::resetNBuffers()
{
    this->setNBuffers(32);
}

void CaptureV4L2::reset()
{
    this->resetStreams();
    this->resetImageControls();
    this->resetCameraControls();
}

void CaptureV4L2::updateDevices()
{
    decltype(this->m_devices) devices;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesCaps) devicesCaps;
    decltype(this->m_imageControls) imageControls;
    decltype(this->m_cameraControls) cameraControls;

    QList<v4l2_buf_type> bufType = {
        V4L2_BUF_TYPE_VIDEO_CAPTURE,
        V4L2_BUF_TYPE_VIDEO_OUTPUT,
        V4L2_BUF_TYPE_VIDEO_OVERLAY
    };

    QDir devicesDir("/dev");

    auto devicesFiles = devicesDir.entryList(QStringList() << "video*",
                                             QDir::System
                                             | QDir::Readable
                                             | QDir::Writable
                                             | QDir::NoSymLinks
                                             | QDir::NoDotAndDotDot
                                             | QDir::CaseSensitive,
                                             QDir::Name);

    v4l2_capability capability;
    memset(&capability, 0, sizeof(v4l2_capability));

    for (const QString &devicePath: devicesFiles) {
        auto fileName = devicesDir.absoluteFilePath(devicePath);
        int fd = x_open(fileName.toStdString().c_str(), O_RDWR | O_NONBLOCK, 0);

        if (fd >= 0) {
            // Check if this is a video capture device.
            if (this->xioctl(fd, VIDIOC_QUERYCAP, &capability) >= 0
                && capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
                v4l2_format fmt;
                memset(&fmt, 0, sizeof(v4l2_format));
                fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

                // Check if it has at least a default format.
                if (this->xioctl(fd, VIDIOC_G_FMT, &fmt) >= 0) {
                    devices << fileName;
                    descriptions[fileName] = reinterpret_cast<const char *>(capability.card);
                    QVariantList caps;

                    // Enumerate all supported formats.
                    for (const v4l2_buf_type &type: bufType) {
                        v4l2_fmtdesc fmt;
                        memset(&fmt, 0, sizeof(v4l2_fmtdesc));
                        fmt.type = type;

                        for (fmt.index = 0;
                             this->xioctl(fd, VIDIOC_ENUM_FMT, &fmt) >= 0;
                             fmt.index++) {
                            v4l2_frmsizeenum frmsize;
                            memset(&frmsize, 0, sizeof(v4l2_frmsizeenum));
                            frmsize.pixel_format = fmt.pixelformat;

                            // Eenumerate frame sizes.
                            for (frmsize.index = 0;
                                 this->xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0;
                                 frmsize.index++) {
                                if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                                    caps << this->capsFps(fd,
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

                    devicesCaps[fileName] = caps;
                    imageControls[fileName] = this->controls(fd, V4L2_CTRL_CLASS_USER);
                    cameraControls[fileName] = this->controls(fd, V4L2_CTRL_CLASS_CAMERA);
                }
            }

            x_close(fd);
        }
    }

    this->m_descriptions = descriptions;
    this->m_devicesCaps = devicesCaps;
    this->m_imageControls = imageControls;
    this->m_cameraControls = cameraControls;

    if (this->m_devices != devices) {
        if (!this->m_devices.isEmpty())
            this->m_fsWatcher->removePaths(this->m_devices);

        this->m_devices = devices;

        if (!this->m_devices.isEmpty())
            this->m_fsWatcher->addPaths(this->m_devices);

        emit this->webcamsChanged(this->m_devices);
    }
}

void CaptureV4L2::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path)

    this->updateDevices();
}

void CaptureV4L2::onFileChanged(const QString &fileName)
{
    Q_UNUSED(fileName)
}
