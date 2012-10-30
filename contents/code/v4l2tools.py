#!/usr/bin/python2 -B
# -*- coding: utf-8 -*-
#
# Webcamod, webcam capture plasmoid.
# Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
#
# Webcamod is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Webcamod is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
#
# Email     : hipersayan DOT x AT gmail DOT com
# Web-Site 1: http://github.com/hipersayanX/Webcamoid
# Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796

import os
import sys
import ctypes
import fcntl
import signal
import subprocess
import struct
import threading

from PyQt4 import QtCore, QtGui
from PyKDE4 import kdecore
from v4l2 import v4l2

import appenvironment


class V4L2Tools(QtCore.QObject):
    StreamTypeUnknown = 0
    StreamTypeWebcam = 1
    StreamTypeURI = 2
    StreamTypeDesktop = 3

    devicesModified = QtCore.pyqtSignal()
    playingStateChanged = QtCore.pyqtSignal(bool)
    recordingStateChanged = QtCore.pyqtSignal(bool)
    gstError = QtCore.pyqtSignal()
    frameReady = QtCore.pyqtSignal(QtGui.QImage)
    previewFrameReady = QtCore.pyqtSignal(QtGui.QImage, str)

    def __init__(self, parent=None, watchDevices=False):
        QtCore.QObject.__init__(self, parent)

        self.appEnvironment = appenvironment.AppEnvironment(self)

        QtCore.QCoreApplication.instance().aboutToQuit.connect(self.aboutToQuit)

        self.launcher = 'gst-launch-0.10'
        self.pluginsPath = '/usr/lib/gstreamer-0.10'
        self.extraPluginsPath = '/usr/lib/frei0r-1'

        self.fps = 30
        self.process = None
        self.curDevName = ''
        self.effects = []
        self.playing = False
        self.recording = False
        self.recordAudio = True
        self.effectsPreview = True #False

        self.curOutVidFmt = 'webm'
        self.fileName = ''
        self.videoRecordFormats = []
        self.webcams = []
        self.streams = []
        self.videoPipes = {}

        self.timer = QtCore.QTimer(self)
        self.timer.setInterval(int(1000 / self.fps))
        self.timer.timeout.connect(self.readFrame)

        self.lock = threading.Lock()

        if watchDevices:
            self.fsWatcher = QtCore.QFileSystemWatcher(['/dev'], self)
            self.fsWatcher.directoryChanged.connect(self.devicesModified)

        self.loadConfigs()

    def __enter__(self):
        self.loadConfigs()

        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.stopCurrentDevice()
        self.saveConfigs()

        return True

    @QtCore.pyqtSlot()
    def loadConfigs(self):
        config = kdecore.KSharedConfig.openConfig(self.appEnvironment.configFileName())

        webcamConfigs = config.group('GeneralConfigs')

        self.setLauncher(webcamConfigs.
                readEntry('launcher', 'gst-launch-0.10').toString().toUtf8().data())

        self.pluginsPath = webcamConfigs.readEntry('pluginsPath', '/usr/lib/gstreamer-0.10').toString().toUtf8().data()
        self.extraPluginsPath = webcamConfigs.readEntry('extraPluginsPath', '/usr/lib/frei0r-1').toString().toUtf8().data()
        self.setFps(webcamConfigs.readEntry('fps', 30).toInt()[0])
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

    @QtCore.pyqtSlot()
    def saveConfigs(self):
        config = kdecore.KSharedConfig.openConfig(self.appEnvironment.configFileName())

        webcamConfigs = config.group('GeneralConfigs')

        webcamConfigs.writeEntry('launcher', self.launcher)
        webcamConfigs.writeEntry('pluginsPath', self.pluginsPath)
        webcamConfigs.writeEntry('extraPluginsPath', self.extraPluginsPath)
        webcamConfigs.writeEntry('fps', self.fps)
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

    @QtCore.pyqtSlot()
    def aboutToQuit(self):
        self.stopCurrentDevice()
        self.saveConfigs()

    @QtCore.pyqtSlot(str)
    def setLauncher(self, path='gst-launch-0.10'):
        self.launcher = path

    @QtCore.pyqtSlot(str)
    def setPluginsPath(self, path='/usr/lib/gstreamer-0.10'):
        self.pluginsPath = path

    @QtCore.pyqtSlot(str)
    def setExtraPluginsPath(self, path='/usr/lib/frei0r-1'):
        self.extraPluginsPath = path

    def fcc2s(self, val=0):
        s = ''

        s += chr(val & 0xff)
        s += chr((val >> 8) & 0xff)
        s += chr((val >> 16) & 0xff)
        s += chr((val >> 24) & 0xff)

        return s

    # videoFormats(self, dev_name='/dev/video0') -> (width, height, fourcc)
    def videoFormats(self, dev_name='/dev/video0'):
        formats = []

        try:
            dev_fd = os.open(dev_name, os.O_RDWR | os.O_NONBLOCK, 0)
        except:
            return formats

        for type in [v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE,
                     v4l2.V4L2_BUF_TYPE_VIDEO_OUTPUT,
                     v4l2.V4L2_BUF_TYPE_VIDEO_OVERLAY]:
            fmt = v4l2.v4l2_fmtdesc()
            fmt.index = 0
            fmt.type = type

            try:
                while fcntl.ioctl(dev_fd, v4l2.VIDIOC_ENUM_FMT, fmt) >= 0:
                    frmsize = v4l2.v4l2_frmsizeenum()
                    frmsize.pixel_format = fmt.pixelformat
                    frmsize.index = 0

                    try:
                        while fcntl.ioctl(dev_fd,
                                          v4l2.VIDIOC_ENUM_FRAMESIZES,
                                          frmsize) >= 0:
                            if frmsize.type == v4l2.V4L2_FRMSIZE_TYPE_DISCRETE:
                                formats.append((frmsize.discrete.width,
                                                frmsize.discrete.height,
                                                fmt.pixelformat))

                            frmsize.index += 1
                    except:
                        pass

                    fmt.index += 1
            except:
                pass

        os.close(dev_fd)

        return formats

    def currentVideoFormat(self, dev_name='/dev/video0'):
        try:
            dev_fd = os.open(dev_name, os.O_RDWR | os.O_NONBLOCK, 0)
        except:
            return tuple()

        fmt = v4l2.v4l2_format()
        fmt.type = v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE

        if fcntl.ioctl(dev_fd, v4l2.VIDIOC_G_FMT, fmt) == 0:
            videoFormat = (fmt.fmt.pix.width,
                        fmt.fmt.pix.height,
                        fmt.fmt.pix.pixelformat)
        else:
            videoFormat = tuple()

        os.close(dev_fd)

        return videoFormat

    def setVideoFormat(self, dev_name='/dev/video0', videoFormat=tuple()):
        try:
            dev_fd = os.open(dev_name, os.O_RDWR | os.O_NONBLOCK, 0)
        except:
            return False

        fmt = v4l2.v4l2_format()
        fmt.type = v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE

        if fcntl.ioctl(dev_fd, v4l2.VIDIOC_G_FMT, fmt) == 0:
            fmt.fmt.pix.width = videoFormat[0]
            fmt.fmt.pix.height = videoFormat[1]
            fmt.fmt.pix.pixelformat = videoFormat[2]

            try:
                fcntl.ioctl(dev_fd, v4l2.VIDIOC_S_FMT, fmt)
            except:
                os.close(dev_fd)
                self.startDevice(dev_name, videoFormat)

                return True

        os.close(dev_fd)

        return True

    def captureDevices(self):
        webcamsDevices = []
        devicesDir = QtCore.QDir('/dev')

        devices = devicesDir.entryList(['video*'],
                                       QtCore.QDir.System |
                                       QtCore.QDir.Readable |
                                       QtCore.QDir.Writable |
                                       QtCore.QDir.NoSymLinks |
                                       QtCore.QDir.NoDotAndDotDot |
                                       QtCore.QDir.CaseSensitive,
                                       QtCore.QDir.Name)

        fd = QtCore.QFile()
        capability = v4l2.v4l2_capability()

        for device in devices:
            fd.setFileName(devicesDir.absoluteFilePath(device))

            if fd.open(QtCore.QIODevice.ReadWrite):
                fcntl.ioctl(fd.handle(), v4l2.VIDIOC_QUERYCAP, capability)

                if capability.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE:
                    webcamsDevices.append((str(fd.fileName()),
                                               capability.card,
                                               self.StreamTypeWebcam))

                fd.close()

        self.webcams = webcamsDevices

        return webcamsDevices + self.streams + [('desktop', self.tr('Desktop'), self.StreamTypeDesktop)]

    # queryControl(dev_fd, queryctrl) ->
    #                       (name, type, min, max, step, default, value, menu)
    def queryControl(self, dev_fd, queryctrl):
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

    def listControls(self, dev_name='/dev/video0'):
        queryctrl = v4l2.v4l2_queryctrl(v4l2.V4L2_CTRL_FLAG_NEXT_CTRL)
        controls = []

        try:
            dev_fd = os.open(dev_name, os.O_RDWR | os.O_NONBLOCK, 0)
        except:
            return controls

        try:
            while fcntl.ioctl(dev_fd, v4l2.VIDIOC_QUERYCTRL, queryctrl) == 0:
                control = self.queryControl(dev_fd, queryctrl)

                if control != tuple():
                    controls.append(control)

                queryctrl.id |= v4l2.V4L2_CTRL_FLAG_NEXT_CTRL
        except:
            pass

        if queryctrl.id != v4l2.V4L2_CTRL_FLAG_NEXT_CTRL:
            os.close(dev_fd)

            return controls

        for id in range(v4l2.V4L2_CID_USER_BASE, v4l2.V4L2_CID_LASTP1):
            queryctrl.id = id

            if fcntl.ioctl(dev_fd, v4l2.VIDIOC_QUERYCTRL, queryctrl) == 0:
                control = self.queryControl(dev_fd, queryctrl)

                if control != tuple():
                    controls.append(control)

        queryctrl.id = v4l2.V4L2_CID_PRIVATE_BASE

        while fcntl.ioctl(dev_fd, v4l2.VIDIOC_QUERYCTRL, queryctrl) == 0:
            control = self.queryControl(dev_fd, queryctrl)

            if control != tuple():
                controls.append(control)

            queryctrl.id += 1

        os.close(dev_fd)

        return controls

    def findControls(self, dev_fd):
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

    def setControls(self, dev_name='/dev/video0', controls={}):
        try:
            dev_fd = os.open(dev_name, os.O_RDWR | os.O_NONBLOCK, 0)
        except:
            return False

        ctrl2id = self.findControls(dev_fd)
        mpeg_ctrls = []
        user_ctrls = []

        for control in controls:
            ctrl = v4l2.v4l2_ext_control(0)
            ctrl.id = ctrl2id[control]
            ctrl.value = controls[control]

            if v4l2.V4L2_CTRL_ID2CLASS(ctrl.id) == v4l2.V4L2_CTRL_CLASS_MPEG:
                mpeg_ctrls.append(ctrl)
            else:
                user_ctrls.append(ctrl)

        for user_ctrl in user_ctrls:
            ctrl = v4l2.v4l2_control()
            ctrl.id = user_ctrl.id
            ctrl.value = user_ctrl.value
            fcntl.ioctl(dev_fd, v4l2.VIDIOC_S_CTRL, ctrl)

        if mpeg_ctrls != []:
            ctrls = v4l2.v4l2_ext_controls(0)
            ctrls.ctrl_class = v4l2.V4L2_CTRL_CLASS_MPEG
            ctrls.count = len(mpeg_ctrls)
            ctrls.controls = ctypes.pointer(mpeg_ctrls[0])
            fcntl.ioctl(dev_fd, v4l2.VIDIOC_S_EXT_CTRLS, ctrls)

        os.close(dev_fd)

        return True

    def featuresMatrix(self):
        features = {}

        libAvailable = True

        # GStreamer Core:
        for lib in ['libgstcoreelements.so']:
            if not os.path.exists(os.path.join(self.pluginsPath, lib)):
                libAvailable = False

                break

        features['gst-core'] = [libAvailable, 'GStreamer Core', self.tr('Basic functionality.')]

        libAvailable = True

        # GStreamer Base Plugins:
        for lib in ['libgstalsa.so',
                    'libgstaudioconvert.so',
                    'libgstdecodebin2.so',
                    'libgstffmpegcolorspace.so',
                    'libgsttheora.so',
                    'libgstvideoscale.so',
                    'libgstvorbis.so']:
            if not os.path.exists(os.path.join(self.pluginsPath, lib)):
                libAvailable = False

                break

        features['gst-base-plugins'] = [libAvailable, 'GStreamer Base Plugins', self.tr('Transcoding and audio source.')]

        libAvailable = True

        # GStreamer Good Plugins:
        for lib in ['libgsteffectv.so',
                    'libgstvideo4linux2.so',
                    'libgstvideofilter.so',
                    'libgstximagesrc.so']:
            if not os.path.exists(os.path.join(self.pluginsPath, lib)):
                libAvailable = False

                break

        features['gst-good-plugins'] = [libAvailable, 'GStreamer Good Plugins', self.tr('Basic sources and effects.')]

        libAvailable = True

        # GStreamer Bad Plugins:
        for lib in ['libgstcoloreffects.so',
                    'libgstfrei0r.so',
                    'libgstgaudieffects.so',
                    'libgstgeometrictransform.so',
                    'libgstvp8.so']:
            if not os.path.exists(os.path.join(self.pluginsPath, lib)):
                libAvailable = False

                break

        features['gst-bad-plugins'] = [libAvailable, 'GStreamer Bad Plugins', self.tr('Effects and some codecs.')]

        libAvailable = True

        # frei0r Plugins:
        for lib in ['cartoon.so',
                    'delaygrab.so',
                    'distort0r.so',
                    'equaliz0r.so',
                    'hqdn3d.so',
                    'invert0r.so',
                    'nervous.so',
                    'pixeliz0r.so',
                    'primaries.so',
                    'sobel.so',
                    'sopsat.so',
                    'threelay0r.so',
                    'twolay0r.so']:
            if not os.path.exists(os.path.join(self.extraPluginsPath, lib)):
                libAvailable = False

                break

        features['frei0r-plugins'] = [libAvailable, 'frei0r Plugins', self.tr('Extra effects.')]

        libAvailable = True

        # GStreamer FFmpeg Plugins:
        for lib in ['libgstffmpeg.so']:
            if not os.path.exists(os.path.join(self.pluginsPath, lib)):
                libAvailable = False

                break

        features['gst-ffmpeg'] = [libAvailable, 'GStreamer FFmpeg Plugins', self.tr('Basic functionality.')]

        return features

    def availableEffects(self):
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

    def setEffects(self, effects=[]):
        self.effects = [str(effect) for effect in effects]

        if not self.process:
            return

        self.startDevice(self.curDevName)

    def currentEffects(self):
        return self.effects

    @QtCore.pyqtSlot(int)
    def setFps(self, fps):
        self.fps = fps
        self.timer.setInterval(int(1000 / self.fps))

    def enableAudioRecording(self, enable=True):
        self.recordAudio = enable

    def deviceType(self, dev_name='/dev/video0'):
        if dev_name in [device[0] for device in self.webcams]:
            return self.StreamTypeWebcam
        elif dev_name in [device[0] for device in self.streams]:
            return self.StreamTypeURI
        elif dev_name == 'desktop':
            return self.StreamTypeDesktop
        else:
            return self.StreamTypeUnknown

    def reset(self, dev_name='/dev/video0'):
        videoFormats = self.videoFormats(dev_name)
        self.setVideoFormat(dev_name, videoFormats[0])

        controls = self.listControls(dev_name)
        ctrls = {}

        for control in controls:
            ctrls[control[0]] = control[5]

        self.setControls(dev_name, ctrls)

    def startDevice(self, dev_name='/dev/video0', forcedFormat=tuple(),
                                                  record=False):
        self.stopCurrentDevice()
        deviceType = self.deviceType(dev_name)

        params = [self.launcher, '-qe']

        if deviceType == self.StreamTypeWebcam:
            if forcedFormat == tuple():
                fmt = self.currentVideoFormat(dev_name)

                if fmt == tuple():
                    fmt = self.videoFormats(dev_name)[0]
            else:
                fmt = forcedFormat

            params += ['v4l2src', 'device={0}'.format(dev_name), '!',
                      'video/x-raw-yuv,width={0},height={1},framerate={2}/1'.
                                        format(fmt[0], fmt[1], self.fps), '!']
        elif deviceType == self.StreamTypeURI:
            params += ['uridecodebin', 'uri={0}'.format(dev_name), '!']
        elif deviceType == self.StreamTypeDesktop:
            params += ['ximagesrc', 'show-pointer=true', '!']
        else:
            return

        params += ['tee', 'name=rawvideo']
        params += ['rawvideo.', '!', 'queue', '!', 'ffmpegcolorspace', '!']

        for effect in self.effects:
            params += effect.split() + ['!', 'ffmpegcolorspace', '!']

        params += ['tee', 'name=effects']

        params += ['effects.', '!', 'queue', '!', 'ffmpegcolorspace', '!', 'ffenc_bmp', '!', 'fdsink']

        if self.effectsPreview:
            self.videoPipes = {}

            for effect in self.availableEffects().keys():
                self.videoPipes[effect] = os.pipe()

                params += ['rawvideo.', '!', 'queue', '!', 'ffmpegcolorspace', '!',
                        'videoscale', '!', 'video/x-raw-yuv,width={0},height={1}'.format(128, 96), '!', 'ffmpegcolorspace', '!'] + \
                        effect.split() + \
                        ['!', 'ffmpegcolorspace', '!', 'ffenc_bmp', '!', 'image/bmp', '!',
                        'fdsink', 'fd={0}'.format(self.videoPipes[effect][1])]

        if record:
            suffix, videoEncoder, audioEncoder, muxer = \
                                    self.bestVideoRecordFormat(self.fileName)

            if suffix == '':
                self.process = None
                self.curDevName = ''

                return

            params += ['effects.', '!', 'queue', '!', 'ffmpegcolorspace',
                       '!'] + videoEncoder.split() + ['!', 'queue', '!',
                                                      'muxer.']

            if self.recordAudio:
                params += ['alsasrc', 'device=plughw:0,0', '!', 'queue', '!',
                        'audioconvert', '!', 'queue', '!'] + \
                        audioEncoder.split() + ['!', 'queue', '!', 'muxer.']

            params += muxer.split() + ['name=muxer', '!', 'filesink',
                                       'location={0}'.format(self.fileName)]

        try:
            self.process = subprocess.Popen(params, stdout=subprocess.PIPE)
            self.curDevName = dev_name
            self.playing = True
            self.timer.start()
            self.playingStateChanged.emit(True)
        except:
            self.process = None
            self.curDevName = ''
            self.gstError.emit()

    @QtCore.pyqtSlot()
    def stopCurrentDevice(self):
        if not self.playing:
            return

        os.kill(self.process.pid, signal.SIGINT)
        self.process.wait()

        if self.effectsPreview:
            for effect in self.videoPipes:
                os.close(self.videoPipes[effect][0])
                os.close(self.videoPipes[effect][1])

        self.timer.stop()
        self.videoPipes = {}
        self.process = None
        self.curDevName = ''
        self.playing = False
        self.playingStateChanged.emit(False)

    def threadReadFrame(self):
        self.lock.acquire()

        try:
            if self.process:
                frameHeader = self.process.stdout.read(14)

                if len(frameHeader) == 14:
                    dataSize = struct.unpack('i', frameHeader[2: 6])[0] - 14
                    frameData = self.process.stdout.read(dataSize)
                    self.frameReady.emit(QtGui.QImage.fromData(frameHeader + frameData))
        except:
            pass

        if self.effectsPreview:
            effects = self.videoPipes.keys()

            for effect in effects:
                try:
                    if not effect in self.videoPipes:
                        continue

                    frameHeader = os.read(self.videoPipes[effect][0], 14)

                    if len(frameHeader) != 14:
                        continue

                    dataSize = struct.unpack('i', frameHeader[2: 6])[0] - 14
                    frameData = os.read(self.videoPipes[effect][0], dataSize)
                    self.previewFrameReady.emit(QtGui.QImage.fromData(frameHeader + frameData), effect)
                except:
                    pass

        self.lock.release()

    @QtCore.pyqtSlot()
    def readFrame(self):
        threading.Thread(target=self.threadReadFrame).start()

    @QtCore.pyqtSlot(str)
    def startVideoRecord(self, fileName=''):
        self.stopVideoRecord()
        self.fileName = fileName
        self.startDevice(self.curDevName, tuple(), True)

        if not self.process:
            return

        self.recordingStateChanged.emit(True)
        self.recording = True

    @QtCore.pyqtSlot()
    def stopVideoRecord(self):
        if self.recording:
            dev_name = self.curDevName
            self.stopCurrentDevice()
            self.recording = False
            self.fileName = ''
            self.recordingStateChanged.emit(False)
            self.startDevice(dev_name)

    @QtCore.pyqtSlot()
    def clearVideoRecordFormats(self):
        self.videoRecordFormats = []

    @QtCore.pyqtSlot(str, str, str, str)
    def setVideoRecordFormat(self, suffix='', videoEncoder='',
                                              audioEncoder='', muxer=''):
        self.videoRecordFormats.append((suffix, videoEncoder,
                                                audioEncoder, muxer))

    def bestVideoRecordFormat(self, fileName=''):
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

    @QtCore.pyqtSlot()
    def clearCustomStreams(self):
        self.streams = []
        self.devicesModified.emit()

    @QtCore.pyqtSlot(str, str)
    def setCustomStream(self, dev_name='', description=''):
        self.streams.append((dev_name, description, self.StreamTypeURI))
        self.devicesModified.emit()


if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    label = QtGui.QLabel()
    label.resize(640, 480)
    label.show()

    @QtCore.pyqtSlot(QtGui.QImage)
    def readFrame(frame):
        label.setPixmap(QtGui.QPixmap.fromImage(frame))

    tools = V4L2Tools()
    tools.startDevice('/dev/video0')
    tools.frameReady.connect(readFrame)

    app.exec_()
