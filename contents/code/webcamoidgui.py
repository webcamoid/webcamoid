#!/usr/bin/env python2
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

import sys

from PyQt4 import uic, QtGui, QtCore
from PyKDE4.kdeui import KIcon, KNotification

import v4l2tools
import translator


class WebcamoidGui(QtGui.QWidget):
    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self)

        try:
            uic.loadUi('../ui/webcamoidgui.ui', self)
        except:
            uic.loadUi(parent.package().filePath('ui', 'webcamoidgui.ui'), self)

        self.translator = translator.Translator('self.translator', self)

        self.tools = v4l2tools.V4L2Tools(self, True)
        self.tools.devicesModified.connect(self.updateWebcams)

        self.btnTakePhoto.setIcon(KIcon('camera-photo'))
        self.btnStartStop.setIcon(KIcon('media-playback-start'))

        self.wdgControls.hide()

        self.timer = QtCore.QTimer(self)
        self.timer.setInterval(1)
        self.timer.timeout.connect(self.showFrame)

        for webcam in self.tools.captureDevices():
            self.cbxSetWebcam.addItem(webcam[1])

        self.webcamFrame = QtGui.QImage()

    def resizeEvent(self, event):
        QtGui.QWidget.resizeEvent(self, event)
        size = event.size()
        self.lblFrame.resize(size)

        geometry = QtCore.QRect(0,
                                size.height() - self.wdgControls.height(),
                                size.width(),
                                self.wdgControls.height())

        self.wdgControls.setGeometry(geometry)

    def enterEvent(self, event):
        QtGui.QWidget.enterEvent(self, event)
        self.wdgControls.show()

    def leaveEvent(self, event):
        QtGui.QWidget.leaveEvent(self, event)

        if not self.rect().contains(self.mapFromGlobal(QtGui.QCursor.pos())):
            self.wdgControls.hide()

    def v4l2Tools(self):
        return self.tools

    def ffmpegExecutable(self):
        return self.tools.ffmpegExecutable()

    @QtCore.pyqtSlot(str)
    def setFFmpegExecutable(self, ffmpegExecutable):
        self.tools.setFFmpegExecutable(ffmpegExecutable)

    @QtCore.pyqtSlot()
    def showFrame(self):
        if self.timer.isActive():
            self.webcamFrame = self.tools.readFrame()
            self.lblFrame.setPixmap(QtGui.QPixmap.fromImage(self.webcamFrame))

    @QtCore.pyqtSlot()
    def updateWebcams(self):
        oldDevice = self.tools.currentDevice()
        timer_isActive = self.timer.isActive()
        self.tools.stopCurrentDevice()
        self.btnTakePhoto.setEnabled(False)
        self.btnStartStop.setIcon(KIcon('media-playback-start'))
        self.timer.stop()
        self.webcamFrame = QtGui.QImage()
        self.lblFrame.setPixmap(QtGui.QPixmap.fromImage(self.webcamFrame))
        self.cbxSetWebcam.clear()
        webcams = self.tools.captureDevices()
        dev_names = [device[0] for device in webcams]

        for webcam in webcams:
            self.cbxSetWebcam.addItem(webcam[1])

        if oldDevice in dev_names and timer_isActive:
            if self.tools.startDevice(oldDevice):
                self.btnTakePhoto.setEnabled(True)
                self.btnStartStop.setIcon(KIcon('media-playback-stop'))
                self.timer.start()
            else:
                self.showFFmpegError()

    @QtCore.pyqtSlot()
    def on_btnTakePhoto_clicked(self):
        image = QtGui.QImage(self.webcamFrame)
        filename = self.saveFile()

        if filename != '':
            image.save(filename)

    @QtCore.pyqtSlot(int)
    def on_cbxSetWebcam_currentIndexChanged(self, index):
        if self.timer.isActive():
            if not self.tools.startDevice(
                   self.tools.captureDevices()[index][0]):
                self.showFFmpegError()

    @QtCore.pyqtSlot()
    def on_btnStartStop_clicked(self):
        if self.timer.isActive():
            self.tools.stopCurrentDevice()
            self.btnTakePhoto.setEnabled(False)
            self.btnStartStop.setIcon(KIcon('media-playback-start'))
            self.timer.stop()
            self.webcamFrame = QtGui.QImage()
            self.lblFrame.setPixmap(QtGui.QPixmap.fromImage(self.webcamFrame))
        else:
            if self.tools.startDevice(self.tools.\
                        captureDevices()[self.cbxSetWebcam.currentIndex()][0]):
                self.btnTakePhoto.setEnabled(True)
                self.btnStartStop.setIcon(KIcon('media-playback-stop'))
                self.timer.start()
            else:
                self.showFFmpegError()

    def showFFmpegError(self):
        KNotification.event(
                    KNotification.Error,
                    self.translator.tr('FFmpeg not installed or configured'),
                    self.translator.tr('Please install FFmpeg:\n') +
                    '\n'
                    '<strong>Arch/Chakra</strong>: pacman -S ffmpeg\n'
                    '<strong>Debian/Ubuntu</strong>: apt-get install ffmpeg\n'
                    '<strong>Fedora</strong>: yum install ffmpeg\n'
                    '<strong>OpenSuSE</strong>: zypper install ffmpeg\n'
                    '<strong>Mandriva</strong>: urpmi ffmpeg\n'
                    '<strong>Pardus</strong>: pisi it ffmpeg',
                    QtGui.QPixmap(),
                    None,
                    KNotification.Persistent)

    def saveFile(self):
        filters = 'PNG file (*.png);;'\
                  'JPEG file (*.jpg);;'\
                  'BMP file (*.bmp);;'\
                  'GIF file (*.gif)'

        saveFileDialog = QtGui.QFileDialog(None,
                                           self.translator.tr('Save Image As...'),
                                           './image.png',
                                           filters)

        saveFileDialog.setModal(True)
        saveFileDialog.setDefaultSuffix('png')
        saveFileDialog.setFileMode(QtGui.QFileDialog.AnyFile)
        saveFileDialog.setAcceptMode(QtGui.QFileDialog.AcceptSave)
        saveFileDialog.exec_()

        selected_files = saveFileDialog.selectedFiles()

        return '' if selected_files.isEmpty() else selected_files[0]


if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    webcamoidGui = WebcamoidGui()
    webcamoidGui.show()
    app.exec_()
