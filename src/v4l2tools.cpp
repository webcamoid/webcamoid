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

// http://gstreamer.freedesktop.org/data/doc/gstreamer/head/qt-gstreamer/html/index.html
// http://cgit.freedesktop.org/gstreamer/gstreamer/tree/tools/gst-inspect.c

#include <sys/ioctl.h>
#include <KSharedConfig>
#include <QGst/Init>
#include <QGst/Bus>
#include <glib.h>
#include <gst/gst.h>
#include <gst/gstregistry.h>
#include <linux/videodev2.h>

#include "v4l2tools.h"

V4L2Tools::V4L2Tools(bool watchDevices, QObject *parent): QObject(parent)
{
    this->appEnvironment = new AppEnvironment(this);

    QObject::connect(QCoreApplication::instance(),
                     SIGNAL(aboutToQuit()),
                     this,
                     SLOT(aboutToQuit()));

    this->playing = false;
    this->recording = false;
    this->recordAudio = true;

    this->curOutVidFmt = "webm";

    this->mainPipeline = QGst::Pipeline::create("MainPipeline");
    this->mainPipeline->bus()->addSignalWatch();

    QGlib::connect(this->mainPipeline->bus(),
                   "message",
                   this,
                   &V4L2Tools::busMessage);

    QString captureHash = this->hashFromName("capture");

    this->mainBin = QGst::Bin::fromDescription(QString("tee name=CaptureTee ! "
                                                       "tee name=EffectsTee ! "
                                                       "ffmpegcolorspace ! "
                                                       "ffenc_bmp ! "
                                                       "appsink name=%1 emit-signals=true").arg(captureHash));

    QGst::ElementPtr capture = this->mainBin->getElementByName(captureHash);

    QGlib::connect(capture,
                   "new-buffer",
                   this,
                   &V4L2Tools::readFrame);

    this->mainPipeline->add(this->mainBin);

    if (watchDevices)
    {
        this->fsWatcher = new QFileSystemWatcher(QStringList() << "/dev", this);

        QObject::connect(this->fsWatcher,
                         SIGNAL(directoryChanged()),
                         this,
                         SIGNAL(devicesModified()));
    }

    this->loadConfigs();
}

V4L2Tools::~V4L2Tools()
{
    this->stopCurrentDevice();
    this->saveConfigs();
}

QString V4L2Tools::fcc2s(uint val)
{
    QString s = "";

    s += QChar(val & 0xff);
    s += QChar((val >> 8) & 0xff);
    s += QChar((val >> 16) & 0xff);
    s += QChar((val >> 24) & 0xff);

    return s;
}

QVariantList V4L2Tools::videoFormats(QString dev_name)
{
    QFile device(dev_name);
    QVariantList formats;

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return formats;

    QList<v4l2_buf_type> bufType;

    bufType << V4L2_BUF_TYPE_VIDEO_CAPTURE
            << V4L2_BUF_TYPE_VIDEO_OUTPUT
            << V4L2_BUF_TYPE_VIDEO_OVERLAY;

    foreach (v4l2_buf_type type, bufType)
    {
        struct v4l2_fmtdesc fmt;
        fmt.index = 0;
        fmt.type = type;

        while (ioctl(device.handle(), VIDIOC_ENUM_FMT, &fmt) >= 0)
        {
            struct v4l2_frmsizeenum frmsize;
            frmsize.pixel_format = fmt.pixelformat;
            frmsize.index = 0;

            while (ioctl(device.handle(),
                         VIDIOC_ENUM_FRAMESIZES,
                         &frmsize) >= 0)
            {
                if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                {
                    QVariantList format;

                    format << frmsize.discrete.width
                           << frmsize.discrete.height
                           << fmt.pixelformat;

                    formats << format;
                }

                frmsize.index++;
            }

            fmt.index++;
        }
    }

    device.close();

    return formats;
}

