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
#include <QVariant>
#include <QMap>
#include <QVector>
#include <QDir>
#include <QFileSystemWatcher>
#include <ak.h>
#include <akfrac.h>
#include <akcaps.h>
#include <akpacket.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/videodev2.h>

#ifdef HAVE_V4LUTILS
#include <libv4l2.h>

#define x_ioctl v4l2_ioctl
#define x_open v4l2_open
#define x_close v4l2_close
#define x_read v4l2_read
#define x_mmap v4l2_mmap
#define x_munmap v4l2_munmap
#else
#include <unistd.h>
#include <sys/ioctl.h>

#define x_ioctl ioctl
#define x_open open
#define x_close close
#define x_read read
#define x_mmap mmap
#define x_munmap munmap
#endif

#include "capturev4l2.h"
#include "capturebuffer.h"

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
#ifdef HAVE_EXTENDEDCONTROLS
        {V4L2_CTRL_TYPE_STRING      , "string"     },
        {V4L2_CTRL_TYPE_BITMASK     , "bitmask"    },
        {V4L2_CTRL_TYPE_INTEGER_MENU, "integerMenu"}
#endif
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

class CaptureV4L2Private
{
    public:
        CaptureV4L2 *self;
        QString m_device;
        QList<int> m_streams;
        QStringList m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, QVariantList> m_devicesCaps;
        QMap<QString, QVariantList> m_imageControls;
        QMap<QString, QVariantList> m_cameraControls;
        QFileSystemWatcher *m_fsWatcher;
        AkFrac m_fps;
        AkFrac m_timeBase;
        AkCaps m_caps;
        qint64 m_id;
        QVector<CaptureBuffer> m_buffers;
        CaptureV4L2::IoMethod m_ioMethod;
        int m_nBuffers;
        int m_fd;

        CaptureV4L2Private(CaptureV4L2 *self):
            self(self),
            m_fsWatcher(nullptr),
            m_id(-1),
            m_ioMethod(CaptureV4L2::IoMethodUnknown),
            m_nBuffers(32),
            m_fd(-1)
        {
        }

        QVariantList capsFps(int fd,
                             const v4l2_fmtdesc &format,
                             __u32 width,
                             __u32 height) const;
        QVariantList caps(int fd) const;
        AkFrac fps(int fd) const;
        void setFps(int fd, const AkFrac &fps);
        QVariantList controls(int fd, quint32 controlClass) const;
        bool setControls(int fd,
                         quint32 controlClass,
                         const QVariantMap &controls) const;
        QVariantList queryControl(int handle,
                                  quint32 controlClass,
                                  v4l2_queryctrl *queryctrl) const;
        QMap<QString, quint32> findControls(int handle,
                                            quint32 controlClass) const;
        bool initReadWrite(quint32 bufferSize);
        bool initMemoryMap();
        bool initUserPointer(quint32 bufferSize);
        bool startCapture();
        void stopCapture();
        QString fourccToStr(quint32 format) const;
        quint32 strToFourCC(const QString &format) const;
        int xioctl(int fd, ulong request, void *arg) const;
        AkPacket processFrame(const char *buffer,
                              size_t bufferSize,
                              qint64 pts) const;
};

CaptureV4L2::CaptureV4L2(QObject *parent):
    Capture(parent)
{
    this->d = new CaptureV4L2Private(this);
    this->d->m_fsWatcher = new QFileSystemWatcher({"/dev"}, this);

    QObject::connect(this->d->m_fsWatcher,
                     &QFileSystemWatcher::directoryChanged,
                     this,
                     &CaptureV4L2::onDirectoryChanged);
    QObject::connect(this->d->m_fsWatcher,
                     &QFileSystemWatcher::fileChanged,
                     this,
                     &CaptureV4L2::onFileChanged);

    this->updateDevices();
}

CaptureV4L2::~CaptureV4L2()
{
    delete this->d->m_fsWatcher;
    delete this->d;
}

