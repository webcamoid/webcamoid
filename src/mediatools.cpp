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

// http://gstreamer.freedesktop.org/documentation/
// http://lists.freedesktop.org/mailman/listinfo/gstreamer-devel
// http://api.kde.org/
// http://extragear.kde.org/apps/kipi/
// http://www.mltframework.org/bin/view/MLT/WebHome
// http://cgit.freedesktop.org/gstreamer/gstreamer/tree/docs/random/porting-to-1.0.txt
// LD_PRELOAD='./libWebcamoid.so' ./webcamoid

#include <sys/ioctl.h>
#include <KSharedConfig>
#include <KConfigGroup>
#include <gst/app/gstappsink.h>
#include <linux/videodev2.h>

#include "mediatools.h"

MediaTools::MediaTools(bool watchDevices, QObject *parent): QObject(parent)
{
    gst_init(NULL, NULL);

    this->m_appEnvironment = new AppEnvironment(this);

    QObject::connect(QCoreApplication::instance(),
                     SIGNAL(aboutToQuit()),
                     this,
                     SLOT(aboutToQuit()));

    this->m_waitForEOS = false;

    this->m_mainBin = NULL;
    this->m_mainPipeline = NULL;
    this->m_captureDevice = NULL;
    this->m_effectsBin = NULL;
    this->m_effectsPreviewBin = NULL;
    this->m_recordingBin = NULL;

    this->resetDevice();
    this->resetVideoFormat();
    this->resetEffectsPreview();
    this->resetRecordAudio();
    this->resetRecording();
    this->resetVideoRecordFormats();
    this->resetStreams();
    this->resetWindowSize();

    this->m_pipeline.setPluginsPaths(QStringList() << "share/Plugins/DesktopSrc"
                                                   << "share/Plugins/EffectsBin"
                                                   << "share/Plugins/EffectsPreviewBin"
                                                   << "share/Plugins/RecordBin"
                                                   << "share/Plugins/UriSrc"
                                                   << "share/Plugins/WebcamSrc");

    this->m_captureSrc = this->m_pipeline.add("WebcamSrc");
    this->m_captureSrc->setProperty("device", "/dev/video0");
    this->m_captureSrc->setProperty("size", QSize(640, 480));

    this->m_effectsbin = this->m_pipeline.add("EffectsBin");
    this->m_effectsbin->setProperty("effects", QStringList() << "frei0r-filter-threelay0r"
                                                             << "coloreffects preset=sepia"
                                                             << "agingtv");

    QbPipeline::link(this->m_captureSrc, this->m_effectsbin);
    QbPipeline::link(this->m_captureSrc, this);

    this->m_mainPipeline = gst_pipeline_new("MainPipeline");

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(this->m_mainPipeline));
    this->m_busWatchId = gst_bus_add_watch(bus, MediaTools::busMessage, this);
    gst_object_unref(GST_OBJECT(bus));

    QString captureHash = this->hashFromName("capture");
    QString mainBinDescription = QString("tee name=CaptureTee ! "
                                         "tee name=EffectsTee ! "
                                         "queue ! videoconvert ! "
                                         "avenc_bmp ! "
                                         "appsink name=%1 emit-signals=true max_buffers=1 drop=true").arg(captureHash);

    GError *error = NULL;

    this->m_mainBin = gst_parse_bin_from_description(mainBinDescription.toUtf8().constData(),
                                                     FALSE,
                                                     &error);

    if (!error)
    {
        g_object_set(GST_OBJECT(this->m_mainBin), "name", "mainBin", NULL);

        GstElement *capture = gst_bin_get_by_name(GST_BIN(this->m_mainBin), captureHash.toUtf8().constData());
        this->m_callBacks[captureHash] = g_signal_connect(capture, "new-sample",  G_CALLBACK(MediaTools::readFrame), this);
        gst_object_unref(GST_OBJECT(capture));

        gst_bin_add(GST_BIN(this->m_mainPipeline), this->m_mainBin);
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
    this->m_pipeline.setState(QbElement::ElementStatePlaying);
}

MediaTools::~MediaTools()
{
    this->resetDevice();
    this->saveConfigs();

    g_source_remove(this->m_busWatchId);
    gst_object_unref(GST_OBJECT(this->m_mainPipeline));
}

void MediaTools::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        packet.caps().mimeType() != "video/x-raw" ||
        packet.caps().property("format") != "RGB")
        return;

    QImage iFrame((const uchar *) packet.data(),
                  packet.caps().property("width").toInt(),
                  packet.caps().property("height").toInt(),
                  QImage::Format_RGB888);

    emit this->frameReady(iFrame);
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
    return this->m_effectsPreview;
}

bool MediaTools::recordAudio()
{
    return this->m_recordAudio;
}

bool MediaTools::recording()
{
    return this->m_recording;
}

QVariantList MediaTools::videoRecordFormats()
{
    return this->m_videoRecordFormats;
}