QVariantList V4L2Tools::currentVideoFormat(QString dev_name)
{
    QFile device(dev_name);
    QVariantList format;

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return format;

    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(device.handle(), VIDIOC_G_FMT, &fmt) == 0)
        format << fmt.fmt.pix.width
               << fmt.fmt.pix.height
               << fmt.fmt.pix.pixelformat;

    device.close();

    return format;
}

bool V4L2Tools::setVideoFormat(QString dev_name, QVariantList videoFormat)
{
    QFile device(dev_name);

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return false;

    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(device.handle(), VIDIOC_G_FMT, &fmt) == 0)
    {
        fmt.fmt.pix.width = videoFormat.at(0).toUInt();
        fmt.fmt.pix.height = videoFormat.at(1).toUInt();
        fmt.fmt.pix.pixelformat = videoFormat.at(2).toUInt();

        if (fcntl.ioctl(device.handle(), VIDIOC_S_FMT, &fmt) < 0)
        {
            device.close();
            this->startDevice(dev_name, videoFormat);

            return true;
        }
    }

    device.close();

    return true;
}

QVariantList V4L2Tools::captureDevices()
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
    struct v4l2_capability capability;

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
                    << QString(capability.card)
                    << StreamTypeWebcam;

                webcamsDevices << cap;
            }

            device.close();
        }
    }

    this->webcams = webcamsDevices;

    return webcamsDevices +
           this->streams +
           QVariantList() << "desktop"
                          << this->tr("Desktop")
                          << StreamTypeDesktop;
}

QVariantList V4L2Tools::listControls(QString dev_name)
{
    struct v4l2_queryctrl queryctrl = {V4L2_CTRL_FLAG_NEXT_CTRL};
    QVariantList controls ;

    QFile device(dev_name);

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return controls;

    while (ioctl(device.handle(), VIDIOC_QUERYCTRL, &queryctrl) == 0)
    {
        QVariantList control = this->queryControl(device.handle(), &queryctrl);

        if (!control.isEmpty())
            controls << control;

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
                controls << control;
        }
    }

    queryctrl.id = V4L2_CID_PRIVATE_BASE;

    while (ioctl(device.handle(), VIDIOC_QUERYCTRL, &queryctrl) == 0)
    {
        QVariantList control = this->queryControl(device.handle(), &queryctrl);

        if (!control.isEmpty())
            controls << control;

        queryctrl.id++;
    }

    device.close();

    return controls;
}