QStringList CaptureV4L2::webcams() const
{
    return this->d->m_devices;
}

QString CaptureV4L2::device() const
{
    return this->d->m_device;
}

QList<int> CaptureV4L2::streams()
{
    if (!this->d->m_streams.isEmpty())
        return this->d->m_streams;

    QVariantList caps = this->caps(this->d->m_device);

    if (caps.isEmpty())
        return QList<int>();

    return QList<int> {0};
}

QList<int> CaptureV4L2::listTracks(const QString &mimeType)
{
    if (mimeType != "video/x-raw"
        && !mimeType.isEmpty())
        return QList<int>();

    QVariantList caps = this->caps(this->d->m_device);
    QList<int> streams;

    for (int i = 0; i < caps.count(); i++)
        streams << i;

    return streams;
}

QString CaptureV4L2::ioMethod() const
{
    return ioMethodToStr->value(this->d->m_ioMethod, "any");
}

int CaptureV4L2::nBuffers() const
{
    return this->d->m_nBuffers;
}

QString CaptureV4L2::description(const QString &webcam) const
{
    return this->d->m_descriptions.value(webcam);
}

QVariantList CaptureV4L2::caps(const QString &webcam) const
{
    return this->d->m_devicesCaps.value(webcam);
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
    return this->d->m_imageControls.value(this->d->m_device);
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

    if (this->d->m_fd >= 0)
        fd = this->d->m_fd;
    else
        fd = x_open(this->d->m_device.toStdString().c_str(),
                    O_RDWR | O_NONBLOCK, 0);

    if (!this->d->setControls(fd, V4L2_CTRL_CLASS_USER, imageControlsDiff))
        return false;

    if (this->d->m_fd < 0)
        x_close(fd);

    QVariantList controls;

    for (const auto &control: this->d->m_imageControls.value(this->d->m_device)) {
        auto controlParams = control.toList();
        auto controlName = controlParams[0].toString();

        if (imageControlsDiff.contains(controlName))
            controlParams[6] = imageControlsDiff[controlName];

        controls << QVariant(controlParams);
    }

    this->d->m_imageControls[this->d->m_device] = controls;
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
    return this->d->m_cameraControls.value(this->d->m_device);
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

    if (this->d->m_fd >= 0)
        fd = this->d->m_fd;
    else
        fd = x_open(this->d->m_device.toStdString().c_str(),
                    O_RDWR | O_NONBLOCK, 0);

#ifdef V4L2_CTRL_CLASS_CAMERA
    if (!this->d->setControls(fd, V4L2_CTRL_CLASS_CAMERA, cameraControlsDiff))
        return false;
#endif

    if (this->d->m_fd < 0)
        x_close(fd);

    QVariantList controls;

    for (const auto &control: this->d->m_cameraControls.value(this->d->m_device)) {
        auto controlParams = control.toList();
        auto controlName = controlParams[0].toString();

        if (cameraControlsDiff.contains(controlName))
            controlParams[6] = cameraControlsDiff[controlName];

        controls << QVariant(controlParams);
    }

    this->d->m_cameraControls[this->d->m_device] = controls;
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
    if (this->d->m_buffers.isEmpty())
        return AkPacket();

    if (this->d->m_fd < 0)
        return AkPacket();

    if (this->d->m_ioMethod == IoMethodReadWrite) {
        if (x_read(this->d->m_fd,
                   this->d->m_buffers[0].start,
                   this->d->m_buffers[0].length) < 0)
            return AkPacket();

        timeval timestamp;
        gettimeofday(&timestamp, nullptr);

        qint64 pts = qint64((timestamp.tv_sec
                             + 1e-6 * timestamp.tv_usec)
                            * this->d->m_fps.value());

        return this->d->processFrame(this->d->m_buffers[0].start,
                                     this->d->m_buffers[0].length,
                                     pts);
    } else if (this->d->m_ioMethod == IoMethodMemoryMap
               || this->d->m_ioMethod == IoMethodUserPointer) {
        v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = (this->d->m_ioMethod == IoMethodMemoryMap)?
                            V4L2_MEMORY_MMAP:
                            V4L2_MEMORY_USERPTR;

        if (this->d->xioctl(this->d->m_fd, VIDIOC_DQBUF, &buffer) < 0)
            return AkPacket();

        if (buffer.index >= quint32(this->d->m_buffers.size()))
            return AkPacket();

        qint64 pts = qint64((buffer.timestamp.tv_sec
                             + 1e-6 * buffer.timestamp.tv_usec)
                            * this->d->m_fps.value());

        AkPacket packet =
                this->d->processFrame(this->d->m_buffers[int(buffer.index)].start,
                                      buffer.bytesused,
                                      pts);

        if (this->d->xioctl(this->d->m_fd, VIDIOC_QBUF, &buffer) < 0)
            return AkPacket();

        return packet;
    }

    return AkPacket();
}

