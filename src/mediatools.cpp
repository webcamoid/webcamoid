/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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

#include <sys/ioctl.h>
#include <QtXml>
#include <KSharedConfig>
#include <KConfigGroup>
#include <linux/videodev2.h>

#include "mediatools.h"

MediaTools::MediaTools(bool watchDevices, QObject *parent): QObject(parent)
{
    this->m_appEnvironment = new AppEnvironment(this);

    QObject::connect(QCoreApplication::instance(),
                     SIGNAL(aboutToQuit()),
                     this,
                     SLOT(aboutToQuit()));

    this->resetDevice();
    this->resetVideoFormat();
    this->resetEffectsPreview();
    this->resetPlayAudioFromSource();
    this->m_recordAudioFrom = RecordFromMic;
    this->resetRecording();
    this->resetVideoRecordFormats();
    this->resetStreams();
    this->resetWindowSize();

    Qb::init();

#ifdef QT_DEBUG
    QDir pluginsDir("Qb/Plugins");
    QStringList pluginsPaths = Qb::pluginsPaths();

    foreach (QString pluginPath, pluginsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name))
        pluginsPaths << pluginsDir.absoluteFilePath(pluginPath);

    Qb::setPluginsPaths(pluginsPaths);
#endif

    this->m_pipeline = Qb::create("Bin", "pipeline");

    if (this->m_pipeline)
    {
        QString description("MultiSrc objectName='source' loop=true "
                            "stateChanged>videoMux.setState "
                            "stateChanged>effects.setState "
                            "stateChanged>muxAudioInput.setState !"
                            "Multiplex objectName='videoMux' "
                            "caps='video/x-raw' outputIndex=0 !"
                            "Bin objectName='effects' blocking=false !"
                            "VCapsConvert caps='video/x-raw,format=bgra' source.stateChanged>setState !"
                            "Sync source.stateChanged>setState !"
                            "OUT. ,"
                            "source. !"
                            "Multiplex objectName='muxAudioInput' "
                            "caps='audio/x-raw' outputIndex=0 !"
                            "Multiplex objectName='audioSwitch' "
                            "outputIndex=1 ,"
                            "muxAudioInput. !"
                            "MultiSink objectName='audioOutput' ,"
                            "MultiSrc objectName='mic' !"
                            "Multiplex outputIndex=1 "
                            "mic.stateChanged>setState ! audioSwitch. ,"
                            "effects. ! MultiSink objectName='record' ,"
                            "audioSwitch. ! record.");

        this->m_pipeline->setProperty("description", description);

        this->m_effectsPreview = Qb::create("Bin");

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_source),
                                  Q_ARG(QString, "source"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_effects),
                                  Q_ARG(QString, "effects"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_audioSwitch),
                                  Q_ARG(QString, "audioSwitch"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_audioOutput),
                                  Q_ARG(QString, "audioOutput"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_mic),
                                  Q_ARG(QString, "mic"));

        QMetaObject::invokeMethod(this->m_pipeline.data(),
                                  "element", Qt::DirectConnection,
                                  Q_RETURN_ARG(QbElementPtr, this->m_record),
                                  Q_ARG(QString, "record"));

        this->m_pipeline->link(this);

        if (this->m_source)
        {
            this->m_source->link(this->m_effectsPreview);

            QObject::connect(this->m_source.data(),
                             SIGNAL(error(QString)),
                             this,
                             SIGNAL(error(QString)));
        }

        this->audioSetup();
    }

    if (watchDevices)
    {
        this->m_fsWatcher = new QFileSystemWatcher(QStringList() << "/dev", this);

        QObject::connect(this->m_fsWatcher,
                         SIGNAL(directoryChanged(const QString &)),
                         this,
                         SLOT(onDirectoryChanged(const QString &)));
    }

    this->loadConfigs();
}

MediaTools::~MediaTools()
{
    this->resetDevice();
    this->saveConfigs();
}