bool V4L2Tools::setControls(QString dev_name, QMap<QString, uint> controls)
{
    QFile device(dev_name);

    if (!device.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return false;

    QMap<QString, uint> ctrl2id = this->findControls(device.handle());
    std::vector<struct v4l2_ext_control> mpeg_ctrls;
    std::vector<struct v4l2_ext_control> user_ctrls;

    foreach (QString control, controls.keys())
    {
        struct v4l2_ext_control ctrl = {0};
        ctrl.id = ctrl2id[control];
        ctrl.value = controls[control];

        if (V4L2_CTRL_ID2CLASS(ctrl.id) == V4L2_CTRL_CLASS_MPEG)
            mpeg_ctrls.push_back(ctrl);
        else
            user_ctrls.push_back(ctrl);
    }

    foreach (struct v4l2_ext_control user_ctrl, user_ctrls)
    {
        struct v4l2_control ctrl;
        ctrl.id = user_ctrl.id;
        ctrl.value = user_ctrl.value;
        ioctl(device.handle(), VIDIOC_S_CTRL, &ctrl);
    }

    if (!mpeg_ctrls.empty())
    {
        struct v4l2_ext_controls ctrls = {0};
        ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
        ctrls.count = mpeg_ctrls.size();
        ctrls.controls = &mpeg_ctrls[0];
        fcntl.ioctl(device.handle(), VIDIOC_S_EXT_CTRLS, &ctrls);
    }

    device.close();

    return true;
}

QVariantMap V4L2Tools::featuresMatrix()
{
    QVariantMap features;
    QStringList availableElements;

    GList *headElement = gst_registry_get_plugin_list(gst_registry_get());
    GList *element = headElement;

    while (element)
    {
        GstPlugin *plugin = (GstPlugin *) element->data;
        availableElements << QString(gst_plugin_get_name(plugin));
        element = g_list_next (element);
    }

    gst_plugin_list_free(headElement);

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

    features['gst-core'] = [libAvailable, 'GStreamer Core', self.tr('Basic functionality.')]

    libAvailable = True

    # GStreamer Base Plugins:
    for element in ['alsasrc',
                    'audioconvert',
                    'decodebin',
                    'ffmpegcolorspace',
                    'theoraenc',
                    'videoscale',
                    'vorbisenc']:
        if not element in availableElements:
            libAvailable = False

            break

    features['gst-base-plugins'] = [libAvailable, 'GStreamer Base Plugins', self.tr('Transcoding and audio source.')]

    libAvailable = True

    # GStreamer Good Plugins:
    for element in ['agingtv',
                    'dicetv',
                    'edgetv',
                    'optv',
                    'quarktv',
                    'radioactv',
                    'revtv',
                    'rippletv',
                    'shagadelictv',
                    'streaktv',
                    'v4l2src',
                    'vertigotv',
                    'videobalance',
                    'videoflip',
                    'warptv',
                    'ximagesrc']:
        if not element in availableElements:
            libAvailable = False

            break

    features['gst-good-plugins'] = [libAvailable, 'GStreamer Good Plugins', self.tr('Basic sources and effects.')]

    libAvailable = True

    # GStreamer Bad Plugins:
    for element in ['bulge',
                    'burn',
                    'chromium',
                    'coloreffects',
                    'exclusion',
                    'fisheye',
                    'kaleidoscope',
                    'marble',
                    'mirror',
                    'pinch',
                    'solarize',
                    'sphere',
                    'sphere',
                    'stretch',
                    'tunnel',
                    'twirl',
                    'vp8enc',
                    'waterripple']:
        if not element in availableElements:
            libAvailable = False

            break

    features['gst-bad-plugins'] = [libAvailable, 'GStreamer Bad Plugins', self.tr('Effects and some codecs.')]

    libAvailable = True

    # frei0r Plugins:
    for element in ['frei0r-filter-cartoon',
                    'frei0r-filter-delaygrab',
                    'frei0r-filter-distort0r',
                    'frei0r-filter-equaliz0r',
                    'frei0r-filter-hqdn3d',
                    'frei0r-filter-invert0r',
                    'frei0r-filter-nervous',
                    'frei0r-filter-pixeliz0r',
                    'frei0r-filter-primaries',
                    'frei0r-filter-sobel',
                    'frei0r-filter-sop-sat',
                    'frei0r-filter-threelay0r',
                    'frei0r-filter-twolay0r']:
        if not element in availableElements:
            libAvailable = False

            break

    features['frei0r-plugins'] = [libAvailable, 'frei0r Plugins', self.tr('Extra effects.')]

    libAvailable = True

    # GStreamer FFmpeg Plugins:
    for element in ['ffdec_bmp',
                    'ffenc_bmp']:
        if not element in availableElements:
            libAvailable = False

            break

    features['gst-ffmpeg'] = [libAvailable, 'GStreamer FFmpeg Plugins', self.tr('Basic functionality.')]

    return features
}

QMap<QString, QString> V4L2Tools::availableEffects()
{
    features = self.featuresMatrix()
    effects = {}

    if features['gst-good-plugins'][0]:
        effects.update({'agingtv': self.tr('Old'),
                        'dicetv': self.tr('Dices'),
                        'edgetv': self.tr('Edges'),
                        'optv': self.tr('Hypnotic'),
                        'quarktv': self.tr('Quark'),
                        'radioactv': self.tr('Radioactive'),
                        'revtv': self.tr('Scan Lines'),
                        'rippletv': self.tr('Ripple'),
                        'shagadelictv': self.tr('Psychedelic'),
                        'streaktv': self.tr('Streak'),
                        'vertigotv': self.tr('Vertigo'),
                        'videobalance saturation=1.5 hue=-0.5': self.tr('Hulk'),
                        'videobalance saturation=1.5 hue=+0.5': self.tr('Mauve'),
                        'videobalance saturation=0': self.tr('Noir'),
                        'videobalance saturation=2': self.tr('Saturation'),
                        'videoflip method=clockwise': self.tr('Rotate Right'),
                        'videoflip method=rotate-180': self.tr('Rotate 180'),
                        'videoflip method=counterclockwise': self.tr('Rotate Left'),
                        'videoflip method=horizontal-flip': self.tr('Flip horizontally'),
                        'videoflip method=vertical-flip': self.tr('Flip vertically'),
                        'videoflip method=upper-left-diagonal': self.tr('Flip Top Left'),
                        'videoflip method=upper-right-diagonal': self.tr('Flip Top Right'),
                        'warptv': self.tr('Warp')})

    if features['gst-bad-plugins'][0]:
        effects.update({'bulge': self.tr('Bulge'),
                        'burn': self.tr('Burn'),
                        'chromium': self.tr('Chromium'),
                        'coloreffects preset=heat': self.tr('Heat'),
                        'coloreffects preset=sepia': self.tr('Sepia'),
                        'coloreffects preset=xray': self.tr('X-Ray'),
                        'coloreffects preset=xpro': self.tr('X-Pro'),
                        'exclusion': self.tr('Exclusion'),
                        'fisheye': self.tr('Fish Eye'),
                        'kaleidoscope': self.tr('Kaleidoscope'),
                        'marble': self.tr('Marble'),
                        'mirror': self.tr('Mirror'),
                        'pinch': self.tr('Pinch'),
                        'solarize': self.tr('Solarize'),
                        'sphere': self.tr('Sphere'),
                        'sphere': self.tr('Square'),
                        'stretch': self.tr('Stretch'),
                        'tunnel': self.tr('Tunnel'),
                        'twirl': self.tr('Twirl'),
                        'waterripple': self.tr('Water Ripple')})

    if features['frei0r-plugins'][0]:
        effects.update({'frei0r-filter-cartoon': self.tr('Cartoon'),
                        'frei0r-filter-delaygrab': self.tr('Past'),
                        'frei0r-filter-distort0r': self.tr('Distort'),
                        'frei0r-filter-equaliz0r': self.tr('Equalize'),
                        'frei0r-filter-hqdn3d spatial=0.5 temporal=1.0': self.tr('Drugs'),
                        'frei0r-filter-invert0r': self.tr('Invert'),
                        'frei0r-filter-nervous': self.tr('Nervous'),
                        'frei0r-filter-pixeliz0r': self.tr('Pixelate'),
                        'frei0r-filter-primaries': self.tr('Primary Colors'),
                        'frei0r-filter-sobel': self.tr('Sobel'),
                        'frei0r-filter-sop-sat': self.tr('Crazy Colors'),
                        'frei0r-filter-threelay0r': self.tr('The Godfather'),
                        'frei0r-filter-twolay0r': self.tr('Che Guevara')})

    return effects
}

QStringList V4L2Tools::currentEffects()
{
    return self.effects
}

QStringList V4L2Tools::bestVideoRecordFormat(QString fileName)
{
    root, ext = os.path.splitext(fileName)

    if ext == '':
        return '', '', '', ''

    ext = ext[1:].lower()

    for suffix, videoEncoder, audioEncoder, muxer in \
                                                self.videoRecordFormats:
        for s in suffix.split(','):
            if s.lower().strip() == ext:
                return suffix, videoEncoder, audioEncoder, muxer

    return '', '', '', ''
}

QVariantList V4L2Tools::queryControl(int dev_fd, v4l2_queryctrl *queryctrl)
{
    ctrl = v4l2.v4l2_control(0)
    ext_ctrl = v4l2.v4l2_ext_control(0)
    ctrls = v4l2.v4l2_ext_controls(0)

    if queryctrl.flags & v4l2.V4L2_CTRL_FLAG_DISABLED:
        return tuple()

    if queryctrl.type == v4l2.V4L2_CTRL_TYPE_CTRL_CLASS:
        return tuple()

    ext_ctrl.id = queryctrl.id
    ctrls.ctrl_class = v4l2.V4L2_CTRL_ID2CLASS(queryctrl.id)
    ctrls.count = 1
    ctrls.controls = ctypes.pointer(ext_ctrl)

    if v4l2.V4L2_CTRL_ID2CLASS(queryctrl.id) != v4l2.V4L2_CTRL_CLASS_USER \
        and queryctrl.id < v4l2.V4L2_CID_PRIVATE_BASE:
        if fcntl.ioctl(dev_fd, v4l2.VIDIOC_G_EXT_CTRLS, ctrls):
            return tuple()
    else:
        ctrl.id = queryctrl.id

        if fcntl.ioctl(dev_fd, v4l2.VIDIOC_G_CTRL, ctrl):
            return tuple()

        ext_ctrl.value = ctrl.value

    qmenu = v4l2.v4l2_querymenu(0)
    qmenu.id = queryctrl.id
    menu = []

    if queryctrl.type == v4l2.V4L2_CTRL_TYPE_MENU:
        for i in range(queryctrl.maximum + 1):
            qmenu.index = i

            if fcntl.ioctl(dev_fd, v4l2.VIDIOC_QUERYMENU, qmenu):
                continue

            menu.append(qmenu.name)

    return (queryctrl.name,
            queryctrl.type,
            queryctrl.minimum,
            queryctrl.maximum,
            queryctrl.step,
            queryctrl.default,
            ext_ctrl.value,
            menu)
}

QMap<QString, uint> V4L2Tools::findControls(int dev_fd)
{
    qctrl = v4l2.v4l2_queryctrl(v4l2.V4L2_CTRL_FLAG_NEXT_CTRL)
    controls = {}

    try:
        while fcntl.ioctl(dev_fd, v4l2.VIDIOC_QUERYCTRL, qctrl) == 0:
            if qctrl.type != v4l2.V4L2_CTRL_TYPE_CTRL_CLASS and \
               not (qctrl.flags & v4l2.V4L2_CTRL_FLAG_DISABLED):
                controls[qctrl.name] = qctrl.id

            qctrl.id |= v4l2.V4L2_CTRL_FLAG_NEXT_CTRL
    except:
        pass

    if qctrl.id != v4l2.V4L2_CTRL_FLAG_NEXT_CTRL:
        return controls

    for id in range(v4l2.V4L2_CID_USER_BASE, v4l2.V4L2_CID_LASTP1):
        qctrl.id = id

        if fcntl.ioctl(dev_fd, v4l2.v4l2.VIDIOC_QUERYCTRL, qctrl) == 0 \
           and not (qctrl.flags & v4l2.V4L2_CTRL_FLAG_DISABLED):
            controls[qctrl.name] = qctrl.id

    qctrl.id = v4l2.V4L2_CID_PRIVATE_BASE

    while fcntl.ioctl(dev_fd, v4l2.VIDIOC_QUERYCTRL, qctrl) == 0:
        if not (qctrl.flags & v4l2.V4L2_CTRL_FLAG_DISABLED):
            controls[qctrl.name] = qctrl.id

        qctrl.id += 1

    return controls
}

QString V4L2Tools::hashFromName(QString name)
{
    return 'x{0}'.format(name.encode('hex'))
}

QString V4L2Tools::nameFromHash(QString hash)
{
    return hash[1:].decode('hex')
}

StreamType V4L2Tools::deviceType(QString dev_name)
{
    if dev_name in [device[0] for device in self.webcams]:
        return self.StreamTypeWebcam
    elif dev_name in [device[0] for device in self.streams]:
        return self.StreamTypeURI
    elif dev_name == 'desktop':
        return self.StreamTypeDesktop
    else:
        return self.StreamTypeUnknown
}

void V4L2Tools::loadConfigs()
{
    config = kdecore.KSharedConfig.openConfig(self.appEnvironment.configFileName())

    webcamConfigs = config.group('GeneralConfigs')

    self.enableAudioRecording(webcamConfigs.readEntry('recordAudio', True).toBool())

    effectsConfigs = config.group('Effects')

    effcts = effectsConfigs.readEntry('effects', '').toString().toUtf8().data()

    if effcts != '':
        self.setEffects(effcts.split('&&'))

    videoFormatsConfigs = config.group('VideoRecordFormats')

    videoRecordFormats = videoFormatsConfigs. \
                readEntry('formats',
                          'webm::'
                          'vp8enc quality=10 speed=7 bitrate=1000000000::'
                          'vorbisenc::'
                          'webmmux&&'
                          'ogv, ogg::'
                          'theoraenc quality=63 bitrate=16777215::'
                          'vorbisenc::'
                          'oggmux').toString().toUtf8().data()

    if videoRecordFormats != '':
        for fmt in videoRecordFormats.split('&&'):
            params = fmt.split('::')

            self.setVideoRecordFormat(params[0],
                                      params[1],
                                      params[2],
                                      params[3])

    streamsConfig = config.group('CustomStreams')
    streams = streamsConfig.readEntry('streams', '').toString().toUtf8().data()

    if streams != '':
        for fmt in streams.split('&&'):
            params = fmt.split('::')

            self.setCustomStream(params[0], params[1])
}

void V4L2Tools::saveConfigs()
{
    config = kdecore.KSharedConfig.openConfig(self.appEnvironment.configFileName())

    webcamConfigs = config.group('GeneralConfigs')

    webcamConfigs.writeEntry('recordAudio', self.recordAudio)

    effectsConfigs = config.group('Effects')

    effectsConfigs.writeEntry('effects', '&&'.join(self.effects))

    videoFormatsConfigs = config.group('VideoRecordFormats')

    videoRecordFormats = []

    for suffix, videoEncoder, audioEncoder, muxer in self.\
                                                        videoRecordFormats:
        videoRecordFormats.append('{0}::{1}::{2}::{3}'.format(suffix,
                                                              videoEncoder,
                                                              audioEncoder,
                                                              muxer))

    videoFormatsConfigs.writeEntry('formats',
                                   '&&'.join(videoRecordFormats))

    streamsConfigs = config.group('CustomStreams')

    streams = []

    for dev_name, description, streamType in self.streams:
        streams.append('{0}::{1}'.format(dev_name, description))

    streamsConfigs.writeEntry('streams', '&&'.join(streams))

    config.sync()
}

void V4L2Tools::setEffects(QStringList effects)
{
    effect = [str(effect) for effect in effects]

    if self.effects == effects:
        return

    self.effects = effects
    state, pending, timeout = self.mainPipeline.get_state()

    if pending == gst.STATE_PLAYING:
        self.mainPipeline.set_state(gst.STATE_PAUSED)

    captureTee = self.mainBin.get_by_name('CaptureTee')
    effectsTee = self.mainBin.get_by_name('EffectsTee')

    if self.effectsBin:
        effectsBinIn = self.effectsBin.get_by_name('EffectsBinIn')
        effectsBinOut = self.effectsBin.get_by_name('EffectsBinOut')

        captureTee.unlink(effectsBinIn)
        effectsBinOut.unlink(effectsTee)
        self.mainBin.remove(self.effectsBin)
        captureTee.link(effectsTee)

        self.effectsBin = None

    if self.effects != []:
        pipeline = 'queue name=EffectsBinIn'

        for effect in self.effects:
            pipeline += ' ! ffmpegcolorspace ! {0}'.format(effect)

        pipeline += ' ! identity name=EffectsBinOut'

        self.effectsBin = gst.gst_parse_bin_from_description(pipeline, False)

        effectsBinIn = self.effectsBin.get_by_name('EffectsBinIn')
        effectsBinOut = self.effectsBin.get_by_name('EffectsBinOut')

        captureTee.unlink(effectsTee)
        self.mainBin.add(self.effectsBin)
        captureTee.link(effectsBinIn)
        effectsBinOut.link(effectsTee)

    if pending == gst.STATE_PLAYING:
        self.mainPipeline.set_state(gst.STATE_PLAYING)
}

void V4L2Tools::startEffectsPreview()
{
    state, pending, timeout = self.mainPipeline.get_state()

    if pending == gst.STATE_PLAYING:
        self.mainPipeline.set_state(gst.STATE_PAUSED)

    if not self.effectsPreviewBin:
        pipeline = 'queue name=EffectsPreview ! valve name=EffectsPreviewValve drop=true ! ffmpegcolorspace ! ' \
                   'videoscale ! video/x-raw-rgb,width={0},height={1} ! tee name=preview'.format(128, 96)

        for effect in self.availableEffects().keys():
            previewHash = self.hashFromName(effect)

            pipeline += ' preview. ! queue ! ffmpegcolorspace ! {0} ! ' \
                        'ffmpegcolorspace ! ffenc_bmp ! ' \
                        'appsink name={1} emit-signals=true'.format(effect, previewHash)

        self.effectsPreviewBin = gst.gst_parse_bin_from_description(pipeline, False)
        self.mainBin.add(self.effectsPreviewBin)
        queueEffectsPreview = self.effectsPreviewBin.get_by_name('EffectsPreview')
        captureTee = self.mainBin.get_by_name('CaptureTee')
        captureTee.link(queueEffectsPreview)

        for effect in self.availableEffects().keys():
            previewHash = self.hashFromName(effect)
            preview = self.mainBin.get_by_name(previewHash)
            preview.connect('new-buffer', self.readFrame)

    effectsPreviewValve = self.effectsPreviewBin.get_by_name('EffectsPreviewValve')
    effectsPreviewValve.set_property('drop', False)

    if pending == gst.STATE_PLAYING:
        self.mainPipeline.set_state(gst.STATE_PLAYING)
}

void V4L2Tools::stopEffectsPreview()
{
    if self.effectsPreviewBin:
        state, pending, timeout = self.mainPipeline.get_state()

        if pending == gst.STATE_PLAYING:
            self.mainPipeline.set_state(gst.STATE_PAUSED)

        effectsPreviewValve = self.effectsPreviewBin.get_by_name('EffectsPreviewValve')
        effectsPreviewValve.set_property('drop', True)

        if pending == gst.STATE_PLAYING:
            self.mainPipeline.set_state(gst.STATE_PLAYING)
}

void V4L2Tools::startDevice(QString dev_name, QVariantList forcedFormat)
{
    self.stopCurrentDevice()
    self.captureBuffer = {}
    self.previewsBuffer = {}
    deviceType = self.deviceType(dev_name)

    if deviceType == self.StreamTypeWebcam:
        if forcedFormat == tuple():
            fmt = self.currentVideoFormat(dev_name)

            if fmt == tuple():
                fmt = self.videoFormats(dev_name)[0]
        else:
            fmt = forcedFormat

        self.captureDevice = gst.gst_parse_bin_from_description('v4l2src device={0} ! capsfilter name=capture caps=video/x-raw-yuv,width={1},height={2}'.format(dev_name, fmt[0], fmt[1]), False)
    elif deviceType == self.StreamTypeURI:
        self.captureDevice = gst.gst_parse_bin_from_description('uridecodebin name=capture', False)
        capture = self.captureDevice.get_by_name('capture')
        capture.set_property('uri', dev_name)
    elif deviceType == self.StreamTypeDesktop:
        self.captureDevice = gst.gst_parse_bin_from_description('ximagesrc name=capture show-pointer=true', False)
    else:
        return

    self.mainBin.add(self.captureDevice)
    capture = self.captureDevice.get_by_name('capture')
    captureTee = self.mainBin.get_by_name('CaptureTee')
    capture.link(captureTee)

    self.mainPipeline.set_state(gst.STATE_PLAYING)
    self.curDevName = dev_name
    self.playing = True
    self.playingStateChanged.emit(True)

    #self.gstError.emit()
}

void V4L2Tools::stopCurrentDevice()
{
    if not self.playing:
        return

    self.mainPipeline.set_state(gst.STATE_NULL)

    capture = self.captureDevice.get_by_name('capture')
    captureTee = self.mainBin.get_by_name('CaptureTee')
    capture.unlink(captureTee)
    self.mainBin.remove(self.captureDevice)
    self.captureDevice = None

    self.curDevName = ''
    self.playing = False
    self.playingStateChanged.emit(False)
}

void V4L2Tools::startVideoRecord(QString fileName)
{
    self.stopVideoRecord()

    if not self.playing:
        return

    suffix, videoEncoder, audioEncoder, muxer = self.bestVideoRecordFormat(fileName)

    if suffix == '':
        return

    pipeline = 'appsrc name={capture} emit-signals=true do-timestamp=true ! ffdec_bmp ! ' + \
               'ffmpegcolorspace ! {0} ! queue ! muxer. '.format(videoEncoder)

    if self.recordAudio:
        # autoaudiosrc
        pipeline += 'alsasrc device=plughw:0,0 ! queue ! audioconvert ! ' \
                    'queue ! {0} ! queue ! muxer. '.format(audioEncoder)

    pipeline += '{0} name=muxer ! filesink location="{1}"'.format(muxer, fileName)

    self.pipeRecordVideo.setPipeline(pipeline)
    self.pipeRecordVideo.start()
    self.recordingStateChanged.emit(True)
    self.recording = True
}

void V4L2Tools::stopVideoRecord()
{
    if self.recording:
        dev_name = self.curDevName
        self.pipeRecordVideo.stop()
        self.recording = False
        self.recordingStateChanged.emit(False)
}

void V4L2Tools::clearVideoRecordFormats()
{
    self.videoRecordFormats = []
}

void V4L2Tools::clearCustomStreams()
{
    self.streams = []
    self.devicesModified.emit()
}

void V4L2Tools::setCustomStream(QString dev_name, QString description)
{
    self.streams.append((dev_name, description, self.StreamTypeURI))
    self.devicesModified.emit()
}

void V4L2Tools::enableAudioRecording(bool enable)
{
    self.recordAudio = enable
}

void V4L2Tools::setVideoRecordFormat(QString suffix, QString videoEncoder,
                                     QString audioEncoder, QString muxer)
{
    self.videoRecordFormats.append((suffix, videoEncoder,
                                            audioEncoder, muxer))
}

void V4L2Tools::aboutToQuit()
{
    self.stopCurrentDevice()
    self.saveConfigs()
}

void V4L2Tools::busMessage(const QGst::MessagePtr &message)
{
    if message.type == gst.MESSAGE_EOS:
        self.mainPipeline.set_state(gst.STATE_NULL)
    elif message.type == gst.MESSAGE_ERROR:
        self.mainPipeline.set_state(gst.STATE_NULL)

        err, debug = message.parse_error()
        print('Error: {0} {1}'.format(err, debug))
}

void V4L2Tools::reset(QString dev_name)
{
    videoFormats = self.videoFormats(dev_name)
    self.setVideoFormat(dev_name, videoFormats[0])

    controls = self.listControls(dev_name)
    ctrls = {}

    for control in controls:
        ctrls[control[0]] = control[5]

    self.setControls(dev_name, ctrls)
}

void V4L2Tools::readFrame(QGst::ElementPtr sink)
{
    self.mutex.lock()
    pipename = self.nameFromHash(sink.get_name())
    frame = QtGui.QImage.fromData(sink.emit('pull-buffer'))

    if pipename == 'capture':
        self.frameReady.emit(frame)
    else:
        self.previewFrameReady.emit(frame, pipename)

    self.mutex.unlock()
}
