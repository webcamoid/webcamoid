/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "videocaptureelement.h"

VideoCaptureElement::VideoCaptureElement(): QbElement()
{
    this->m_fd = -1;
    this->m_id = -1;

    this->resetIoMethod();
    this->resetNBuffers();

    // RGB formats
    //this->m_rawToFF[V4L2_PIX_FMT_RGB332] = "";
    this->m_rawToFF[V4L2_PIX_FMT_RGB444] = "rgb444le";
    this->m_rawToFF[V4L2_PIX_FMT_RGB555] = "rgb555le";
    this->m_rawToFF[V4L2_PIX_FMT_RGB565] = "rgb565le";
    this->m_rawToFF[V4L2_PIX_FMT_RGB555X] = "rgb555be";
    this->m_rawToFF[V4L2_PIX_FMT_RGB565X] = "rgb565be";
    //this->m_rawToFF[V4L2_PIX_FMT_BGR666] = "";
    this->m_rawToFF[V4L2_PIX_FMT_BGR24] = "bgr24";
    this->m_rawToFF[V4L2_PIX_FMT_RGB24] = "rgb24";
    this->m_rawToFF[V4L2_PIX_FMT_BGR32] = "bgr0";
    this->m_rawToFF[V4L2_PIX_FMT_RGB32] = "0rgb";

    // Grey formats
    this->m_rawToFF[V4L2_PIX_FMT_GREY] = "gray8a";
    //this->m_rawToFF[V4L2_PIX_FMT_Y4] = "";
    //this->m_rawToFF[V4L2_PIX_FMT_Y6] = "";
    //this->m_rawToFF[V4L2_PIX_FMT_Y10] = "";
    //this->m_rawToFF[V4L2_PIX_FMT_Y12] = "";
    this->m_rawToFF[V4L2_PIX_FMT_Y16] = "gray16le";

    // Grey bit-packed formats
    //this->m_rawToFF[V4L2_PIX_FMT_Y10BPACK] = "";

    // Palette formats
    //this->m_rawToFF[V4L2_PIX_FMT_PAL8] = "";

    // Chrominance formats
    //this->m_rawToFF[V4L2_PIX_FMT_UV8] = "";

    // Luminance+Chrominance formats
    this->m_rawToFF[V4L2_PIX_FMT_YVU410] = "yuv410p";
    this->m_rawToFF[V4L2_PIX_FMT_YVU420] = "yuv420p";
    this->m_rawToFF[V4L2_PIX_FMT_YUYV] = "yuyv422";
    this->m_rawToFF[V4L2_PIX_FMT_YYUV] = "yuv422p";
    //this->m_rawToFF[V4L2_PIX_FMT_YVYU] = "";
    this->m_rawToFF[V4L2_PIX_FMT_UYVY] = "uyvy422";
    this->m_rawToFF[V4L2_PIX_FMT_VYUY] = "yuv422p";
    this->m_rawToFF[V4L2_PIX_FMT_YUV422P] = "yuv422p";
    this->m_rawToFF[V4L2_PIX_FMT_YUV411P] = "yuv411p";
    this->m_rawToFF[V4L2_PIX_FMT_Y41P] = "yuv411p";
    //this->m_rawToFF[V4L2_PIX_FMT_YUV444] = "";
    //this->m_rawToFF[V4L2_PIX_FMT_YUV555] = "";
    //this->m_rawToFF[V4L2_PIX_FMT_YUV565] = "";
    //this->m_rawToFF[V4L2_PIX_FMT_YUV32] = "";
    this->m_rawToFF[V4L2_PIX_FMT_YUV410] = "yuv410p";
    this->m_rawToFF[V4L2_PIX_FMT_YUV420] = "yuv420p";
    //this->m_rawToFF[V4L2_PIX_FMT_HI240] = "";
    //this->m_rawToFF[V4L2_PIX_FMT_HM12] = "";
    //this->m_rawToFF[V4L2_PIX_FMT_M420] = "";

    // two planes -- one Y, one Cr + Cb interleaved
    this->m_rawToFF[V4L2_PIX_FMT_NV12] = "nv12";
    this->m_rawToFF[V4L2_PIX_FMT_NV21] = "nv21";
    this->m_rawToFF[V4L2_PIX_FMT_NV16] = "nv16";
    //this->m_rawToFF[V4L2_PIX_FMT_NV61] = "";
    //this->m_rawToFF[V4L2_PIX_FMT_NV24] = "";
    //this->m_rawToFF[V4L2_PIX_FMT_NV42] = "";

    // Bayer formats
    this->m_rawToFF[V4L2_PIX_FMT_SBGGR8] = "bayer_bggr8";
    this->m_rawToFF[V4L2_PIX_FMT_SGBRG8] = "bayer_gbrg8";
    this->m_rawToFF[V4L2_PIX_FMT_SGRBG8] = "bayer_grbg8";
    this->m_rawToFF[V4L2_PIX_FMT_SRGGB8] = "bayer_rggb8";

    // 10bit raw bayer, expanded to 16 bits
    this->m_rawToFF[V4L2_PIX_FMT_SBGGR16] = "bayer_bggr16le";

    // compressed formats
    this->m_compressedToFF[V4L2_PIX_FMT_MJPEG] = "mjpeg";
    this->m_compressedToFF[V4L2_PIX_FMT_JPEG] = "mjpeg";
    //this->m_compressedToFF[V4L2_PIX_FMT_DV] = "";
    //this->m_compressedToFF[V4L2_PIX_FMT_MPEG] = "";
    this->m_compressedToFF[V4L2_PIX_FMT_H264] = "h264";
    this->m_compressedToFF[V4L2_PIX_FMT_H264_NO_SC] = "h264";
    this->m_compressedToFF[V4L2_PIX_FMT_H264_MVC] = "h264";
    this->m_compressedToFF[V4L2_PIX_FMT_H263] = "h263";
    this->m_compressedToFF[V4L2_PIX_FMT_MPEG1] = "mpeg1video";
    this->m_compressedToFF[V4L2_PIX_FMT_MPEG2] = "mpeg2video";
    this->m_compressedToFF[V4L2_PIX_FMT_MPEG4] = "mpeg4";
    //this->m_compressedToFF[V4L2_PIX_FMT_XVID] = "";
    this->m_compressedToFF[V4L2_PIX_FMT_VC1_ANNEX_G] = "vc1";
    this->m_compressedToFF[V4L2_PIX_FMT_VC1_ANNEX_L] = "vc1";
    this->m_compressedToFF[V4L2_PIX_FMT_VP8] = "vp8";

    //  Vendor-specific formats
    this->m_compressedToFF[V4L2_PIX_FMT_CPIA1] = "cpia";

    // V4L2 controls
    this->m_ctrlTypeToString[V4L2_CTRL_TYPE_INTEGER] = "integer";
    this->m_ctrlTypeToString[V4L2_CTRL_TYPE_BOOLEAN] = "boolean";
    this->m_ctrlTypeToString[V4L2_CTRL_TYPE_MENU] = "menu";
    this->m_ctrlTypeToString[V4L2_CTRL_TYPE_BUTTON] = "button";
    this->m_ctrlTypeToString[V4L2_CTRL_TYPE_INTEGER64] = "integer64";
    this->m_ctrlTypeToString[V4L2_CTRL_TYPE_CTRL_CLASS] = "ctrlClass";
    this->m_ctrlTypeToString[V4L2_CTRL_TYPE_STRING] = "string";
    this->m_ctrlTypeToString[V4L2_CTRL_TYPE_BITMASK] = "bitmask";

#ifdef V4L2_CTRL_TYPE_INTEGER_MENU
    this->m_ctrlTypeToString[V4L2_CTRL_TYPE_INTEGER_MENU] = "integerMenu";
#endif

    this->m_webcams = this->webcams();
    this->m_fsWatcher = new QFileSystemWatcher(QStringList() << "/dev");
    this->m_fsWatcher->setParent(this);

    QObject::connect(this->m_fsWatcher,
                     SIGNAL(directoryChanged(const QString &)),
                     this,
                     SLOT(onDirectoryChanged(const QString &)));

    QObject::connect(&this->m_timer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(readFrame()));
}