QVariantList MediaTools::streams()
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
        fmt.index = 0;
        fmt.type = type;

        while (ioctl(deviceFile.handle(), VIDIOC_ENUM_FMT, &fmt) >= 0)
        {
            v4l2_frmsizeenum frmsize;
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

QVariantList MediaTools::captureDevices()
{
    QVariantList webcamsDevices;
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

    foreach (QString devicePath, devices)
    {
        device.setFileName(devicesDir.absoluteFilePath(devicePath));

        if (device.open(QIODevice::ReadWrite))
        {
            ioctl(device.handle(), VIDIOC_QUERYCAP, &capability);

            if (capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)
            {
                QVariantList cap;

                cap << device.fileName()
                    << QString((const char *) capability.card)
                    << StreamTypeWebcam;

                webcamsDevices << QVariant(cap);
            }

            device.close();
        }
    }

    this->m_webcams = webcamsDevices;

    QVariantList desktopDevice = QVariantList() << "desktop"
                                                << this->tr("Desktop")
                                                << StreamTypeDesktop;

    QVariantList allDevices = webcamsDevices +
                              this->m_streams +
                              QVariantList() << QVariant(desktopDevice);

    return allDevices;
}

QVariantList MediaTools::listControls(QString dev_name)
{
    v4l2_queryctrl queryctrl;
    queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    QVariantList controls;

    QFile device(dev_name);

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return controls;

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
        ctrl.id = user_ctrl.id;
        ctrl.value = user_ctrl.value;
        ioctl(device.handle(), VIDIOC_S_CTRL, &ctrl);
    }

    if (!mpeg_ctrls.empty())
    {
        v4l2_ext_controls ctrls;
        ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
        ctrls.count = mpeg_ctrls.size();
        ctrls.controls = &mpeg_ctrls[0];
        ioctl(device.handle(), VIDIOC_S_EXT_CTRLS, &ctrls);
    }

    device.close();

    return true;
}

QVariantMap MediaTools::featuresMatrix()
{
    QVariantMap features;
    QStringList availableElements;

    GList *headElement = gst_registry_get_feature_list(gst_registry_get(), GST_TYPE_ELEMENT_FACTORY);
    GList *element = headElement;

    while (element)
    {
        GstPluginFeature *pluginFeature = GST_PLUGIN_FEATURE(element->data);
        availableElements << QString(gst_plugin_feature_get_name(pluginFeature));
        element = g_list_next(element);
    }

    gst_plugin_feature_list_free(headElement);

    QStringList elements;
    bool libAvailable = true;

    elements << "appsink"
             << "appsrc"
             << "filesink"
             << "queue"
             << "tee"
             << "uridecodebin";

    // GStreamer Core:
    foreach (QString element, elements)
        if (!availableElements.contains(element))
        {
            libAvailable = false;

            break;
        }

    features["gst-core"] = QVariantList() << libAvailable
                                          << "GStreamer Core"
                                          << this->tr("Basic functionality.");

    elements.clear();
    libAvailable = true;

    elements << "alsasrc"
             << "audioconvert"
             << "decodebin"
             << "videoconvert"
             << "theoraenc"
             << "videoscale"
             << "vorbisenc";

    // GStreamer Base Plugins:
    foreach (QString element, elements)
        if (!availableElements.contains(element))
        {
            libAvailable = false;

            break;
        }

    features["gst-base-plugins"] = QVariantList() << libAvailable
                                                  << "GStreamer Base Plugins"
                                                  << this->tr("Transcoding and audio source.");

    elements.clear();
    libAvailable = true;

    elements << "agingtv"
             << "dicetv"
             << "edgetv"
             << "optv"
             << "quarktv"
             << "radioactv"
             << "revtv"
             << "rippletv"
             << "shagadelictv"
             << "streaktv"
             << "v4l2src"
             << "vertigotv"
             << "videobalance"
             << "videoflip"
             << "warptv"
             << "ximagesrc";

    // GStreamer Good Plugins:
    foreach (QString element, elements)
        if (!availableElements.contains(element))
        {
            libAvailable = false;

            break;
        }

    features["gst-good-plugins"] = QVariantList() << libAvailable
                                                  << "GStreamer Good Plugins"
                                                  << this->tr("Basic sources and effects.");

    elements.clear();
    libAvailable = true;

    elements << "bulge"
             << "burn"
             << "chromium"
             << "coloreffects"
             << "exclusion"
             << "fisheye"
             << "kaleidoscope"
             << "marble"
             << "mirror"
             << "pinch"
             << "solarize"
             << "sphere"
             << "stretch"
             << "tunnel"
             << "twirl"
             << "vp8enc"
             << "waterripple";

    // GStreamer Bad Plugins:
    foreach (QString element, elements)
        if (!availableElements.contains(element))
        {
            libAvailable = false;

            break;
        }

    features["gst-bad-plugins"] = QVariantList() << libAvailable
                                                 << "GStreamer Bad Plugins"
                                                 << this->tr("Effects and some codecs.");

    elements.clear();
    libAvailable = true;

    elements << "frei0r-filter-cartoon"
             << "frei0r-filter-delaygrab"
             << "frei0r-filter-distort0r"
             << "frei0r-filter-equaliz0r"
             << "frei0r-filter-hqdn3d"
             << "frei0r-filter-invert0r"
             << "frei0r-filter-nervous"
             << "frei0r-filter-pixeliz0r"
             << "frei0r-filter-primaries"
             << "frei0r-filter-sobel"
             << "frei0r-filter-sop-sat"
             << "frei0r-filter-threelay0r"
             << "frei0r-filter-twolay0r";

    // frei0r Plugins:
    foreach (QString element, elements)
        if (!availableElements.contains(element))
        {
            libAvailable = false;

            break;
        }

    features["frei0r-plugins"] = QVariantList() << libAvailable
                                                << "frei0r Plugins"
                                                << this->tr("Extra effects.");

    elements.clear();
    libAvailable = true;

    elements << "avdec_bmp"
             << "avenc_bmp";

    // GStreamer FFmpeg Plugins:
    foreach (QString element, elements)
        if (!availableElements.contains(element))
        {
            libAvailable = false;

            break;
        }

    features["gst-ffmpeg"] = QVariantList() << libAvailable
                                            << "GStreamer FFmpeg Plugins"
                                            << this->tr("Basic functionality.");

    return features;
}