QVariantList CaptureV4L2Private::capsFps(int fd,
                                         const struct v4l2_fmtdesc &format,
                                         __u32 width,
                                         __u32 height) const
{
    QVariantList caps;

#ifdef VIDIOC_ENUM_FRAMEINTERVALS
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
            fps = AkFrac(frmival.discrete.denominator,
                         frmival.discrete.numerator);
        else
            fps = AkFrac(frmival.stepwise.min.denominator,
                         frmival.stepwise.max.numerator);

        videoCaps.setProperty("fps", fps.toString());
        caps << QVariant::fromValue(videoCaps);
    }
#else
    struct v4l2_streamparm params;
    memset(&params, 0, sizeof(v4l2_streamparm));
    params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (this->xioctl(fd, VIDIOC_G_PARM, &params) >= 0) {
        AkCaps videoCaps;
        auto timeperframe = &params.parm.capture.timeperframe;
        videoCaps.setMimeType("video/unknown");
        videoCaps.setProperty("fourcc", this->fourccToStr(format.pixelformat));
        videoCaps.setProperty("width", width);
        videoCaps.setProperty("height", height);
        videoCaps.setProperty("fps", AkFrac(timeperframe->denominator,
                                            timeperframe->numerator).toString());
        caps << QVariant::fromValue(videoCaps);
    }
#endif

    return caps;
}

QVariantList CaptureV4L2Private::caps(int fd) const
{
    QVariantList caps;

#ifndef VIDIOC_ENUM_FRAMESIZES
    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    uint width = 0;
    uint height = 0;

    // Check if it has at least a default format.
    if (this->xioctl(fd, VIDIOC_G_FMT, &fmt) >= 0) {
        width = fmt.fmt.pix.width;
        height = fmt.fmt.pix.height;
    }

    if (width <= 0 || height <= 0)
        return {};
#endif

    // Enumerate all supported formats.
    v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc, 0, sizeof(v4l2_fmtdesc));
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    for (fmtdesc.index = 0;
         this->xioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) >= 0;
         fmtdesc.index++) {
#ifdef VIDIOC_ENUM_FRAMESIZES
        v4l2_frmsizeenum frmsize;
        memset(&frmsize, 0, sizeof(v4l2_frmsizeenum));
        frmsize.pixel_format = fmtdesc.pixelformat;

        // Enumerate frame sizes.
        for (frmsize.index = 0;
             this->xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0;
             frmsize.index++) {
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                caps << this->capsFps(fd,
                                      fmtdesc,
                                      frmsize.discrete.width,
                                      frmsize.discrete.height);
            } else {
#if 0
                for (uint height = frmsize.stepwise.min_height;
                     height < frmsize.stepwise.max_height;
                     height += frmsize.stepwise.step_height)
                    for (uint width = frmsize.stepwise.min_width;
                         width < frmsize.stepwise.max_width;
                         width += frmsize.stepwise.step_width) {
                        caps << this->capsFps(fd,
                                              fmtdesc,
                                              width,
                                              height);
                    }
#endif
            }
        }
