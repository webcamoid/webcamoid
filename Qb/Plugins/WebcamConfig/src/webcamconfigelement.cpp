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

#include "webcamconfigelement.h"

WebcamConfigElement::WebcamConfigElement(): QbElement()
{
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
}

QStringList WebcamConfigElement::webcams() const
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

    foreach (QString devicePath, devices)
    {
        device.setFileName(devicesDir.absoluteFilePath(devicePath));

        if (device.open(QIODevice::ReadWrite))
        {
            ioctl(device.handle(), VIDIOC_QUERYCAP, &capability);

            if (capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)
                webcams << device.fileName();

            device.close();
        }
    }

    return webcams;
}

QString WebcamConfigElement::description(const QString &webcam) const
{
    QFile device;
    v4l2_capability capability;
    memset(&capability, 0, sizeof(v4l2_capability));

    device.setFileName(webcam);

    if (device.open(QIODevice::ReadWrite))
    {
        ioctl(device.handle(), VIDIOC_QUERYCAP, &capability);

        if (capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)
            return QString((const char *) capability.card);

        device.close();
    }

    return "";
}

QVariantList WebcamConfigElement::availableSizes(const QString &webcam) const
{
    QFile device(webcam);
    QVariantList sizeList;

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return sizeList;

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

        while (ioctl(device.handle(), VIDIOC_ENUM_FMT, &fmt) >= 0)
        {
            v4l2_frmsizeenum frmsize;
            memset(&frmsize, 0, sizeof(v4l2_frmsizeenum));
            frmsize.pixel_format = fmt.pixelformat;
            frmsize.index = 0;

            while (ioctl(device.handle(),
                         VIDIOC_ENUM_FRAMESIZES,
                         &frmsize) >= 0)
            {
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

QSize WebcamConfigElement::size(const QString &webcam) const
{
    QFile deviceFile(webcam);
    QSize size;

    if (!deviceFile.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return size;

    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(deviceFile.handle(), VIDIOC_G_FMT, &fmt) >= 0)
        size = QSize(fmt.fmt.pix.width,
                     fmt.fmt.pix.height);

    deviceFile.close();

    return size;
}

bool WebcamConfigElement::setSize(const QString &webcam, const QSize &size) const
{
    QFile deviceFile(webcam);

    if (!deviceFile.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return false;

    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(deviceFile.handle(), VIDIOC_G_FMT, &fmt) == 0)
    {
        fmt.fmt.pix.width = size.width();
        fmt.fmt.pix.height = size.height();
        fmt.fmt.pix.pixelformat = this->format(webcam, size);

        ioctl(deviceFile.handle(), VIDIOC_S_FMT, &fmt);
    }

    deviceFile.close();

    emit this->sizeChanged(webcam, size);

    return true;
}

bool WebcamConfigElement::resetSize(const QString &webcam) const
{
    return this->setSize(webcam, this->availableSizes(webcam)[0].toSize());
}

QVariantList WebcamConfigElement::controls(const QString &webcam) const
{
    QVariantList controls;

    QFile device(webcam);

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return controls;

    v4l2_queryctrl queryctrl;
    memset(&queryctrl, 0, sizeof(v4l2_queryctrl));
    queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;

    while (ioctl(device.handle(), VIDIOC_QUERYCTRL, &queryctrl) == 0)
    {
        QVariantList control = this->queryControl(device.handle(), &queryctrl);

        if (!control.isEmpty())
            controls << QVariant(control);

        queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

    if (queryctrl.id != V4L2_CTRL_FLAG_NEXT_CTRL)
    {
        device.close();

        return controls;
    }

    for (int id = V4L2_CID_USER_BASE; id < V4L2_CID_LASTP1; id++)
    {
        queryctrl.id = id;

        if (ioctl(device.handle(), VIDIOC_QUERYCTRL, &queryctrl) == 0)
        {
            QVariantList control = this->queryControl(device.handle(), &queryctrl);

            if (!control.isEmpty())
                controls << QVariant(control);
        }
    }

    for (queryctrl.id = V4L2_CID_PRIVATE_BASE; ioctl(device.handle(), VIDIOC_QUERYCTRL, &queryctrl) == 0; queryctrl.id++)
    {
        QVariantList control = this->queryControl(device.handle(), &queryctrl);

        if (!control.isEmpty())
            controls << QVariant(control);
    }

    device.close();

    return controls;
}

bool WebcamConfigElement::setControls(const QString &webcam, const QVariantMap &controls) const
{
    QFile device(webcam);

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return false;

    QMap<QString, uint> ctrl2id = this->findControls(device.handle());
    QVector<v4l2_ext_control> mpegCtrls;
    QVector<v4l2_ext_control> userCtrls;

    foreach (QString control, controls.keys())
    {
        v4l2_ext_control ctrl;
        ctrl.id = ctrl2id[control];
        ctrl.value = controls[control].toInt();

        if (V4L2_CTRL_ID2CLASS(ctrl.id) == V4L2_CTRL_CLASS_MPEG)
            mpegCtrls << ctrl;
        else
            userCtrls << ctrl;
    }

    foreach (v4l2_ext_control user_ctrl, userCtrls)
    {
        v4l2_control ctrl;
        memset(&ctrl, 0, sizeof(v4l2_control));
        ctrl.id = user_ctrl.id;
        ctrl.value = user_ctrl.value;
        ioctl(device.handle(), VIDIOC_S_CTRL, &ctrl);
    }

    if (!mpegCtrls.isEmpty())
    {
        v4l2_ext_controls ctrls;
        memset(&ctrls, 0, sizeof(v4l2_ext_control));
        ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
        ctrls.count = mpegCtrls.size();
        ctrls.controls = &mpegCtrls[0];
        ioctl(device.handle(), VIDIOC_S_EXT_CTRLS, &ctrls);
    }

    device.close();

    emit this->controlsChanged(webcam, controls);

    return true;
}

bool WebcamConfigElement::resetControls(const QString &webcam) const
{
    QVariantMap controls;

    foreach (QVariant control, this->controls(webcam))
    {
        QVariantList params = control.toList();

        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setControls(webcam, controls);
}

bool WebcamConfigElement::event(QEvent *event)
{
    bool r;

    if (event->type() == QEvent::ThreadChange)
    {
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

__u32 WebcamConfigElement::format(const QString &webcam, const QSize &size) const
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

        while (ioctl(device.handle(), VIDIOC_ENUM_FMT, &fmt) >= 0)
        {
            v4l2_frmsizeenum frmsize;
            memset(&frmsize, 0, sizeof(v4l2_frmsizeenum));
            frmsize.pixel_format = fmt.pixelformat;
            frmsize.index = 0;

            while (ioctl(device.handle(),
                         VIDIOC_ENUM_FRAMESIZES,
                         &frmsize) >= 0)
            {
                if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                    if (QSize(frmsize.discrete.width,
                              frmsize.discrete.height) == size)
                    {
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

QVariantList WebcamConfigElement::queryControl(int handle, v4l2_queryctrl *queryctrl) const
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
        queryctrl->id < V4L2_CID_PRIVATE_BASE)
    {
        if (ioctl(handle, VIDIOC_G_EXT_CTRLS, &ctrls))
            return QVariantList();
    }
    else
    {
        v4l2_control ctrl;
        memset(&ctrl, 0, sizeof(v4l2_control));
        ctrl.id = queryctrl->id;

        if (ioctl(handle, VIDIOC_G_CTRL, &ctrl))
            return QVariantList();

        ext_ctrl.value = ctrl.value;
    }

    v4l2_querymenu qmenu;
    memset(&qmenu, 0, sizeof(v4l2_querymenu));
    qmenu.id = queryctrl->id;
    QStringList menu;

    if (queryctrl->type == V4L2_CTRL_TYPE_MENU)
        for (int i = 0; i < queryctrl->maximum + 1; i++)
        {
            qmenu.index = i;

            if (ioctl(handle, VIDIOC_QUERYMENU, &qmenu))
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

QMap<QString, uint> WebcamConfigElement::findControls(int handle) const
{
    v4l2_queryctrl qctrl;
    memset(&qctrl, 0, sizeof(v4l2_queryctrl));
    qctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    QMap<QString, uint> controls;

    while (ioctl(handle, VIDIOC_QUERYCTRL, &qctrl) == 0)
    {
        if (qctrl.type != V4L2_CTRL_TYPE_CTRL_CLASS &&
            !(qctrl.flags & V4L2_CTRL_FLAG_DISABLED))
            controls[QString((const char *) qctrl.name)] = qctrl.id;

        qctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

    if (qctrl.id != V4L2_CTRL_FLAG_NEXT_CTRL)
        return controls;

    for (int id = V4L2_CID_USER_BASE; id < V4L2_CID_LASTP1; id++)
    {
        qctrl.id = id;

        if (ioctl(handle, VIDIOC_QUERYCTRL, &qctrl) == 0 &&
           !(qctrl.flags & V4L2_CTRL_FLAG_DISABLED))
            controls[QString((const char *) qctrl.name)] = qctrl.id;
    }

    qctrl.id = V4L2_CID_PRIVATE_BASE;

    while (ioctl(handle, VIDIOC_QUERYCTRL, &qctrl) == 0)
    {
        if (!(qctrl.flags & V4L2_CTRL_FLAG_DISABLED))
            controls[QString((const char *) qctrl.name)] = qctrl.id;

        qctrl.id++;
    }

    return controls;
}

void WebcamConfigElement::reset(const QString &webcam) const
{
    this->resetSize(webcam);
    this->resetControls(webcam);
}

void WebcamConfigElement::reset() const
{
    foreach (QString webcam, this->webcams())
        this->reset(webcam);
}

void WebcamConfigElement::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path)

    QStringList webcams = this->webcams();

    if (webcams != this->m_webcams)
    {
        emit this->webcamsChanged(webcams);

        this->m_webcams = webcams;
    }
}