QMap<QString, QString> MediaTools::availableEffects()
{
    QVariantMap features = this->featuresMatrix();
    QMap<QString, QString> effects;

    if (features["gst-good-plugins"].toList().at(0).toBool())
    {
        effects["agingtv"] = this->tr("Old");
        effects["dicetv"] = this->tr("Dices");
        effects["edgetv"] = this->tr("Edges");
        effects["optv"] = this->tr("Hypnotic");
        effects["quarktv"] = this->tr("Quark");
        effects["radioactv"] = this->tr("Radioactive");
        effects["revtv"] = this->tr("Scan Lines");
        effects["rippletv"] = this->tr("Ripple");
        effects["shagadelictv"] = this->tr("Psychedelic");
        effects["streaktv"] = this->tr("Streak");
        effects["vertigotv"] = this->tr("Vertigo");
        effects["videobalance saturation=1.5 hue=-0.5"] = this->tr("Hulk");
        effects["videobalance saturation=1.5 hue=+0.5"] = this->tr("Mauve");
        effects["videobalance saturation=0"] = this->tr("Noir");
        effects["videobalance saturation=2"] = this->tr("Saturation");
        effects["videoflip method=clockwise"] = this->tr("Rotate Right");
        effects["videoflip method=rotate-180"] = this->tr("Rotate 180");
        effects["videoflip method=counterclockwise"] = this->tr("Rotate Left");
        effects["videoflip method=horizontal-flip"] = this->tr("Flip horizontally");
        effects["videoflip method=vertical-flip"] = this->tr("Flip vertically");
        effects["videoflip method=upper-left-diagonal"] = this->tr("Flip Top Left");
        effects["videoflip method=upper-right-diagonal"] = this->tr("Flip Top Right");
        effects["warptv"] = this->tr("Warp");
    }

    if (features["gst-bad-plugins"].toList().at(0).toBool())
    {
        effects["bulge"] = this->tr("Bulge");
        effects["burn"] = this->tr("Burn");
        effects["chromium"] = this->tr("Chromium");
        effects["coloreffects preset=heat"] = this->tr("Heat");
        effects["coloreffects preset=sepia"] = this->tr("Sepia");
        effects["coloreffects preset=xray"] = this->tr("X-Ray");
        effects["coloreffects preset=xpro"] = this->tr("X-Pro");
        effects["exclusion"] = this->tr("Exclusion");
        effects["fisheye"] = this->tr("Fish Eye");
        effects["kaleidoscope"] = this->tr("Kaleidoscope");
        effects["marble"] = this->tr("Marble");
        effects["mirror"] = this->tr("Mirror");
        effects["pinch"] = this->tr("Pinch");
        effects["solarize"] = this->tr("Solarize");
        effects["sphere"] = this->tr("Sphere");
        effects["square"] = this->tr("Square");
        effects["stretch"] = this->tr("Stretch");
        effects["tunnel"] = this->tr("Tunnel");
        effects["twirl"] = this->tr("Twirl");
        effects["waterripple"] = this->tr("Water Ripple");
    }

    if (features["frei0r-plugins"].toList().at(0).toBool())
    {
        effects["frei0r-filter-cartoon"] = this->tr("Cartoon");
        effects["frei0r-filter-delaygrab"] = this->tr("Past");
        effects["frei0r-filter-distort0r"] = this->tr("Distort");
        effects["frei0r-filter-equaliz0r"] = this->tr("Equalize");
        effects["frei0r-filter-hqdn3d spatial=0.5 temporal=1.0"] = this->tr("Drugs");
        effects["frei0r-filter-invert0r"] = this->tr("Invert");
        effects["frei0r-filter-nervous"] = this->tr("Nervous");
        effects["frei0r-filter-pixeliz0r"] = this->tr("Pixelate");
        effects["frei0r-filter-primaries"] = this->tr("Primary Colors");
        effects["frei0r-filter-sobel"] = this->tr("Sobel");
        effects["frei0r-filter-sop-sat"] = this->tr("Crazy Colors");
        effects["frei0r-filter-threelay0r"] = this->tr("The Godfather");
        effects["frei0r-filter-twolay0r"] = this->tr("Che Guevara");
    }

    return effects;
}