void MediaTools::iStream(const QbPacket &packet)
{
    QString sender = this->sender()->objectName();

    QImage frame(packet.buffer().data(),
                 packet.caps().property("width").toInt(),
                 packet.caps().property("height").toInt(),
                 QImage::Format_ARGB32);

    if (sender == "pipeline")
    {
        emit this->frameReady(frame);

        if (frame.size() != this->m_curFrameSize)
        {
            emit this->frameSizeChanged(frame.size());
            this->m_curFrameSize = frame.size();
        }
    }
    else
    {
        QString name = this->nameFromHash(sender);

        emit this->previewFrameReady(frame, name);
    }
}

QString MediaTools::device()
{
    return this->m_device;
}

QVariantList MediaTools::videoFormat(QString device)
{
    QFile deviceFile(device.isEmpty()? this->device(): device);
    QVariantList format;

    if (!deviceFile.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return format;

    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(deviceFile.handle(), VIDIOC_G_FMT, &fmt) >= 0)
        format << fmt.fmt.pix.width
               << fmt.fmt.pix.height
               << fmt.fmt.pix.pixelformat;

    deviceFile.close();

    return format;
}

bool MediaTools::effectsPreview()
{
    return this->m_showEffectsPreview;
}

bool MediaTools::playAudioFromSource()
{
    return this->m_playAudioFromSource;
}

MediaTools::RecordFrom MediaTools::recordAudioFrom()
{
    return this->m_recordAudioFrom;
}

bool MediaTools::recording()
{
    return this->m_recording;
}

QList<QStringList> MediaTools::videoRecordFormats()
{
    return this->m_videoRecordFormats;
}

QList<QStringList> MediaTools::streams()
{
    return this->m_streams;
}

QSize MediaTools::windowSize()
{
    return this->m_windowSize;
}

QString MediaTools::fcc2s(uint val)
{
    QString s = "";

    s += QChar(val & 0xff);
    s += QChar((val >> 8) & 0xff);
    s += QChar((val >> 16) & 0xff);
    s += QChar((val >> 24) & 0xff);

    return s;
}

QVariantList MediaTools::videoFormats(QString device)
{
    QFile deviceFile(device);
    QVariantList formats;

    if (!deviceFile.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return formats;

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

        while (ioctl(deviceFile.handle(), VIDIOC_ENUM_FMT, &fmt) >= 0)
        {
            v4l2_frmsizeenum frmsize;
            memset(&frmsize, 0, sizeof(v4l2_frmsizeenum));
            frmsize.pixel_format = fmt.pixelformat;
            frmsize.index = 0;

            while (ioctl(deviceFile.handle(),
                         VIDIOC_ENUM_FRAMESIZES,
                         &frmsize) >= 0)
            {
                if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                {
                    QVariantList format;

                    format << frmsize.discrete.width
                           << frmsize.discrete.height
                           << fmt.pixelformat;

                    formats << QVariant(format);
                }

                frmsize.index++;
            }

            fmt.index++;
        }
    }

    deviceFile.close();

    return formats;
}

QList<QStringList> MediaTools::captureDevices()
{
    QList<QStringList> webcamsDevices;
    QDir devicesDir("/dev");

    QStringList devices = devicesDir.entryList(QStringList() << "video*",
                                               QDir::System |
                                               QDir::Readable |
                                               QDir::Writable |
                                               QDir::NoSymLinks |
                                               QDir::NoDotAndDotDot |
                                               QDir::CaseSensitive,
                                               QDir::Name);

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
            {
                QStringList cap;

                cap << device.fileName()
                    << QString((const char *) capability.card);

                webcamsDevices << cap;
            }

            device.close();
        }
    }

    QStringList desktopDevice;

    desktopDevice << ":0.0" << this->tr("Desktop");

    QList<QStringList> allDevices = webcamsDevices +
                                    this->m_streams +
                                    QList<QStringList>() << desktopDevice;

    return allDevices;
}

