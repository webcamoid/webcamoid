#!/usr/bin/env python2
# -*- coding: utf-8 -*-
#
# Webcamod, Show and take Photos with your webcam.
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

import sys

from PyQt4 import uic, QtGui, QtCore
from PyKDE4.kdecore import KStandardDirs
from PyKDE4.kdeui import KIcon, KNotification

import v4l2tools
import translator
import infotools


class WebcamoidGui(QtGui.QWidget):
    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self)

        try:
            uic.loadUi('../ui/webcamoidgui.ui', self)
        except:
            uic.loadUi(parent.package().filePath('ui', 'webcamoidgui.ui'), self)
            self.setStyleSheet('background-color: rgba(0, 0, 0, 0);')

        self.translator = translator.Translator('self.translator', parent)

        self.tools = v4l2tools.V4L2Tools(self, True)
        self.tools.devicesModified.connect(self.updateWebcams)
        self.tools.playingStateChanged.connect(self.playingStateChanged)
        self.tools.recordingStateChanged.connect(self.recordingStateChanged)
        self.tools.gstError.connect(self.showGstError)
        self.tools.frameReady.connect(self.showFrame)

        self.btnTakePhoto.setIcon(KIcon('camera-photo'))
        self.btnStartStop.setIcon(KIcon('media-playback-start'))
        self.btnVideoRecord.setIcon(KIcon('video-x-generic'))

        self.updateColors()
        self.wdgControls.hide()

        for webcam in self.tools.captureDevices():
            self.cbxSetWebcam.addItem(webcam[1])

        self.webcamFrame = QtGui.QImage()

        self.infoTools = infotools.InfoTools(self)

    def changeEvent(self, event):
        QtGui.QWidget.changeEvent(self, event)

        if event == QtCore.QEvent.StyleChange:
            self.updateColors()

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

    def updateColors(self):
        # ~/.kde4/share/config/kdeglobals
        kdeglobals = KStandardDirs.locate('config', 'kdeglobals')

        settings = QtCore.QSettings(kdeglobals,
                                    QtCore.QSettings.IniFormat,
                                    self)

        r, g, b = settings.value('Colors:Button/ForegroundNormal').toList()

        c = 'color: rgb({0}, {1}, {2});'.format(r.toInt()[0],
                                                g.toInt()[0],
                                                b.toInt()[0])

        r, g, b = settings.value('Colors:Button/BackgroundNormal').toList()

        bc = 'background-color: rgb({0}, {1}, {2});'.format(r.toInt()[0],
                                                            g.toInt()[0],
                                                            b.toInt()[0])

        r, g, b = settings.value('Colors:Button/BackgroundAlternate').toList()

        abc = 'alternate-background-color: rgb({0}, {1}, {2});'.\
                                                            format(r.toInt()[0],
                                                                   g.toInt()[0],
                                                                   b.toInt()[0])

        styleSheet = '{0}{1}{2}'.format(c, bc, abc)

        self.btnTakePhoto.setStyleSheet(styleSheet)
        self.btnVideoRecord.setStyleSheet(styleSheet)
        self.cbxSetWebcam.setStyleSheet(styleSheet)
        self.btnStartStop.setStyleSheet(styleSheet)

    def v4l2Tools(self):
        return self.tools

    def effects(self):
        return self.tools.currentEffects()

    def processExecutable(self):
        return self.tools.processExecutable()

    def supportedVideoRecordFormats(self):
        return self.tools.supportedVideoRecordFormats()

    @QtCore.pyqtSlot(str, str, str, str)
    def setVideoRecordFormat(self,
                             suffix='',
                             videoEncoder='',
                             audioEncoder='',
                             muxer=''):
        return self.tools.setVideoRecordFormat(suffix,
                                               videoEncoder,
                                               audioEncoder,
                                               muxer)

    @QtCore.pyqtSlot(str)
    def setProcessExecutable(self, processExecutable):
        self.tools.setProcessExecutable(processExecutable)

    @QtCore.pyqtSlot(list)
    def setEffects(self, effects):
        return self.tools.setEffects(effects)

    @QtCore.pyqtSlot(QtGui.QImage)
    def showFrame(self, webcamFrame):
        self.lblFrame.setPixmap(QtGui.QPixmap.fromImage(webcamFrame))

    @QtCore.pyqtSlot()
    def updateWebcams(self):
        oldDevice = self.tools.currentDevice()
        timer_isActive = self.tools.isPlaying()
        self.tools.stopCurrentDevice()
        self.cbxSetWebcam.clear()
        webcams = self.tools.captureDevices()
        dev_names = [device[0] for device in webcams]

        for webcam in webcams:
            self.cbxSetWebcam.addItem(webcam[1])

        if oldDevice in dev_names and timer_isActive:
            self.tools.startDevice(oldDevice)

    @QtCore.pyqtSlot(bool)
    def playingStateChanged(self, playing):
        if playing:
            self.btnTakePhoto.setEnabled(True)
            self.btnVideoRecord.setEnabled(True)
            self.btnStartStop.setIcon(KIcon('media-playback-stop'))
        else:
            self.btnTakePhoto.setEnabled(False)
            self.btnVideoRecord.setEnabled(False)
            self.btnStartStop.setIcon(KIcon('media-playback-start'))
            self.webcamFrame = QtGui.QImage()
            self.lblFrame.setPixmap(QtGui.QPixmap.fromImage(self.webcamFrame))

    @QtCore.pyqtSlot(bool)
    def recordingStateChanged(self, recording):
        if recording:
            self.btnVideoRecord.setIcon(KIcon('media-playback-stop'))
        else:
            self.btnVideoRecord.setIcon(KIcon('video-x-generic'))

    @QtCore.pyqtSlot()
    def on_btnTakePhoto_clicked(self):
        image = QtGui.QImage(self.webcamFrame)
        filename = self.saveFile()

        if filename != '':
            image.save(filename)

    @QtCore.pyqtSlot()
    def on_btnVideoRecord_clicked(self):
        if self.tools.isRecording():
            self.tools.stopVideoRecord()
        else:
            self.tools.startVideoRecord(self.saveFile(True))

    @QtCore.pyqtSlot(int)
    def on_cbxSetWebcam_currentIndexChanged(self, index):
        if self.tools.isPlaying():
            self.tools.startDevice(self.tools.captureDevices()[index][0])

    @QtCore.pyqtSlot()
    def on_btnStartStop_clicked(self):
        if self.tools.isPlaying():
            self.tools.stopCurrentDevice()
        else:
            self.tools.startDevice(self.tools.\
                        captureDevices()[self.cbxSetWebcam.currentIndex()][0])

    @QtCore.pyqtSlot()
    def showGstError(self):
        cmd, isCmd = self.infoTools.gstInstallCommand()

        if isCmd:
            msg = self.translator.tr('Please install GStreamer:\n\n')
        else:
            msg = self.translator.tr('Please install the following '\
                                     'packages:\n\n')

        KNotification.event(
                    KNotification.Error,
                    self.translator.tr('GStreamer not installed or configured'),
                    msg + cmd,
                    QtGui.QPixmap(),
                    None,
                    KNotification.Persistent)

    def saveFile(self, video=False):
        if video:
            videoRecordFormats = self.tools.supportedVideoRecordFormats()

            filters = []
            fst = True
            defaultSuffix = ''

            for suffix, videoEncoder, audioEncoder, muxer in videoRecordFormats:
                for s in suffix.split(','):
                    s = s.strip()
                    filters.append('{0} file (*.{1})'. \
                                   format(s.upper(), s.lower()))

                    if fst:
                        defaultSuffix = s.lower()
                        fst = False

            filters = ';;'.join(filters)
            defaultFileName = './video.{0}'.format(defaultSuffix)
        else:
            filters = 'PNG file (*.png);;'\
                      'JPEG file (*.jpg);;'\
                      'BMP file (*.bmp);;'\
                      'GIF file (*.gif)'

            defaultSuffix = 'png'
            defaultFileName = './image.png'

        if defaultSuffix == '':
            return ''

        saveFileDialog = QtGui.QFileDialog(None,
                                           self.translator.\
                                                tr('Save File As...'),
                                           defaultFileName,
                                           filters)

        saveFileDialog.setModal(True)
        saveFileDialog.setDefaultSuffix(defaultSuffix)
        saveFileDialog.setFileMode(QtGui.QFileDialog.AnyFile)
        saveFileDialog.setAcceptMode(QtGui.QFileDialog.AcceptSave)
        saveFileDialog.exec_()

        selected_files = saveFileDialog.selectedFiles()

        return '' if selected_files.isEmpty() else str(selected_files[0])


if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    webcamoidGui = WebcamoidGui()
    webcamoidGui.show()
    app.exec_()