QStringList MediaTools::currentEffects()
{
    return this->m_effects;
}

QStringList MediaTools::bestVideoRecordFormat(QString fileName)
{
    QString ext = QFileInfo(fileName).completeSuffix();

    if (ext.isEmpty())
        return QStringList();

    foreach (QVariant format, this->m_videoRecordFormats)
        foreach (QString s, format.toList().at(0).toString().split(",", QString::SkipEmptyParts))
            if (s.toLower().trimmed() == ext)
                return format.toStringList();

    return QStringList();
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
        ctrl.id = queryctrl->id;

        if (ioctl(dev_fd, VIDIOC_G_CTRL, &ctrl))
            return QVariantList();

        ext_ctrl.value = ctrl.value;
    }

    v4l2_querymenu qmenu;
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

QMap<QString, uint> MediaTools::findControls(int dev_fd)
{
    v4l2_queryctrl qctrl;
    qctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    QMap<QString, uint> controls;

    while (ioctl(dev_fd, VIDIOC_QUERYCTRL, &qctrl) == 0)
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

        if (ioctl(dev_fd, VIDIOC_QUERYCTRL, &qctrl) == 0 &&
           !(qctrl.flags & V4L2_CTRL_FLAG_DISABLED))
            controls[QString((const char *) qctrl.name)] = qctrl.id;
    }

    qctrl.id = V4L2_CID_PRIVATE_BASE;

    while (ioctl(dev_fd, VIDIOC_QUERYCTRL, &qctrl) == 0)
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

MediaTools::StreamType MediaTools::deviceType(QString device)
{
    QStringList webcams;

    foreach (QVariant deviceName, this->m_webcams)
        webcams << deviceName.toList().at(0).toString();

    QStringList streams;

    foreach (QVariant deviceName, this->m_streams)
        streams << deviceName.toList().at(0).toString();

    if (webcams.contains(device))
        return StreamTypeWebcam;
    else if (streams.contains(device))
        return StreamTypeURI;
    else if (device == "desktop")
        return StreamTypeDesktop;
    else
        return StreamTypeUnknown;
}

gboolean MediaTools::busMessage(GstBus *bus, GstMessage *message, gpointer self)
{
    Q_UNUSED(bus)

    MediaTools *mediaTools = (MediaTools* ) self;

    if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS)
    {
        if (mediaTools->m_waitForEOS)
            mediaTools->m_waitForEOS = false;
        else
            gst_element_set_state(mediaTools->m_mainPipeline, GST_STATE_NULL);
    }
    else if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR)
    {
        GError *err;
        gchar *debug;

        gst_message_parse_error(message, &err, &debug);
        qDebug() << "Error: " << err->message;
        g_error_free (err);
        g_free(debug);

        gst_element_set_state(mediaTools->m_mainPipeline, GST_STATE_NULL);
    }
    else
    {
    }

    return TRUE;
}

void MediaTools::readFrame(GstElement *appsink, gpointer self)
{
    MediaTools *mediaTools = (MediaTools* ) self;

    mediaTools->m_mutex.lock();

    gchar *elementName = gst_element_get_name(appsink);
    QString pipename = mediaTools->nameFromHash(elementName);
    g_free(elementName);

    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo mapInfo;
    QImage frame;

    if (gst_buffer_map(buffer, &mapInfo, GST_MAP_READ))
    {
        frame = QImage::fromData((const uchar *) mapInfo.data, mapInfo.size);
        gst_buffer_unmap(buffer, &mapInfo);
    }

    gst_buffer_unref(buffer);
    gst_sample_unref(sample);

    if (pipename == "capture")
    {
        emit mediaTools->frameReady(frame);

        if (frame.size() != mediaTools->m_curFrameSize)
        {
            emit mediaTools->frameSizeChanged(frame.size());
            mediaTools->m_curFrameSize = frame.size();
        }
    }
    else
        emit mediaTools->previewFrameReady(frame, pipename);

    mediaTools->m_mutex.unlock();
}

void MediaTools::setRecordAudio(bool recordAudio)
{
    this->m_recordAudio = recordAudio;
}