QStringList VideoCaptureElement::webcams() const
{
    QDir devicesDir("/dev");

    QStringList devices = devicesDir.entryList(QStringList() << "video*",
                                               QDir::System |
                                               QDir::Readable |
                                               QDir::Writable |
                                               QDir::NoSymLinks |
                                               QDir::NoDotAndDotDot |
                                               QDir::CaseSensitive,
                                               QDir::Name);

    QStringList webcams;
    QFile device;
    v4l2_capability capability;
    memset(&capability, 0, sizeof(v4l2_capability));

    foreach (QString devicePath, devices) {
        device.setFileName(devicesDir.absoluteFilePath(devicePath));

        if (device.open(QIODevice::ReadWrite)) {
            this->xioctl(device.handle(), VIDIOC_QUERYCAP, &capability);

            if (capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)
                webcams << device.fileName();

            device.close();
        }
    }

    return webcams;
}

QString VideoCaptureElement::device() const
{
    return this->m_device;
}

QString VideoCaptureElement::ioMethod() const
{
    if (this->m_ioMethod == IoMethodReadWrite)
        return "readWrite";
    else if (this->m_ioMethod == IoMethodMemoryMap)
        return "memoryMap";
    else if (this->m_ioMethod == IoMethodUserPointer)
        return "userPointer";

    return "any";
}