#else
        caps << this->capsFps(fd,
                              fmtdesc,
                              width,
                              height);
#endif
    }

    return caps;
}

AkFrac CaptureV4L2Private::fps(int fd) const
{
    AkFrac fps(30, 1);
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

void CaptureV4L2Private::setFps(int fd, const AkFrac &fps)
{
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

QVariantList CaptureV4L2Private::controls(int fd, quint32 controlClass) const
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

bool CaptureV4L2Private::setControls(int fd,
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

QVariantList CaptureV4L2Private::queryControl(int handle,
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

QMap<QString, quint32> CaptureV4L2Private::findControls(int handle,
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

bool CaptureV4L2Private::initReadWrite(quint32 bufferSize)
{
    this->m_buffers.resize(1);

    this->m_buffers[0].length = bufferSize;
    this->m_buffers[0].start = new char[bufferSize];

    if (!this->m_buffers[0].start) {
        this->m_buffers.clear();

        return false;
    }

    memset(this->m_buffers[0].start, 0, bufferSize);

    return true;
}

bool CaptureV4L2Private::initMemoryMap()
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
        this->m_buffers[i].start =
                reinterpret_cast<char *>(x_mmap(nullptr,
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

bool CaptureV4L2Private::initUserPointer(quint32 bufferSize)
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

        memset(this->m_buffers[i].start, 0, bufferSize);
    }

    if (error) {
        for (qint32 i = 0; i < this->m_buffers.size(); i++)
            delete [] this->m_buffers[i].start;

        this->m_buffers.clear();

        return false;
    }

    return true;
}

bool CaptureV4L2Private::startCapture()
{
    bool error = false;

    if (this->m_ioMethod == CaptureV4L2::IoMethodMemoryMap) {
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
    } else if (this->m_ioMethod == CaptureV4L2::IoMethodUserPointer) {
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
        self->uninit();

    this->m_id = Ak::id();

    return !error;
}

void CaptureV4L2Private::stopCapture()
{
    if (this->m_ioMethod == CaptureV4L2::IoMethodMemoryMap
        || this->m_ioMethod == CaptureV4L2::IoMethodUserPointer) {
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        this->xioctl(this->m_fd, VIDIOC_STREAMOFF, &type);
    }
}

QString CaptureV4L2Private::fourccToStr(quint32 format) const
{
    char fourcc[5];
    memcpy(fourcc, &format, sizeof(quint32));
    fourcc[4] = 0;

    return QString(fourcc);
}

quint32 CaptureV4L2Private::strToFourCC(const QString &format) const
{
    quint32 fourcc;
    memcpy(&fourcc, format.toStdString().c_str(), sizeof(quint32));

    return fourcc;
}

int CaptureV4L2Private::xioctl(int fd, ulong request, void *arg) const
{
    int r = -1;

    forever {
        r = x_ioctl(fd, request, arg);

        if (r != -1 || errno != EINTR)
            break;
    }

    return r;
}

AkPacket CaptureV4L2Private::processFrame(const char *buffer,
                                          size_t bufferSize,
                                          qint64 pts) const
{
    QByteArray oBuffer(buffer, int(bufferSize));
    AkPacket oPacket(this->m_caps, oBuffer);

    oPacket.setPts(pts);
    oPacket.setTimeBase(this->m_timeBase);
    oPacket.setIndex(0);
    oPacket.setId(this->m_id);

    return oPacket;
}

bool CaptureV4L2::init()
{
    // Frames read must be blocking so we does not waste CPU time.
    this->d->m_fd =
            x_open(this->d->m_device.toStdString().c_str(),
                   O_RDWR, // | O_NONBLOCK,
                   0);

    if (this->d->m_fd < 0)
        return false;

    v4l2_capability capabilities;
    memset(&capabilities, 0, sizeof(v4l2_capability));

    if (this->d->xioctl(this->d->m_fd, VIDIOC_QUERYCAP, &capabilities) < 0) {
        qDebug() << "VideoCapture: Can't query capabilities.";
        x_close(this->d->m_fd);

        return false;
    }

    QList<int> streams = this->streams();

    if (streams.isEmpty()) {
        qDebug() << "VideoCapture: No streams available.";
        x_close(this->d->m_fd);

        return false;
    }

    QVariantList supportedCaps = this->caps(this->d->m_device);
    AkCaps caps = supportedCaps[streams[0]].value<AkCaps>();
    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    this->d->xioctl(this->d->m_fd, VIDIOC_G_FMT, &fmt);
    fmt.fmt.pix.pixelformat =
            this->d->strToFourCC(caps.property("fourcc").toString());
    fmt.fmt.pix.width = caps.property("width").toUInt();
    fmt.fmt.pix.height = caps.property("height").toUInt();

    if (this->d->xioctl(this->d->m_fd, VIDIOC_S_FMT, &fmt) < 0) {
        qDebug() << "VideoCapture: Can't set format:"
                 << this->d->fourccToStr(fmt.fmt.pix.pixelformat);
        x_close(this->d->m_fd);

        return false;
    }

    this->d->setFps(this->d->m_fd, caps.property("fps").toString());
    this->d->m_caps = caps;
    this->d->m_fps = caps.property("fps").toString();
    this->d->m_timeBase = this->d->m_fps.invert();

    if (this->d->m_ioMethod == IoMethodReadWrite
        && capabilities.capabilities & V4L2_CAP_READWRITE
        && this->d->initReadWrite(fmt.fmt.pix.sizeimage)) {
    } else if (this->d->m_ioMethod == IoMethodMemoryMap
             && capabilities.capabilities & V4L2_CAP_STREAMING
             && this->d->initMemoryMap()) {
    } else if (this->d->m_ioMethod == IoMethodUserPointer
             && capabilities.capabilities & V4L2_CAP_STREAMING
             && this->d->initUserPointer(fmt.fmt.pix.sizeimage)) {
    } else
        this->d->m_ioMethod = IoMethodUnknown;

    if (this->d->m_ioMethod != IoMethodUnknown)
        return this->d->startCapture();

    if (capabilities.capabilities & V4L2_CAP_STREAMING) {
        if (this->d->initMemoryMap())
            this->d->m_ioMethod = IoMethodMemoryMap;
        else if (this->d->initUserPointer(fmt.fmt.pix.sizeimage))
            this->d->m_ioMethod = IoMethodUserPointer;
    }

    if (this->d->m_ioMethod == IoMethodUnknown) {
        if (capabilities.capabilities & V4L2_CAP_READWRITE
            && this->d->initReadWrite(fmt.fmt.pix.sizeimage))
            this->d->m_ioMethod = IoMethodReadWrite;
        else
            return false;
    }

    return this->d->startCapture();
}

void CaptureV4L2::uninit()
{
    this->d->stopCapture();

    if (!this->d->m_buffers.isEmpty()) {
        if (this->d->m_ioMethod == IoMethodReadWrite)
            delete [] this->d->m_buffers[0].start;
        else if (this->d->m_ioMethod == IoMethodMemoryMap)
            for (qint32 i = 0; i < this->d->m_buffers.size(); i++)
                x_munmap(this->d->m_buffers[i].start,
                         this->d->m_buffers[i].length);
        else if (this->d->m_ioMethod == IoMethodUserPointer)
            for (qint32 i = 0; i < this->d->m_buffers.size(); i++)
                delete [] this->d->m_buffers[i].start;
    }

    x_close(this->d->m_fd);
    this->d->m_caps.clear();
    this->d->m_fps = AkFrac();
    this->d->m_timeBase = AkFrac();
    this->d->m_buffers.clear();
}

void CaptureV4L2::setDevice(const QString &device)
{
    if (this->d->m_device == device)
        return;

    this->d->m_device = device;
    emit this->deviceChanged(device);
}

void CaptureV4L2::setStreams(const QList<int> &streams)
{
    if (streams.isEmpty())
        return;

    int stream = streams[0];

    if (stream < 0)
        return;

    auto supportedCaps = this->caps(this->d->m_device);

    if (stream >= supportedCaps.length())
        return;

    QList<int> inputStreams;
    inputStreams << stream;

    if (this->streams() == inputStreams)
        return;

    this->d->m_streams = inputStreams;
    emit this->streamsChanged(inputStreams);
}

void CaptureV4L2::setIoMethod(const QString &ioMethod)
{
    if (this->d->m_fd >= 0)
        return;

    IoMethod ioMethodEnum = ioMethodToStr->key(ioMethod, IoMethodUnknown);

    if (this->d->m_ioMethod == ioMethodEnum)
        return;

    this->d->m_ioMethod = ioMethodEnum;
    emit this->ioMethodChanged(ioMethod);
}

void CaptureV4L2::setNBuffers(int nBuffers)
{
    if (this->d->m_nBuffers == nBuffers)
        return;

    this->d->m_nBuffers = nBuffers;
    emit this->nBuffersChanged(nBuffers);
}

void CaptureV4L2::resetDevice()
{
    this->setDevice("");
}

void CaptureV4L2::resetStreams()
{
    QVariantList supportedCaps = this->caps(this->d->m_device);
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
    decltype(this->d->m_devices) devices;
    decltype(this->d->m_descriptions) descriptions;
    decltype(this->d->m_devicesCaps) devicesCaps;
    decltype(this->d->m_imageControls) imageControls;
    decltype(this->d->m_cameraControls) cameraControls;

    QDir devicesDir("/dev");

    auto devicesFiles = devicesDir.entryList(QStringList() << "video*",
                                             QDir::System
                                             | QDir::Readable
                                             | QDir::Writable
                                             | QDir::NoSymLinks
                                             | QDir::NoDotAndDotDot
                                             | QDir::CaseSensitive,
                                             QDir::Name);

    for (const QString &devicePath: devicesFiles) {
        auto fileName = devicesDir.absoluteFilePath(devicePath);
        int fd = x_open(fileName.toStdString().c_str(), O_RDWR | O_NONBLOCK, 0);

        if (fd < 0)
            continue;

        auto caps = this->d->caps(fd);

        if (!caps.empty()) {
            v4l2_capability capability;
            memset(&capability, 0, sizeof(v4l2_capability));
            QString description;

            if (this->d->xioctl(fd, VIDIOC_QUERYCAP, &capability) >= 0)
                description = reinterpret_cast<const char *>(capability.card);

            devices << fileName;
            descriptions[fileName] = description;
            devicesCaps[fileName] = caps;

            imageControls[fileName] =
                    this->d->controls(fd, V4L2_CTRL_CLASS_USER);
#ifdef V4L2_CTRL_CLASS_CAMERA
            cameraControls[fileName] =
                    this->d->controls(fd, V4L2_CTRL_CLASS_CAMERA);
#endif
        }

        x_close(fd);
    }

    this->d->m_descriptions = descriptions;
    this->d->m_devicesCaps = devicesCaps;
    this->d->m_imageControls = imageControls;
    this->d->m_cameraControls = cameraControls;

    if (this->d->m_devices != devices) {
        if (!this->d->m_devices.isEmpty())
            this->d->m_fsWatcher->removePaths(this->d->m_devices);

        this->d->m_devices = devices;

        if (!this->d->m_devices.isEmpty())
            this->d->m_fsWatcher->addPaths(this->d->m_devices);

        emit this->webcamsChanged(this->d->m_devices);
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

#include "moc_capturev4l2.cpp"