void MediaTools::setRecording(bool recording, QString fileName)
{
    if (!this->m_mainPipeline)
    {
        this->m_recording = false;
        emit this->recordingChanged(this->m_recording);

        return;
    }

    this->m_waitForEOS = true;

    if (this->m_recordingBin &&
        gst_element_send_event(this->m_mainPipeline, gst_event_new_eos()))
    {
        while (this->m_waitForEOS)
            QCoreApplication::processEvents();
    }

    this->m_waitForEOS = false;

    GstState state;

    gst_element_get_state(this->m_mainPipeline, &state, NULL, GST_CLOCK_TIME_NONE);

    if (state == GST_STATE_PLAYING)
        gst_element_set_state(this->m_mainPipeline, GST_STATE_NULL);

    if (this->m_recordingBin)
    {
        GstElement *effectsTee = gst_bin_get_by_name(GST_BIN(this->m_mainBin), "EffectsTee");
        GstElement *queueVideoRecording = gst_bin_get_by_name(GST_BIN(this->m_recordingBin), "VideoRecording");
        gst_element_unlink(effectsTee, queueVideoRecording);
        gst_object_unref(GST_OBJECT(effectsTee));
        gst_object_unref(GST_OBJECT(queueVideoRecording));

        gst_bin_remove(GST_BIN(this->m_mainBin), this->m_recordingBin);
        this->m_recordingBin = NULL;

        this->m_recording = false;
        emit this->recordingChanged(this->m_recording);
    }

    if (recording)
    {
        QStringList format = this->bestVideoRecordFormat(fileName);

        if (format.isEmpty())
        {
            this->m_recording = false;
            emit this->recordingChanged(this->m_recording);

            return;
        }

        QString pipeline = QString("queue name=VideoRecording ! videoconvert ! "
                                   "%1 ! queue ! muxer. ").arg(format.at(1));

        if (this->m_recordAudio)
            pipeline += QString("autoaudiosrc ! queue ! audioconvert ! "
                                "queue ! %1 ! queue ! muxer. ").arg(format.at(2));

        pipeline += QString("%1 name=muxer ! filesink location=\"%2\"").arg(format.at(3))
                                                                       .arg(fileName);

        GError *error = NULL;

        this->m_recordingBin = gst_parse_bin_from_description(pipeline.toUtf8().constData(),
                                                              FALSE,
                                                              &error);

        if (error)
        {
            this->m_recordingBin = NULL;
            this->m_recording = false;
            emit this->recordingChanged(this->m_recording);

            return;
        }

        g_object_set(GST_OBJECT(this->m_recordingBin), "name", "recordingBin", NULL);
        gst_bin_add(GST_BIN(this->m_mainBin), this->m_recordingBin);

        GstElement *effectsTee = gst_bin_get_by_name(GST_BIN(this->m_mainBin), "EffectsTee");
        GstElement *queueVideoRecording = gst_bin_get_by_name(GST_BIN(this->m_recordingBin), "VideoRecording");
        gst_element_link(effectsTee, queueVideoRecording);
        gst_object_unref(GST_OBJECT(effectsTee));
        gst_object_unref(GST_OBJECT(queueVideoRecording));

        this->m_recording = true;
        emit this->recordingChanged(this->m_recording);
    }

    if (state == GST_STATE_PLAYING)
        gst_element_set_state(this->m_mainPipeline, GST_STATE_PLAYING);
}

void MediaTools::setDevice(QString device)
{
    if (!this->m_mainPipeline)
    {
        this->m_device = "";

        return;
    }

    GstState state;

    gst_element_get_state(this->m_mainPipeline, &state, NULL, GST_CLOCK_TIME_NONE);

    if (state == GST_STATE_PLAYING)
        gst_element_set_state(this->m_mainPipeline, GST_STATE_NULL);

    if (this->m_captureDevice)
    {
        this->resetRecording();
        this->resetEffectsPreview();

        GstElement *capture = gst_bin_get_by_name(GST_BIN(this->m_captureDevice), "capture");
        GstElement *captureTee = gst_bin_get_by_name(GST_BIN(this->m_mainBin), "CaptureTee");
        gst_element_unlink(capture, captureTee);
        gst_object_unref(GST_OBJECT(capture));
        gst_object_unref(GST_OBJECT(captureTee));

        gst_bin_remove(GST_BIN(this->m_mainBin), this->m_captureDevice);
        this->m_captureDevice = NULL;

        this->m_device = "";
        emit this->deviceChanged(this->m_device);
    }

    if (!device.isEmpty())
    {
        StreamType deviceType = this->deviceType(device);
        GError *error = NULL;

        if (deviceType == StreamTypeWebcam)
        {
            QVariantList fmt = this->videoFormat(device);

            QString description = QString("v4l2src device=%1 ! "
                                          "capsfilter name=capture "
                                          "caps=video/x-raw,width=%2,height=%3").arg(device)
                                                                                    .arg(fmt.at(0).toInt())
                                                                                    .arg(fmt.at(1).toInt());

            this->m_captureDevice = gst_parse_bin_from_description(description.toUtf8().constData(),
                                                                   FALSE,
                                                                   &error);

            g_object_set(GST_OBJECT(this->m_captureDevice), "name", "captureDevice", NULL);
        }
        else if (deviceType == StreamTypeURI)
        {
            QString scheme = QUrl(device).scheme();
            QString pipeline;

            bool hasAudio = false;
            bool playAudio = false;

            foreach (QVariant stream, this->m_streams)
                if (stream.toList().at(0).toString() == device)
                {
                    hasAudio = stream.toList().at(2).toBool();
                    playAudio = stream.toList().at(3).toBool();

                    break;
                }

            if (scheme.isEmpty() || scheme == "file")
            {
                device.replace(QRegExp("^file://"), "");

                if (!QFile::exists(device))
                {
                    this->m_device = "";

                    return;
                }

                pipeline = "filesrc";
            }
            else if (scheme == "mms" || scheme == "mmsu" || scheme == "mmst")
                pipeline = "mmssrc";
            else if (scheme == "rtsp")
                pipeline = "rtspsrc";
            else
                pipeline = "souphttpsrc";

            pipeline += " name=captureResource ! decodebin name=dec ! videoconvert name=capture";

            if (hasAudio)
            {
                pipeline += " dec. ! ";

                if (playAudio)
                    pipeline += "autoaudiosink";
                else
                    pipeline += "fakesink";
            }

            this->m_captureDevice = gst_parse_bin_from_description(pipeline.toUtf8().constData(),
                                                                   FALSE,
                                                                   &error);

            g_object_set(GST_OBJECT(this->m_captureDevice), "name", "captureDevice", NULL);

            GstElement *captureResource = gst_bin_get_by_name(GST_BIN(this->m_captureDevice), "captureResource");
            g_object_set(GST_OBJECT(captureResource), "location", device.toUtf8().constData(), NULL);
            gst_object_unref(GST_OBJECT(captureResource));
        }
        else if (deviceType == StreamTypeDesktop)
        {
            this->m_captureDevice = gst_parse_bin_from_description("ximagesrc name=capture show-pointer=true",
                                                                   FALSE,
                                                                   &error);

            g_object_set(GST_OBJECT(this->m_captureDevice), "name", "captureDevice", NULL);
        }
        else
            return;

        if (error)
        {
            this->m_captureDevice = NULL;
            this->m_device = "";

            return;
        }

        gst_bin_add(GST_BIN(this->m_mainBin), this->m_captureDevice);

        GstElement *capture = gst_bin_get_by_name(GST_BIN(this->m_captureDevice), "capture");
        GstElement *captureTee = gst_bin_get_by_name(GST_BIN(this->m_mainBin), "CaptureTee");
        gst_element_link(capture, captureTee);
        gst_object_unref(GST_OBJECT(capture));
        gst_object_unref(GST_OBJECT(captureTee));

        gst_element_set_state(this->m_mainPipeline, GST_STATE_PLAYING);

        this->m_device = device;
        emit this->deviceChanged(this->m_device);

        //self.gstError.emit()
    }
}