QVariantList MediaTools::listControls(QString dev_name)
{
    QVariantList controls;

    QFile device(dev_name);

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

bool MediaTools::setControls(QString dev_name, QMap<QString, uint> controls)
{
    QFile device(dev_name);

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return false;

    QMap<QString, uint> ctrl2id = this->findControls(device.handle());
    std::vector<v4l2_ext_control> mpeg_ctrls;
    std::vector<v4l2_ext_control> user_ctrls;

    foreach (QString control, controls.keys())
    {
        v4l2_ext_control ctrl;
        ctrl.id = ctrl2id[control];
        ctrl.value = controls[control];

        if (V4L2_CTRL_ID2CLASS(ctrl.id) == V4L2_CTRL_CLASS_MPEG)
            mpeg_ctrls.push_back(ctrl);
        else
            user_ctrls.push_back(ctrl);
    }

    foreach (v4l2_ext_control user_ctrl, user_ctrls)
    {
        v4l2_control ctrl;
        memset(&ctrl, 0, sizeof(v4l2_control));
        ctrl.id = user_ctrl.id;
        ctrl.value = user_ctrl.value;
        ioctl(device.handle(), VIDIOC_S_CTRL, &ctrl);
    }

    if (!mpeg_ctrls.empty())
    {
        v4l2_ext_controls ctrls;
        memset(&ctrls, 0, sizeof(v4l2_ext_control));
        ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
        ctrls.count = mpeg_ctrls.size();
        ctrls.controls = &mpeg_ctrls[0];
        ioctl(device.handle(), VIDIOC_S_EXT_CTRLS, &ctrls);
    }

    device.close();

    return true;
}

QMap<QString, QString> MediaTools::availableEffects()
{
    QMap<QString, QString> effects;

    QDomDocument effectsXml("effects");
    QFile xmlFile(":/webcamoid/share/effects.xml");
    xmlFile.open(QIODevice::ReadOnly);
    effectsXml.setContent(&xmlFile);
    xmlFile.close();

    QDomNodeList effectNodes = effectsXml.documentElement().childNodes();

    for (int effect = 0; effect < effectNodes.count(); effect++)
    {
        QDomNode effectNode = effectNodes.item(effect);

        if (!effectNode.isElement())
            continue;

        QDomNamedNodeMap attributtes = effectNode.attributes();
        QString effectName = attributtes.namedItem("name").nodeValue();
        QString effectDescription = effectNode.firstChild().toText().data();

        effects[effectDescription] = effectName;
    }

    return effects;
}

QStringList MediaTools::currentEffects()
{
    return this->m_effectsList;
}

QString MediaTools::bestRecordFormatOptions(QString fileName)
{
    QString ext = QFileInfo(fileName).completeSuffix();

    if (ext.isEmpty())
        return "";

    foreach (QStringList format, this->m_videoRecordFormats)
        foreach (QString s, format[0].split(",", QString::SkipEmptyParts))
            if (s.toLower().trimmed() == ext)
                return format[1];

    return "";
}

QVariantList MediaTools::queryControl(int dev_fd, v4l2_queryctrl *queryctrl)
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
        if (ioctl(dev_fd, VIDIOC_G_EXT_CTRLS, &ctrls))
            return QVariantList();
    }
    else
    {
        v4l2_control ctrl;
        memset(&ctrl, 0, sizeof(v4l2_control));
        ctrl.id = queryctrl->id;

        if (ioctl(dev_fd, VIDIOC_G_CTRL, &ctrl))
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

            if (ioctl(dev_fd, VIDIOC_QUERYMENU, &qmenu))
                continue;

            menu << QString((const char *) qmenu.name);
        }

    return QVariantList() << QString((const char *) queryctrl->name)
                          << queryctrl->type
                          << queryctrl->minimum
                          << queryctrl->maximum
                          << queryctrl->step
                          << queryctrl->default_value
                          << ext_ctrl.value
                          << menu;
}