int VideoCaptureElement::nBuffers() const
{
    return this->m_nBuffers;
}

bool VideoCaptureElement::isCompressed() const
{
    return false;
}

QString VideoCaptureElement::caps(v4l2_format *format, bool *changePxFmt) const
{
    if (this->m_caps)
        return this->m_caps.toString();

    bool closeFd = false;
    int fd = this->m_fd;

    if (fd < 0) {
        fd = open(this->m_device.toStdString().c_str(), O_RDWR);

        v4l2_capability capabilities;

        if (this->xioctl(fd, VIDIOC_QUERYCAP, &capabilities) < 0) {
            qDebug() << "VideoCapture: Can't query capabilities.";

            if (closeFd)
                close(fd);

            return "";
        }

        closeFd = true;
    }

    v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (this->xioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
        qDebug() << "VideoCapture: Can't get default input format.";

        if (closeFd)
            close(fd);

        return "";
    }

    if (!this->m_rawToFF.contains(fmt.fmt.pix.pixelformat)) {
        quint32 pixelFormat = this->defaultFormat(fd, false);

        if (pixelFormat) {
            fmt.fmt.pix.pixelformat = pixelFormat;

            if (changePxFmt)
                *changePxFmt = true;
        }
        else {
            qDebug() << "VideoCapture: Doesn't support format:" << this->fourccToStr(fmt.fmt.pix.pixelformat);

            if (closeFd)
                close(fd);

            return "";
        }
    }

    if (format)
        memcpy(format, &fmt, sizeof(v4l2_format));

    QbCaps caps;

    caps.setMimeType("video/x-raw");
    caps.setProperty("format", this->v4l2ToFF(fmt.fmt.pix.pixelformat));
    caps.setProperty("width", fmt.fmt.pix.width);
    caps.setProperty("height", fmt.fmt.pix.height);
    caps.setProperty("fps", this->fps(fd).toString());

    if (closeFd)
        close(fd);

    return caps.toString();
}

QString VideoCaptureElement::description(const QString &webcam) const
{
    QFile device;
    v4l2_capability capability;
    memset(&capability, 0, sizeof(v4l2_capability));

    device.setFileName(webcam);

    if (device.open(QIODevice::ReadWrite)) {
        this->xioctl(device.handle(), VIDIOC_QUERYCAP, &capability);

        if (capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)
            return QString((const char *) capability.card);

        device.close();
    }

    return "";
}

QVariantList VideoCaptureElement::availableSizes(const QString &webcam) const
{
    QFile device(webcam);
    QVariantList sizeList;

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return sizeList;

    QList<v4l2_buf_type> bufType;

    bufType << V4L2_BUF_TYPE_VIDEO_CAPTURE
            << V4L2_BUF_TYPE_VIDEO_OUTPUT
            << V4L2_BUF_TYPE_VIDEO_OVERLAY;

    foreach (v4l2_buf_type type, bufType) {
        v4l2_fmtdesc fmt;
        memset(&fmt, 0, sizeof(v4l2_fmtdesc));
        fmt.index = 0;
        fmt.type = type;

        while (this->xioctl(device.handle(), VIDIOC_ENUM_FMT, &fmt) >= 0) {
            v4l2_frmsizeenum frmsize;
            memset(&frmsize, 0, sizeof(v4l2_frmsizeenum));
            frmsize.pixel_format = fmt.pixelformat;
            frmsize.index = 0;

            while (this->xioctl(device.handle(),
                         VIDIOC_ENUM_FRAMESIZES,
                         &frmsize) >= 0) {
                if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                    sizeList << QSize(frmsize.discrete.width,
                                      frmsize.discrete.height);

                frmsize.index++;
            }

            fmt.index++;
        }
    }

    device.close();

    return sizeList;
}