void MediaTools::setVideoFormat(QVariantList videoFormat, QString device)
{
    QString curDevice = this->device();
    GstState state;

    gst_element_get_state(this->m_mainPipeline, &state, NULL, GST_CLOCK_TIME_NONE);

    if (state == GST_STATE_PLAYING && (device.isEmpty() || device == curDevice))
        this->resetDevice();

    QFile deviceFile(device.isEmpty()? curDevice: device);

    if (!deviceFile.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return;

    v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(deviceFile.handle(), VIDIOC_G_FMT, &fmt) == 0)
    {
        fmt.fmt.pix.width = videoFormat.at(0).toUInt();
        fmt.fmt.pix.height = videoFormat.at(1).toUInt();
        fmt.fmt.pix.pixelformat = videoFormat.at(2).toUInt();

        ioctl(deviceFile.handle(), VIDIOC_S_FMT, &fmt);
    }

    deviceFile.close();

    if (state == GST_STATE_PLAYING && (device.isEmpty() || device == curDevice))
        this->setDevice(device.isEmpty()? curDevice: device);
}

void MediaTools::setEffectsPreview(bool effectsPreview)
{
    this->m_effectsPreview = effectsPreview;

    if (!this->m_mainPipeline)
        return;

    GstState state;

    gst_element_get_state(this->m_mainPipeline, &state, NULL, GST_CLOCK_TIME_NONE);

    if (state == GST_STATE_PLAYING)
        gst_element_set_state(this->m_mainPipeline, GST_STATE_PAUSED);

    if (this->m_effectsPreviewBin)
    {
        gst_element_set_state(this->m_effectsPreviewBin, GST_STATE_NULL);

        foreach (QString effect, this->availableEffects().keys())
        {
            QString previewHash = this->hashFromName(effect);
            GstElement *preview = gst_bin_get_by_name(GST_BIN(this->m_mainBin), previewHash.toUtf8().constData());

            g_signal_handler_disconnect(preview, this->m_callBacks[previewHash]);
            gst_object_unref(GST_OBJECT(preview));
        }

        GstElement *queueEffectsPreview = gst_bin_get_by_name(GST_BIN(this->m_effectsPreviewBin), "EffectsPreview");
        GstElement *captureTee = gst_bin_get_by_name(GST_BIN(this->m_mainBin), "CaptureTee");
        gst_element_unlink(captureTee, queueEffectsPreview);
        gst_object_unref(GST_OBJECT(queueEffectsPreview));
        gst_object_unref(GST_OBJECT(captureTee));

        gst_bin_remove(GST_BIN(this->m_mainBin), this->m_effectsPreviewBin);
        this->m_effectsPreviewBin = NULL;
    }

    if (effectsPreview)
    {
        QString pipeline = QString("queue name=EffectsPreview ! videoconvert ! "
                                   "videoscale ! video/x-raw,format=RGB,width=%1,height=%2 ! tee name=preview").arg(128)
                                                                                                        .arg(96);

        foreach (QString effect, this->availableEffects().keys())
        {
            QString previewHash = this->hashFromName(effect);

            pipeline += QString(" preview. ! queue ! videoconvert ! %1 ! "
                                "videoconvert ! avenc_bmp ! "
                                "appsink name=%2 emit-signals=true max_buffers=1 drop=true").arg(effect).arg(previewHash);
        }

        GError *error = NULL;

        this->m_effectsPreviewBin = gst_parse_bin_from_description(pipeline.toUtf8().constData(),
                                                                   FALSE,
                                                                   &error);

        if (error)
        {
            this->m_effectsPreviewBin = NULL;

            return;
        }

        g_object_set(GST_OBJECT(this->m_effectsPreviewBin), "name", "effectsPreviewBin", NULL);
        gst_bin_add(GST_BIN(this->m_mainBin), this->m_effectsPreviewBin);

        gst_element_set_state(this->m_effectsPreviewBin, GST_STATE_PAUSED);

        GstElement *queueEffectsPreview = gst_bin_get_by_name(GST_BIN(this->m_effectsPreviewBin), "EffectsPreview");
        GstElement *captureTee = gst_bin_get_by_name(GST_BIN(this->m_mainBin), "CaptureTee");
        gst_element_link(captureTee, queueEffectsPreview);
        gst_object_unref(GST_OBJECT(queueEffectsPreview));
        gst_object_unref(GST_OBJECT(captureTee));

        foreach (QString effect, this->availableEffects().keys())
        {
            QString previewHash = this->hashFromName(effect);
            GstElement *preview = gst_bin_get_by_name(GST_BIN(this->m_mainBin), previewHash.toUtf8().constData());

            this->m_callBacks[previewHash] = g_signal_connect(preview, "new-sample", G_CALLBACK(MediaTools::readFrame), this);

            gst_object_unref(GST_OBJECT(preview));
        }
    }

    if (state == GST_STATE_PLAYING)
        gst_element_set_state(this->m_mainPipeline, GST_STATE_PLAYING);
}

