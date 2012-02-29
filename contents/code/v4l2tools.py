#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Webcamod, Show and take Photos with your webcam.
# Copyright (C) 2011  Gonzalo Exequiel Pedone
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with This program. If not, see <http://www.gnu.org/licenses/>.
#
# Email   : hipersayan.x@gmail.com
# Web-Site: http://hipersayanx.blogspot.com/

import os
import sys
import ctypes
import fcntl
import subprocess
import tempfile
from v4l2 import v4l2
from PyQt4 import QtCore, QtGui

class V4L2Tools(QtCore.QObject):
    devicesModified = QtCore.pyqtSignal()

    def __init__(self, parent=None, watchDevices=False):
        QtCore.QObject.__init__(self, parent)
        self.ffmpeg_executable = 'ffmpeg'
        self.fps = 30
        self.process = None
        self.current_dev_name = ''
        self.videoSize = QtCore.QSize()
        self.videoPipe = QtCore.QFile()
        self.devnull = None

        if watchDevices:
            self.fsWatcher = QtCore.QFileSystemWatcher(['/dev'], self)
            self.fsWatcher.directoryChanged.connect(self.devicesModified)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.stopCurrentDevice()

        return True

    def ffmpegExecutable(self):
        return self.ffmpeg_executable

    @QtCore.pyqtSlot(str)
    def setFFmpegExecutable(self, path='ffmpeg'):
        self.ffmpeg_executable = path

    def currentDevice(self):
        return self.current_dev_name

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

        for type in [v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE, v4l2.V4L2_BUF_TYPE_VIDEO_OUTPUT, v4l2.V4L2_BUF_TYPE_VIDEO_OVERLAY]:
            fmt = v4l2.v4l2_fmtdesc()
            fmt.index = 0
            fmt.type = type

            try:
                while fcntl.ioctl(dev_fd, v4l2.VIDIOC_ENUM_FMT, fmt) >= 0:
                    frmsize = v4l2.v4l2_frmsizeenum()
                    frmsize.pixel_format = fmt.pixelformat
                    frmsize.index = 0

                    try:
                        while fcntl.ioctl(dev_fd, v4l2.VIDIOC_ENUM_FRAMESIZES, frmsize) >= 0:
                            if frmsize.type == v4l2.V4L2_FRMSIZE_TYPE_DISCRETE:
                                formats.append((frmsize.discrete.width, frmsize.discrete.height, fmt.pixelformat))

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

        videoFormat = (fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.pixelformat) if fcntl.ioctl(dev_fd, v4l2.VIDIOC_G_FMT, fmt) == 0 else tuple()
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
            fd.setFileName(devicesDir.absoluteFilePath(device));

            if fd.open(QtCore.QIODevice.ReadWrite):
                fcntl.ioctl(fd.handle(), v4l2.VIDIOC_QUERYCAP, capability);

                if capability.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE:
                    webcamsDevices.append((str(fd.fileName()), capability.card))

                fd.close()

        return webcamsDevices

    # queryControl(dev_fd, queryctrl) -> (name, type, min, max, step, default, value, menu)
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

        if (v4l2.V4L2_CTRL_ID2CLASS(queryctrl.id) != v4l2.V4L2_CTRL_CLASS_USER and queryctrl.id < v4l2.V4L2_CID_PRIVATE_BASE):
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

        return (queryctrl.name, queryctrl.type, queryctrl.minimum, queryctrl.maximum, queryctrl.step, queryctrl.default, ext_ctrl.value, menu)

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
                if qctrl.type != v4l2.V4L2_CTRL_TYPE_CTRL_CLASS and not (qctrl.flags & v4l2.V4L2_CTRL_FLAG_DISABLED):
                    controls[qctrl.name] = qctrl.id

                qctrl.id |= v4l2.V4L2_CTRL_FLAG_NEXT_CTRL
        except:
            pass

        if qctrl.id != v4l2.V4L2_CTRL_FLAG_NEXT_CTRL:
            return controls

        for id in range(v4l2.V4L2_CID_USER_BASE, v4l2.V4L2_CID_LASTP1):
            qctrl.id = id

            if fcntl.ioctl(dev_fd, v4l2.v4l2.VIDIOC_QUERYCTRL, qctrl) == 0 and not (qctrl.flags & v4l2.V4L2_CTRL_FLAG_DISABLED):
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

    def reset(self, dev_name='/dev/video0'):
        videoFormats = self.videoFormats(dev_name)
        self.setVideoFormat(dev_name, videoFormats[0])

        controls = self.listControls(dev_name)
        self.setControls(dev_name, {control[0]: control[5] for control in controls})

    def startDevice(self, dev_name='/dev/video0', forcedFormat=tuple()):
        self.stopCurrentDevice()

        if forcedFormat == tuple():
            fmt = self.currentVideoFormat(dev_name)

            if fmt == tuple():
                fmt = self.videoFormats(dev_name)[0]
        else:
            fmt = forcedFormat

        pipefile = os.path.join(tempfile.gettempdir(), os.path.basename(dev_name) + '.tmp')

        try:
            os.remove(pipefile)
        except:
            pass

        os.mkfifo(pipefile, 0644)
        self.devnull = open(os.devnull, 'w')

        try:
            self.process = subprocess.Popen([self.ffmpeg_executable,
                                            '-y',
                                            '-f', 'video4linux2',
                                            '-s', '{}x{}'.format(fmt[0], fmt[1]),
                                            '-r', str(self.fps),
                                            '-i', dev_name,
                                            '-f', 'rawvideo',
                                            '-vcodec', 'rawvideo',
                                            '-pix_fmt', 'rgb24',
                                            pipefile],
                                            stdin=subprocess.PIPE,
                                            stdout=self.devnull,
                                            stderr=self.devnull)
        except:
            os.remove(pipefile)
            self.process = None

            return False

        self.videoPipe.setFileName(pipefile)
        self.videoPipe.open(QtCore.QIODevice.ReadOnly)
        self.videoSize = QtCore.QSize(fmt[0], fmt[1])
        self.current_dev_name = dev_name

        return True

    def stopCurrentDevice(self):
        if self.process != None:
            self.process.kill()
            self.process.wait()
            self.videoPipe.close()
            pipefile = os.path.join(tempfile.gettempdir(), os.path.basename(self.current_dev_name) + '.tmp')
            os.remove(pipefile)

            if self.devnull != None:
                self.devnull.close()

            self.process = None
            self.current_dev_name = ''
            self.videoSize = QtCore.QSize()

    @QtCore.pyqtSlot()
    def readFrame(self):
        if self.process != None:
            frame = self.videoPipe.read(3 * self.videoSize.width() * self.videoSize.height())

            return QtGui.QImage(frame, self.videoSize.width(), self.videoSize.height(), QtGui.QImage.Format_RGB888)

if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    tools = V4L2Tools()
    tools.startDevice('/dev/video0')

    for i in range(100):
        tools.readFrame()
        QtCore.QCoreApplication.processEvents()

    tools.stopCurrentDevice()
    app.exec_()