QMap<QString, uint> MediaTools::findControls(int devFd)
{
    v4l2_queryctrl qctrl;
    memset(&qctrl, 0, sizeof(v4l2_queryctrl));
    qctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    QMap<QString, uint> controls;

    while (ioctl(devFd, VIDIOC_QUERYCTRL, &qctrl) == 0)
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

        if (ioctl(devFd, VIDIOC_QUERYCTRL, &qctrl) == 0 &&
           !(qctrl.flags & V4L2_CTRL_FLAG_DISABLED))
            controls[QString((const char *) qctrl.name)] = qctrl.id;
    }

    qctrl.id = V4L2_CID_PRIVATE_BASE;

    while (ioctl(devFd, VIDIOC_QUERYCTRL, &qctrl) == 0)
    {
        if (!(qctrl.flags & V4L2_CTRL_FLAG_DISABLED))
            controls[QString((const char *) qctrl.name)] = qctrl.id;

        qctrl.id++;
    }

    return controls;
}

QString MediaTools::hashFromName(QString name)
{
    return QString("x") + name.toUtf8().toHex();
}

QString MediaTools::nameFromHash(QString hash)
{
    return QByteArray::fromHex(hash.mid(1).toUtf8());
}

void MediaTools::setRecordAudioFrom(RecordFrom recordAudio)
{
    if (this->m_recordAudioFrom == recordAudio)
        return;

    if (!this->m_mic ||
        !this->m_audioSwitch ||
        !this->m_record)
    {
        this->m_recordAudioFrom = recordAudio;

        return;
    }

    if (recordAudio == RecordFromNone)
    {
        if (this->m_recordAudioFrom == RecordFromMic)
            this->m_mic->setState(QbElement::ElementStateNull);

        this->m_audioSwitch->setState(QbElement::ElementStateNull);

        QObject::disconnect(this->m_record.data(),
                            SIGNAL(stateChanged(ElementState)),
                            this->m_audioSwitch.data(),
                            SLOT(setState(ElementState)));

        if (this->m_recordAudioFrom == RecordFromMic)
            QObject::disconnect(this->m_record.data(),
                                SIGNAL(stateChanged(ElementState)),
                                this->m_mic.data(),
                                SLOT(setState(ElementState)));
    }
    else
    {
        if (recordAudio == RecordFromSource)
        {
            if (this->m_recordAudioFrom == RecordFromMic)
            {
                this->m_mic->setState(QbElement::ElementStateNull);

                QObject::disconnect(this->m_record.data(),
                                    SIGNAL(stateChanged(ElementState)),
                                    this->m_mic.data(),
                                    SLOT(setState(ElementState)));
            }

            this->m_audioSwitch->setProperty("inputIndex", 0);
        }
        else if (recordAudio == RecordFromMic)
        {
            if (this->m_record->state() == QbElement::ElementStatePlaying ||
                this->m_record->state() == QbElement::ElementStatePaused)
                this->m_mic->setState(this->m_record->state());

            QObject::connect(this->m_record.data(),
                             SIGNAL(stateChanged(ElementState)),
                             this->m_mic.data(),
                             SLOT(setState(ElementState)));

            this->m_audioSwitch->setProperty("inputIndex", 1);
        }

        if (this->m_recordAudioFrom == RecordFromNone)
        {
            if (this->m_record->state() == QbElement::ElementStatePlaying ||
                this->m_record->state() == QbElement::ElementStatePaused)
                this->m_audioSwitch->setState(this->m_record->state());

            QObject::connect(this->m_record.data(),
                             SIGNAL(stateChanged(ElementState)),
                             this->m_audioSwitch.data(),
                             SLOT(setState(ElementState)));
        }
    }

    this->m_recordAudioFrom = recordAudio;
}

void MediaTools::setRecording(bool recording, QString fileName)
{
    if (!this->m_pipeline || !this->m_record)
    {
        this->m_recording = false;
        emit this->recordingChanged(this->m_recording);

        return;
    }

    if (this->m_record->state() != QbElement::ElementStateNull)
    {
        this->m_record->setState(QbElement::ElementStateNull);

        this->m_recording = false;
        emit this->recordingChanged(this->m_recording);
    }

    if (recording)
    {
        QString options = this->bestRecordFormatOptions(fileName);

        if (options == "")
        {
            this->m_recording = false;
            emit this->recordingChanged(this->m_recording);

            return;
        }

        this->m_record->setProperty("location", fileName);
        this->m_record->setProperty("options", options);
        this->m_record->setState(QbElement::ElementStatePlaying);

        if (this->m_record->state() == QbElement::ElementStatePlaying)
            this->m_recording = true;
        else
            this->m_recording = false;

        emit this->recordingChanged(this->m_recording);
    }
}