void MediaTools::setVideoRecordFormats(QVariantList videoRecordFormats)
{
    this->m_videoRecordFormats = videoRecordFormats;
}

void MediaTools::setStreams(QVariantList streams)
{
    this->m_streams = streams;
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

void MediaTools::resetRecordAudio()
{
    this->setRecordAudio(true);
}

void MediaTools::resetRecording()
{
    this->setRecording(false);
}

void MediaTools::resetVideoRecordFormats()
{
    this->setVideoRecordFormats(QVariantList());
}

void MediaTools::resetStreams()
{
    this->setStreams(QVariantList());
}

void MediaTools::resetWindowSize()
{
    this->setWindowSize(QSize());
}

void MediaTools::loadConfigs()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig(this->m_appEnvironment->configFileName());

    KConfigGroup webcamConfigs = config->group("GeneralConfigs");
    this->enableAudioRecording(webcamConfigs.readEntry("recordAudio", true));
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
                              "vp8enc quality=10 speed=7 bitrate=1000000000::"
                              "vorbisenc::"
                              "webmmux&&"
                              "ogv, ogg::"
                              "theoraenc quality=63 bitrate=16777215::"
                              "vorbisenc::"
                              "oggmux");

    if (!videoRecordFormats.isEmpty())
        foreach (QString fmt, videoRecordFormats.split("&&", QString::SkipEmptyParts))
        {
            QStringList params = fmt.split("::", QString::SkipEmptyParts);

            this->setVideoRecordFormat(params.at(0),
                                       params.at(1),
                                       params.at(2),
                                       params.at(3));
        }

    KConfigGroup streamsConfig = config->group("CustomStreams");
    QString streams = streamsConfig.readEntry("streams", "");

    if (!streams.isEmpty())
        foreach (QString fmt, streams.split("&&"))
        {
            QStringList params = fmt.split("::");

            this->setCustomStream(params.at(0).trimmed(),
                                  params.at(1).trimmed(),
                                  params.at(2).trimmed().toInt()? true: false,
                                  params.at(3).trimmed().toInt()? true: false);
        }
}