QSize VideoCaptureElement::size(const QString &webcam) const
{
    QFile deviceFile(webcam);
    QSize size;

    if (!deviceFile.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return size;

    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (this->xioctl(deviceFile.handle(), VIDIOC_G_FMT, &fmt) >= 0)
        size = QSize(fmt.fmt.pix.width,
                     fmt.fmt.pix.height);

    deviceFile.close();

    return size;
}

bool VideoCaptureElement::setSize(const QString &webcam, const QSize &size) const
{
    QFile deviceFile(webcam);

    if (!deviceFile.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return false;

    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (this->xioctl(deviceFile.handle(), VIDIOC_G_FMT, &fmt) == 0) {
        fmt.fmt.pix.width = size.width();
        fmt.fmt.pix.height = size.height();
        fmt.fmt.pix.pixelformat = this->format(webcam, size);

        this->xioctl(deviceFile.handle(), VIDIOC_S_FMT, &fmt);
    }

    deviceFile.close();

    emit this->sizeChanged(webcam, size);

    return true;
}

bool VideoCaptureElement::resetSize(const QString &webcam) const
{
    return this->setSize(webcam, this->availableSizes(webcam)[0].toSize());
}

QVariantList VideoCaptureElement::controls(const QString &webcam) const
{
    QVariantList controls;

    QFile device(webcam);

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return controls;

    v4l2_queryctrl queryctrl;
    memset(&queryctrl, 0, sizeof(v4l2_queryctrl));
    queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;

    while (this->xioctl(device.handle(), VIDIOC_QUERYCTRL, &queryctrl) == 0) {
        QVariantList control = this->queryControl(device.handle(), &queryctrl);

        if (!control.isEmpty())
            controls << QVariant(control);

        queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

    if (queryctrl.id != V4L2_CTRL_FLAG_NEXT_CTRL) {
        device.close();

        return controls;
    }

    for (int id = V4L2_CID_USER_BASE; id < V4L2_CID_LASTP1; id++) {
        queryctrl.id = id;

        if (this->xioctl(device.handle(), VIDIOC_QUERYCTRL, &queryctrl) == 0) {
            QVariantList control = this->queryControl(device.handle(), &queryctrl);

            if (!control.isEmpty())
                controls << QVariant(control);
        }
    }

    for (queryctrl.id = V4L2_CID_PRIVATE_BASE;
         this->xioctl(device.handle(), VIDIOC_QUERYCTRL, &queryctrl) == 0;
         queryctrl.id++) {
        QVariantList control = this->queryControl(device.handle(), &queryctrl);

        if (!control.isEmpty())
            controls << QVariant(control);
    }

    device.close();

    return controls;
}

bool VideoCaptureElement::setControls(const QString &webcam, const QVariantMap &controls) const
{
    QFile device(webcam);

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return false;

    QMap<QString, uint> ctrl2id = this->findControls(device.handle());
    QVector<v4l2_ext_control> mpegCtrls;
    QVector<v4l2_ext_control> userCtrls;

    foreach (QString control, controls.keys()) {
        v4l2_ext_control ctrl;
        ctrl.id = ctrl2id[control];
        ctrl.value = controls[control].toInt();

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
        ctrls.count = mpegCtrls.size();
        ctrls.controls = &mpegCtrls[0];
        this->xioctl(device.handle(), VIDIOC_S_EXT_CTRLS, &ctrls);
    }

    device.close();

    emit this->controlsChanged(webcam, controls);

    return true;
}

bool VideoCaptureElement::resetControls(const QString &webcam) const
{
    QVariantMap controls;

    foreach (QVariant control, this->controls(webcam)) {
        QVariantList params = control.toList();

        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setControls(webcam, controls);
}

bool VideoCaptureElement::event(QEvent *event)
{
    bool r;

    if (event->type() == QEvent::ThreadChange) {
        QObject::disconnect(this->m_fsWatcher,
                            SIGNAL(directoryChanged(const QString &)),
                            this,
                            SLOT(onDirectoryChanged(const QString &)));

        r = QObject::event(event);
        this->m_fsWatcher->moveToThread(this->thread());

        QObject::connect(this->m_fsWatcher,
                         SIGNAL(directoryChanged(const QString &)),
                         this,
                         SLOT(onDirectoryChanged(const QString &)));
    }
    else
        r = QObject::event(event);

    return r;
}

__u32 VideoCaptureElement::format(const QString &webcam, const QSize &size) const
{
    QFile device(webcam);

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return 0;

    QList<v4l2_buf_type> bufType;

    bufType << V4L2_BUF_TYPE_VIDEO_CAPTURE
            << V4L2_BUF_TYPE_VIDEO_OUTPUT
            << V4L2_BUF_TYPE_VIDEO_OVERLAY;

    foreach (v4l2_buf_type type, bufType)
    {
        v4l2_fmtdesc fmt;
        memset(&fmt, 0, sizeof(v4l2_fmtdesc));
        fmt.index = 0;
        fmt.type = type;

        while (this->xioctl(device.handle(), VIDIOC_ENUM_FMT, &fmt) >= 0) {
            v4l2_frmsizeenum frmsize;
            memset(&frmsize, 0, sizeof(v4l2_frmsizeenum));
            frmsize.pixel_format = fmt.pixelformat;
            frmsize.index = 0;

            while (this->xioctl(device.handle(),
                         VIDIOC_ENUM_FRAMESIZES,
                         &frmsize) >= 0) {
                if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                    if (QSize(frmsize.discrete.width,
                              frmsize.discrete.height) == size) {
                        device.close();

                        return fmt.pixelformat;
                    }

                frmsize.index++;
            }

            fmt.index++;
        }
    }

    device.close();

    return 0;
}

QVariantList VideoCaptureElement::queryControl(int handle, v4l2_queryctrl *queryctrl) const
{
    if (queryctrl->flags & V4L2_CTRL_FLAG_DISABLED)
        return QVariantList();

    if (queryctrl->type == V4L2_CTRL_TYPE_CTRL_CLASS)
        return QVariantList();

    v4l2_ext_control ext_ctrl;
    ext_ctrl.id = queryctrl->id;

    v4l2_ext_controls ctrls;
    ctrls.ctrl_class = V4L2_CTRL_ID2CLASS(queryctrl->id);
    ctrls.count = 1;
    ctrls.controls = &ext_ctrl;

    if (V4L2_CTRL_ID2CLASS(queryctrl->id) != V4L2_CTRL_CLASS_USER &&
        queryctrl->id < V4L2_CID_PRIVATE_BASE) {
        if (this->xioctl(handle, VIDIOC_G_EXT_CTRLS, &ctrls))
            return QVariantList();
    }
    else {
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
            qmenu.index = i;

            if (this->xioctl(handle, VIDIOC_QUERYMENU, &qmenu))
                continue;

            menu << QString((const char *) qmenu.name);
        }

    v4l2_ctrl_type type = static_cast<v4l2_ctrl_type>(queryctrl->type);

    return QVariantList() << QString((const char *) queryctrl->name)
                          << this->m_ctrlTypeToString[type]
                          << queryctrl->minimum
                          << queryctrl->maximum
                          << queryctrl->step
                          << queryctrl->default_value
                          << ext_ctrl.value
                          << menu;
}

QMap<QString, uint> VideoCaptureElement::findControls(int handle) const
{
    v4l2_queryctrl qctrl;
    memset(&qctrl, 0, sizeof(v4l2_queryctrl));
    qctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    QMap<QString, uint> controls;

    while (this->xioctl(handle, VIDIOC_QUERYCTRL, &qctrl) == 0) {
        if (qctrl.type != V4L2_CTRL_TYPE_CTRL_CLASS &&
            !(qctrl.flags & V4L2_CTRL_FLAG_DISABLED))
            controls[QString((const char *) qctrl.name)] = qctrl.id;

        qctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

    if (qctrl.id != V4L2_CTRL_FLAG_NEXT_CTRL)
        return controls;

    for (int id = V4L2_CID_USER_BASE; id < V4L2_CID_LASTP1; id++) {
        qctrl.id = id;

        if (this->xioctl(handle, VIDIOC_QUERYCTRL, &qctrl) == 0 &&
           !(qctrl.flags & V4L2_CTRL_FLAG_DISABLED))
            controls[QString((const char *) qctrl.name)] = qctrl.id;
    }

    qctrl.id = V4L2_CID_PRIVATE_BASE;

    while (this->xioctl(handle, VIDIOC_QUERYCTRL, &qctrl) == 0) {
        if (!(qctrl.flags & V4L2_CTRL_FLAG_DISABLED))
            controls[QString((const char *) qctrl.name)] = qctrl.id;

        qctrl.id++;
    }

    return controls;
}

int VideoCaptureElement::xioctl(int fd, int request, void *arg) const
{
    int r = -1;

    while (true) {
        r = ioctl(fd, request, arg);

        if (r != -1 || errno != EINTR)
            break;
    }

    return r;
}

QString VideoCaptureElement::v4l2ToFF(quint32 fmt) const
{
    if (this->m_rawToFF.contains(fmt))
        return this->m_rawToFF[fmt];

    if (this->m_compressedToFF.contains(fmt))
        return this->m_compressedToFF[fmt];

    return "";
}

quint32 VideoCaptureElement::defaultFormat(int fd, bool compressed) const
{
    v4l2_fmtdesc fmtdesc;

    for (int i = 0; ; i++) {
        memset(&fmtdesc, 0, sizeof(fmtdesc));
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmtdesc.index = i;

        if (this->xioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) < 0)
            break;

        bool isCompressed = (fmtdesc.flags & V4L2_FMT_FLAG_COMPRESSED)? true: false;

        if ((isCompressed && compressed)
            || (!isCompressed && !compressed))
            if ((!compressed && this->m_rawToFF.contains(fmtdesc.pixelformat))
                || (compressed && this->m_compressedToFF.contains(fmtdesc.pixelformat)))
                return fmtdesc.pixelformat;
    }

    return 0;
}

bool VideoCaptureElement::isCompressedFormat(quint32 format)
{
    v4l2_fmtdesc fmtdesc;

    for (int i = 0; ; i++) {
        memset(&fmtdesc, 0, sizeof(fmtdesc));
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmtdesc.index = i;

        if (this->xioctl(this->m_fd, VIDIOC_ENUM_FMT, &fmtdesc) < 0)
            break;

        if (fmtdesc.pixelformat == format)
            return (fmtdesc.flags & V4L2_FMT_FLAG_COMPRESSED)? true: false;
    }

    return false;
}

bool VideoCaptureElement::initReadWrite(quint32 bufferSize)
{
    this->m_buffers.resize(1);

    this->m_buffers[0].length = bufferSize;
    this->m_buffers[0].start = new char[bufferSize];

    if (!this->m_buffers[0].start)
        return false;

    return true;
}

bool VideoCaptureElement::initMemoryMap()
{
    v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(requestBuffers));

    requestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requestBuffers.memory = V4L2_MEMORY_MMAP;
    requestBuffers.count = this->m_nBuffers;

    if (this->xioctl(this->m_fd, VIDIOC_REQBUFS, &requestBuffers) < 0)
        return false;

    if (requestBuffers.count < 1)
        return false;

    this->m_buffers.resize(requestBuffers.count);
    bool error = false;

    for (quint32 i = 0; i < requestBuffers.count; i++) {
        v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(buffer));

        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = i;

        if (this->xioctl(this->m_fd, VIDIOC_QUERYBUF, &buffer) < 0) {
            error = true;

            break;
        }

        this->m_buffers[i].length = buffer.length;

        this->m_buffers[i].start = (char *) mmap(NULL,
                                                 buffer.length,
                                                 PROT_READ | PROT_WRITE,
                                                 MAP_SHARED,
                                                 this->m_fd,
                                                 buffer.m.offset);

        if (this->m_buffers[i].start == MAP_FAILED) {
            error = true;

            break;
        }
    }

    if (error) {
        for (qint32 i = 0; i < this->m_buffers.size(); i++)
            munmap(this->m_buffers[i].start, this->m_buffers[i].length);

        return false;
    }

    return true;
}

bool VideoCaptureElement::initUserPointer(quint32 bufferSize)
{
    v4l2_requestbuffers requestBuffers;
    memset(&requestBuffers, 0, sizeof(requestBuffers));

    requestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requestBuffers.memory = V4L2_MEMORY_USERPTR;
    requestBuffers.count = this->m_nBuffers;

    if (this->xioctl(this->m_fd, VIDIOC_REQBUFS, &requestBuffers) < 0)
        return false;

    this->m_buffers.resize(requestBuffers.count);
    bool error = false;

    for (quint32 i = 0; i < requestBuffers.count; i++) {
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

        return false;
    }

    return true;
}

void VideoCaptureElement::stopCapture()
{
    if (this->m_ioMethod == IoMethodMemoryMap
        || this->m_ioMethod == IoMethodUserPointer) {
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        this->xioctl(this->m_fd, VIDIOC_STREAMOFF, &type);
    }
}

void VideoCaptureElement::uninit()
{
    this->m_timer.stop();
    this->stopCapture();

    if (this->m_ioMethod == IoMethodReadWrite)
        delete this->m_buffers[0].start;
    else if (this->m_ioMethod == IoMethodMemoryMap)
        for (qint32 i = 0; i < this->m_buffers.size(); i++)
            munmap(this->m_buffers[i].start, this->m_buffers[i].length);
    else if (this->m_ioMethod == IoMethodUserPointer)
        for (qint32 i = 0; i < this->m_buffers.size(); i++)
            delete this->m_buffers[i].start;

    close(this->m_fd);
    this->m_caps.clear();
    this->m_fd = -1;
}

bool VideoCaptureElement::startCapture()
{
    bool error = false;

    if (this->m_ioMethod == IoMethodMemoryMap) {
        for (qint32 i = 0; i < this->m_buffers.size(); i++) {
            v4l2_buffer buffer;
            memset(&buffer, 0, sizeof(buffer));

            buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buffer.memory = V4L2_MEMORY_MMAP;
            buffer.index = i;

            if (this->xioctl(this->m_fd, VIDIOC_QBUF, &buffer) < 0)
                error = true;
        }

        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (this->xioctl(this->m_fd, VIDIOC_STREAMON, &type) < 0)
            error = true;
    }
    else if (this->m_ioMethod == IoMethodUserPointer) {
        for (qint32 i = 0; i < this->m_buffers.size(); i++) {
            v4l2_buffer buffer;
            memset(&buffer, 0, sizeof(buffer));

            buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buffer.memory = V4L2_MEMORY_USERPTR;
            buffer.index = i;
            buffer.m.userptr = (unsigned long) this->m_buffers[i].start;
            buffer.length = this->m_buffers[i].length;

            if (this->xioctl(this->m_fd, VIDIOC_QBUF, &buffer) < 0)
                error = true;
        }

        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (this->xioctl(this->m_fd, VIDIOC_STREAMON, &type) < 0)
            error = true;
    }

    if (error)
        this->uninit();

    if (!error)
        this->m_timer.start();

    this->m_id = Qb::id();

    return !error;
}

bool VideoCaptureElement::init()
{
    this->m_fd = open(this->m_device.toStdString().c_str(), O_RDWR);

    v4l2_capability capabilities;

    if (this->xioctl(this->m_fd, VIDIOC_QUERYCAP, &capabilities) < 0) {
        qDebug() << "VideoCapture: Can't query capabilities.";
        close(this->m_fd);

        return false;
    }

    v4l2_format fmt;
    bool changePxFmt = false;

    QbCaps caps = this->caps(&fmt, &changePxFmt);

    if (changePxFmt && this->xioctl(this->m_fd, VIDIOC_S_FMT, &fmt) < 0) {
        qDebug() << "VideoCapture: Can't set format:" << this->fourccToStr(fmt.fmt.pix.pixelformat);
        close(this->m_fd);

        return false;
    }

    this->m_caps = caps;
    this->m_fps = caps.property("fps").toString();
    this->m_timeBase = this->m_fps.invert();

    if (this->m_ioMethod == IoMethodReadWrite
        && capabilities.capabilities & V4L2_CAP_READWRITE
        && this->initReadWrite(fmt.fmt.pix.sizeimage)) {
    }
    else if (this->m_ioMethod == IoMethodMemoryMap
             && capabilities.capabilities & V4L2_CAP_STREAMING
             && this->initMemoryMap()) {
    }
    else if (this->m_ioMethod == IoMethodUserPointer
             && capabilities.capabilities & V4L2_CAP_STREAMING
             && this->initUserPointer(fmt.fmt.pix.sizeimage)) {
    }
    else
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
    }
    else
        return false;

    return this->startCapture();
}

void VideoCaptureElement::processFrame(char *buffer, quint32 bufferSize, qint64 pts)
{
    QbBufferPtr oBuffer(new char[bufferSize]);

    if (!oBuffer)
        return;

    memcpy(oBuffer.data(), buffer, bufferSize);

    QbPacket oPacket(this->m_caps,
                     oBuffer,
                     bufferSize);

    oPacket.setPts(pts);
    oPacket.setTimeBase(this->m_timeBase);
    oPacket.setIndex(0);
    oPacket.setId(this->m_id);

    emit this->oStream(oPacket);
}

QString VideoCaptureElement::fourccToStr(quint32 format) const
{
    QString fourcc;

    for (int i = 0; i < 4; i++) {
        fourcc.append(QChar(format & 0xff));
        format >>= 8;
    }

    return fourcc;
}

QbFrac VideoCaptureElement::fps(int fd) const
{
    QbFrac fps;
    v4l2_std_id stdId;

    if (this->xioctl(fd, VIDIOC_G_STD, &stdId) >= 0) {
        v4l2_standard standard;
        memset(&standard, 0, sizeof(standard));

        standard.index = 0;

        while (this->xioctl(fd, VIDIOC_ENUMSTD, &standard) == 0) {
            if (standard.id & stdId) {
                fps = QbFrac(standard.frameperiod.denominator,
                             standard.frameperiod.numerator);

                break;
            }

            standard.index++;
        }
    }

    v4l2_streamparm streamparm;
    memset(&streamparm, 0, sizeof(streamparm));

    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (this->xioctl(fd, VIDIOC_G_PARM, &streamparm) >= 0) {
        if (streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)
            fps = QbFrac(streamparm.parm.capture.timeperframe.denominator,
                         streamparm.parm.capture.timeperframe.numerator);
    }

    return fps;
}

void VideoCaptureElement::stateChange(QbElement::ElementState from, QbElement::ElementState to)
{
    if (from == QbElement::ElementStateNull
        && to == QbElement::ElementStatePaused)
        this->init();
    else if (from == QbElement::ElementStatePaused
             && to == QbElement::ElementStateNull)
        this->uninit();
}

void VideoCaptureElement::setDevice(const QString &device)
{
    this->m_device = device;
}

void VideoCaptureElement::setIoMethod(const QString &ioMethod)
{
    if (this->m_fd >= 0)
        return;

    if (ioMethod == "readWrite")
        this->m_ioMethod = IoMethodReadWrite;
    else if (ioMethod == "memoryMap")
        this->m_ioMethod = IoMethodMemoryMap;
    else if (ioMethod == "userPointer")
        this->m_ioMethod = IoMethodUserPointer;

    this->m_ioMethod = IoMethodUnknown;
}

void VideoCaptureElement::resetIoMethod()
{
    this->setIoMethod("any");
}

void VideoCaptureElement::setNBuffers(int nBuffers)
{
    this->m_nBuffers = nBuffers;
}

void VideoCaptureElement::resetDevice()
{
    this->setDevice("");
}

void VideoCaptureElement::resetNBuffers()
{
    this->setNBuffers(32);
}

void VideoCaptureElement::reset(const QString &webcam) const
{
    this->resetSize(webcam);
    this->resetControls(webcam);
}

void VideoCaptureElement::reset() const
{
    foreach (QString webcam, this->webcams())
        this->reset(webcam);
}

void VideoCaptureElement::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path)

    QStringList webcams = this->webcams();

    if (webcams != this->m_webcams) {
        emit this->webcamsChanged(webcams);

        this->m_webcams = webcams;
    }
}