void MediaTools::mutexLock()
{
    this->m_mutex.lock();
}

void MediaTools::mutexUnlock()
{
    this->m_mutex.unlock();
}

void MediaTools::setDevice(QString device)
{
    if (device.isEmpty())
    {
        this->resetRecording();
        this->resetEffectsPreview();

        if (this->m_source)
            this->m_source->setState(QbElement::ElementStateNull);

        this->m_device = "";
        emit this->deviceChanged(this->m_device);
    }
    else
    {
        if (!this->m_source)
            return;

        this->m_source->setProperty("location", device);

        this->m_source->setState(QbElement::ElementStatePlaying);

        if (this->m_source->state() == QbElement::ElementStatePlaying)
            this->m_device = device;
        else
            this->m_device = "";

        emit this->deviceChanged(this->m_device);
    }
}

void MediaTools::setVideoFormat(QVariantList videoFormat, QString device)
{
    QString curDevice = this->device();
    QbElement::ElementState state = this->m_source->state();

    if (state == QbElement::ElementStatePlaying && (device.isEmpty() || device == curDevice))
        this->resetDevice();

    QFile deviceFile(device.isEmpty()? curDevice: device);

    if (!deviceFile.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return;

    v4l2_format fmt;
    memset(&fmt, 0, sizeof(v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(deviceFile.handle(), VIDIOC_G_FMT, &fmt) == 0)
    {
        fmt.fmt.pix.width = videoFormat.at(0).toUInt();
        fmt.fmt.pix.height = videoFormat.at(1).toUInt();
        fmt.fmt.pix.pixelformat = videoFormat.at(2).toUInt();

        ioctl(deviceFile.handle(), VIDIOC_S_FMT, &fmt);
    }

    deviceFile.close();

    if (state == QbElement::ElementStatePlaying && (device.isEmpty() || device == curDevice))
        this->setDevice(device.isEmpty()? curDevice: device);
}

void MediaTools::setEffectsPreview(bool effectsPreview)
{
    this->m_showEffectsPreview = effectsPreview;

    if (!this->m_effectsPreview || !this->m_source)
        return;

    this->m_effectsPreview->setState(QbElement::ElementStateNull);

    if (effectsPreview && this->m_source->state() == QbElement::ElementStatePlaying)
    {
        QString description = this->m_effectsPreview->property("description").toString();

        if (description.isEmpty())
        {
            description = QString("IN. ! VCapsConvert objectName='preview' "
                                  "caps='video/x-raw,width=%1,height=%2'").arg(128)
                                                                          .arg(96);

            QStringList effects = this->availableEffects().keys();

            foreach (QString effect, effects)
            {
                QString previewHash = this->hashFromName(effect);

                description += QString(", preview. !"
                                       "%1 !"
                                       "VCapsConvert objectName='%2' "
                                       "caps='video/x-raw,format=bgra'").arg(effect)
                                                                        .arg(previewHash);
            }

            this->m_effectsPreview->setProperty("description", description);

            foreach (QString effect, effects)
            {
                QString previewHash = this->hashFromName(effect);
                QbElementPtr preview;

                QMetaObject::invokeMethod(this->m_effectsPreview.data(),
                                          "element", Qt::DirectConnection,
                                          Q_RETURN_ARG(QbElementPtr, preview),
                                          Q_ARG(QString, previewHash));

                preview->link(this);
            }
        }

        this->m_effectsPreview->setState(QbElement::ElementStatePlaying);
    }
}

void MediaTools::setPlayAudioFromSource(bool playAudio)
{
    this->m_playAudioFromSource = playAudio;

    if (!this->m_source || !this->m_audioOutput)
        return;

    QbElement::ElementState sourceState = this->m_source->state();

    if (playAudio)
    {
        if (sourceState == QbElement::ElementStatePlaying ||
            sourceState == QbElement::ElementStatePaused)
            this->m_audioOutput->setState(sourceState);

        QObject::connect(this->m_source.data(),
                         SIGNAL(stateChanged(ElementState)),
                         this->m_audioOutput.data(),
                         SLOT(setState(ElementState)));
    }
    else
    {
        this->m_audioOutput->setState(QbElement::ElementStateNull);

        QObject::disconnect(this->m_source.data(),
                            SIGNAL(stateChanged(ElementState)),
                            this->m_audioOutput.data(),
                            SLOT(setState(ElementState)));
    }
}

void MediaTools::setVideoRecordFormats(QList<QStringList> videoRecordFormats)
{
    this->m_videoRecordFormats = videoRecordFormats;
}

void MediaTools::setStreams(QList<QStringList> streams)
{
    this->m_streams = streams;

    emit this->devicesModified();
}

void MediaTools::setWindowSize(QSize windowSize)
{
    this->m_windowSize = windowSize;
}

void MediaTools::resetDevice()
{
    this->setDevice("");
}

void MediaTools::resetVideoFormat(QString device)
{
    device = device.isEmpty()? this->device(): device;

    if (!device.isEmpty())
    {
        QVariantList videoFormats = this->videoFormats(device);
        this->setVideoFormat(videoFormats.at(0).toList(), device);
    }
}

void MediaTools::resetEffectsPreview()
{
    this->setEffectsPreview(false);
}

void MediaTools::resetPlayAudioFromSource()
{
    this->setPlayAudioFromSource(true);
}

void MediaTools::resetRecordAudioFrom()
{
    this->setRecordAudioFrom(RecordFromMic);
}

void MediaTools::resetRecording()
{
    this->setRecording(false);
}

void MediaTools::resetVideoRecordFormats()
{
    this->setVideoRecordFormats(QList<QStringList>());
}

void MediaTools::resetStreams()
{
    this->setStreams(QList<QStringList>());
}

void MediaTools::resetWindowSize()
{
    this->setWindowSize(QSize());
}

void MediaTools::loadConfigs()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig(this->m_appEnvironment->configFileName());

    KConfigGroup webcamConfigs = config->group("GeneralConfigs");
    this->setPlayAudioFromSource(webcamConfigs.readEntry("playAudio", true));

    int recordFrom = webcamConfigs.readEntry("recordAudioFrom",
                                             static_cast<int>(RecordFromMic));

    this->setRecordAudioFrom(static_cast<RecordFrom>(recordFrom));

    QStringList windowSize = webcamConfigs.readEntry("windowSize", "320,240").split(",", QString::SkipEmptyParts);
    this->m_windowSize = QSize(windowSize.at(0).trimmed().toInt(),
                               windowSize.at(1).trimmed().toInt());

    KConfigGroup effectsConfigs = config->group("Effects");

    QString effcts = effectsConfigs.readEntry("effects", "");

    if (!effcts.isEmpty())
        this->setEffects(effcts.split("&&", QString::SkipEmptyParts));

    KConfigGroup videoFormatsConfigs = config->group("VideoRecordFormats");

    QString videoRecordFormats = videoFormatsConfigs.
                    readEntry("formats",
                              "webm::"
                              "-r 25 -vcodec libvpx -acodec libvorbis -f webm&&"
                              "ogv, ogg::"
                              "-r 25 -vcodec libtheora -acodec libvorbis -f ogg");

    if (!videoRecordFormats.isEmpty())
        foreach (QString fmt, videoRecordFormats.split("&&", QString::SkipEmptyParts))
        {
            QStringList params = fmt.split("::", QString::SkipEmptyParts);

            this->setVideoRecordFormat(params.at(0),
                                       params.at(1));
        }

    KConfigGroup streamsConfig = config->group("CustomStreams");
    QString streams = streamsConfig.readEntry("streams", "");

    if (!streams.isEmpty())
        foreach (QString fmt, streams.split("&&"))
        {
            QStringList params = fmt.split("::");

            this->setStream(params.at(0).trimmed(),
                            params.at(1).trimmed());
        }
}

void MediaTools::saveConfigs()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig(this->m_appEnvironment->configFileName());

    KConfigGroup webcamConfigs = config->group("GeneralConfigs");

    webcamConfigs.writeEntry("playAudio", this->playAudioFromSource());

    webcamConfigs.writeEntry("recordAudioFrom",
                             static_cast<int>(this->recordAudioFrom()));

    webcamConfigs.writeEntry("windowSize", QString("%1,%2").arg(this->m_windowSize.width())
                                                           .arg(this->m_windowSize.height()));

    KConfigGroup effectsConfigs = config->group("Effects");

    effectsConfigs.writeEntry("effects", this->m_effectsList.join("&&"));

    KConfigGroup videoFormatsConfigs = config->group("VideoRecordFormats");

    QStringList videoRecordFormats;

    foreach (QStringList format, this->m_videoRecordFormats)
        videoRecordFormats << QString("%1::%2").arg(format[0])
                                               .arg(format[1]);

    videoFormatsConfigs.writeEntry("formats",
                                   videoRecordFormats.join("&&"));

    KConfigGroup streamsConfigs = config->group("CustomStreams");

    QStringList streams;

    foreach (QStringList stream, this->m_streams)
        streams << QString("%1::%2").arg(stream.at(0))
                                    .arg(stream.at(1));

    streamsConfigs.writeEntry("streams", streams.join("&&"));

    config->sync();
}