void MediaTools::saveConfigs()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig(this->m_appEnvironment->configFileName());

    KConfigGroup webcamConfigs = config->group("GeneralConfigs");

    webcamConfigs.writeEntry("recordAudio", this->m_recordAudio);

    webcamConfigs.writeEntry("windowSize", QString("%1,%2").arg(this->m_windowSize.width())
                                                           .arg(this->m_windowSize.height()));

    KConfigGroup effectsConfigs = config->group("Effects");

    effectsConfigs.writeEntry("effects", this->m_effects.join("&&"));

    KConfigGroup videoFormatsConfigs = config->group("VideoRecordFormats");

    QStringList videoRecordFormats;

    foreach (QVariant format, this->m_videoRecordFormats)
        videoRecordFormats << QString("%1::%2::%3::%4").arg(format.toList().at(0).toString())
                                                       .arg(format.toList().at(1).toString())
                                                       .arg(format.toList().at(2).toString())
                                                       .arg(format.toList().at(3).toString());

    videoFormatsConfigs.writeEntry("formats",
                                   videoRecordFormats.join("&&"));

    KConfigGroup streamsConfigs = config->group("CustomStreams");

    QStringList streams;

    foreach (QVariant stream, this->m_streams)
        streams << QString("%1::%2::%3::%4").arg(stream.toList().at(0).toString())
                                            .arg(stream.toList().at(1).toString())
                                            .arg(stream.toList().at(2).toInt())
                                            .arg(stream.toList().at(3).toInt());

    streamsConfigs.writeEntry("streams", streams.join("&&"));

    config->sync();
}

void MediaTools::setEffects(QStringList effects)
{
    if (this->m_effects == effects)
        return;

    this->m_effects = effects;

    GstState state;

    gst_element_get_state(this->m_mainPipeline, &state, NULL, GST_CLOCK_TIME_NONE);

    if (state == GST_STATE_PLAYING)
        gst_element_set_state(this->m_mainPipeline, GST_STATE_PAUSED);

    GstElement *captureTee = gst_bin_get_by_name(GST_BIN(this->m_mainBin), "CaptureTee");
    GstElement *effectsTee = gst_bin_get_by_name(GST_BIN(this->m_mainBin), "EffectsTee");

    if (this->m_effectsBin)
    {
        gst_element_set_state(this->m_effectsBin, GST_STATE_NULL);

        GstElement *effectsBinIn = gst_bin_get_by_name(GST_BIN(this->m_effectsBin), "EffectsBinIn");
        GstElement *effectsBinOut = gst_bin_get_by_name(GST_BIN(this->m_effectsBin), "EffectsBinOut");

        gst_element_unlink(captureTee, effectsBinIn);
        gst_element_unlink(effectsBinOut, effectsTee);
        gst_bin_remove(GST_BIN(this->m_mainBin), this->m_effectsBin);
        gst_element_link(captureTee, effectsTee);

        gst_object_unref(GST_OBJECT(effectsBinIn));
        gst_object_unref(GST_OBJECT(effectsBinOut));
        this->m_effectsBin = NULL;
    }

    if (!this->m_effects.isEmpty())
    {
        QString pipeline = "queue name=EffectsBinIn";

        foreach (QString effect, this->m_effects)
            pipeline += QString(" ! videoconvert ! %1").arg(effect);

        pipeline += " ! identity name=EffectsBinOut";

        GError *error = NULL;

        this->m_effectsBin = gst_parse_bin_from_description(pipeline.toUtf8().constData(),
                                                            FALSE,
                                                            &error);

        if (error)
        {
            this->m_effectsBin = NULL;

            return;
        }

        g_object_set(GST_OBJECT(this->m_effectsBin), "name", "effectsBin", NULL);

        GstElement *effectsBinIn = gst_bin_get_by_name(GST_BIN(this->m_effectsBin), "EffectsBinIn");
        GstElement *effectsBinOut = gst_bin_get_by_name(GST_BIN(this->m_effectsBin), "EffectsBinOut");

        gst_element_unlink(captureTee, effectsTee);
        gst_bin_add(GST_BIN(this->m_mainBin), this->m_effectsBin);
        gst_element_link(captureTee, effectsBinIn);
        gst_element_link(effectsBinOut, effectsTee);

        gst_object_unref(GST_OBJECT(effectsBinIn));
        gst_object_unref(GST_OBJECT(effectsBinOut));
    }

    gst_object_unref(GST_OBJECT(captureTee));
    gst_object_unref(GST_OBJECT(effectsTee));

    if (state == GST_STATE_PLAYING)
        gst_element_set_state(this->m_mainPipeline, GST_STATE_PLAYING);
}

void MediaTools::clearVideoRecordFormats()
{
    this->m_videoRecordFormats.clear();
}

void MediaTools::clearCustomStreams()
{
    this->m_streams.clear();
    emit this->devicesModified();
}

void MediaTools::setCustomStream(QString dev_name, QString description, bool hasAudio, bool playAudio)
{
    this->m_streams << QVariant(QVariantList() << dev_name
                                               << description
                                               << hasAudio
                                               << playAudio
                                               << StreamTypeURI);

    emit this->devicesModified();
}

void MediaTools::enableAudioRecording(bool enable)
{
    this->m_recordAudio = enable;
}

void MediaTools::setVideoRecordFormat(QString suffix, QString videoEncoder,
                                     QString audioEncoder, QString muxer)
{
    this->m_videoRecordFormats << QVariant(QVariantList() << suffix
                                                          << videoEncoder
                                                          << audioEncoder
                                                          << muxer);
}

void MediaTools::aboutToQuit()
{
    this->resetDevice();
    this->saveConfigs();
}

void MediaTools::mutexLock()
{
    this->m_mutex.lock();
}

void MediaTools::mutexUnlock()
{
    this->m_mutex.unlock();
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