bool VideoCaptureElement::readFrame()
{
    if (this->m_fd < 0)
        return false;

    if (this->m_ioMethod == IoMethodReadWrite) {
        if (read(this->m_fd, this->m_buffers[0].start, this->m_buffers[0].length) < 0)
            return false;

        timeval timestamp;
        gettimeofday(&timestamp, NULL);

        qint64 pts = (timestamp.tv_sec
                      + 1e-6 * timestamp.tv_usec)
                       * this->m_fps.value();

        this->processFrame(this->m_buffers[0].start, this->m_buffers[0].length, pts);
    }
    else if (this->m_ioMethod == IoMethodMemoryMap
             || this->m_ioMethod == IoMethodUserPointer) {
        v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(buffer));

        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        buffer.memory = (this->m_ioMethod == IoMethodMemoryMap)?
                            V4L2_MEMORY_MMAP:
                            V4L2_MEMORY_USERPTR;

        if (this->xioctl(this->m_fd, VIDIOC_DQBUF, &buffer) < 0)
            return false;

        if (buffer.index >= (quint32) this->m_buffers.size())
            return false;

        qint64 pts = (buffer.timestamp.tv_sec
                       + 1e-6 * buffer.timestamp.tv_usec)
                        * this->m_fps.value();

        this->processFrame(this->m_buffers[buffer.index].start, buffer.bytesused, pts);

        if (this->xioctl(this->m_fd, VIDIOC_QBUF, &buffer) < 0)
            return false;
    }

    return true;
}