void MediaTools::setEffects(QStringList effects)
{
    if (this->m_effectsList == effects)
        return;

    this->m_effectsList = effects;

    if (this->m_effectsList.isEmpty())
        this->m_effects->setProperty("description", "");
    else
    {
        QString description = "IN.";

        foreach (QString effect, this->m_effectsList)
            description += QString(" ! %1").arg(effect);

        description += " ! OUT.";

        this->m_effects->setProperty("description", description);
    }
}

void MediaTools::clearVideoRecordFormats()
{
    this->m_videoRecordFormats.clear();
}

void MediaTools::setStream(QString dev_name, QString description)
{
    this->m_streams << (QStringList() << dev_name
                                      << description);

    emit this->devicesModified();
}

void MediaTools::setVideoRecordFormat(QString suffix, QString options)
{
    this->m_videoRecordFormats << (QStringList() << suffix
                                                 << options);
}

void MediaTools::aboutToQuit()
{
    this->resetDevice();
    this->saveConfigs();
}

void MediaTools::reset(QString device)
{
    this->resetVideoFormat(device);

    QVariantList controls = this->listControls(device);
    QMap<QString, uint> ctrls;

    foreach (QVariant control, controls)
        ctrls[control.toList().at(0).toString()] = control.toList().at(5).toUInt();

    this->setControls(device, ctrls);
}

void MediaTools::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path)

    emit this->devicesModified();
}

void MediaTools::audioSetup()
{
    if (!this->m_mic || !this->m_audioOutput)
        return;

    if (QFileInfo("/usr/bin/pulseaudio").exists())
    {
        this->m_mic->setProperty("location", "pulse");
        this->m_audioOutput->setProperty("location", "pulse");
        this->m_audioOutput->setProperty("options", "-vn -ac 2 -f alsa");
    }
    else if (QFileInfo("/proc/asound/version").exists())
    {
        this->m_mic->setProperty("location", "hw:0");
        this->m_audioOutput->setProperty("location", "hw:0");
        this->m_audioOutput->setProperty("options", "-vn -ac 2 -f alsa");
    }
    else if (QFileInfo("/dev/dsp").exists())
    {
        this->m_mic->setProperty("location", "/dev/dsp");
        this->m_audioOutput->setProperty("location", "/dev/dsp");
        this->m_audioOutput->setProperty("options", "-vn -ac 2 -f oss");
    }
}
