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
import time

from PyQt4 import uic, QtGui, QtCore
from PyKDE4 import kdecore, kdeui, plasmascript

import effects
import infotools
import appenvironment
import v4l2tools
import videorecordconfig
import webcamconfig


class MainWindow(QtGui.QWidget):
    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self)

        self.appEnvironment = appenvironment.AppEnvironment(self)

        uic.loadUi(self.resolvePath('../ui/mainwindow.ui'), self)

        if isinstance(parent, plasmascript.Applet):
            self.setStyleSheet('QWidget#MainWindow{background-color: rgba(0, 0, 0, 0);}')

        self.setWindowTitle('{0} {1}'.format(QtCore.QCoreApplication.applicationName(),
                                             QtCore.QCoreApplication.applicationVersion()))

        self.tools = v4l2tools.V4L2Tools(self, True)
        self.tools.devicesModified.connect(self.updateWebcams)
        self.tools.playingStateChanged.connect(self.playingStateChanged)
        self.tools.recordingStateChanged.connect(self.recordingStateChanged)
        self.tools.gstError.connect(self.showGstError)
        self.tools.frameReady.connect(self.showFrame)

        self.setWindowIcon(kdeui.KIcon('camera-web'))
        self.btnTakePhoto.setIcon(kdeui.KIcon('camera-photo'))
        self.btnStartStop.setIcon(kdeui.KIcon('media-playback-start'))
        self.btnVideoRecord.setIcon(kdeui.KIcon('video-x-generic'))
        self.btnConfigure.setIcon(kdeui.KIcon('configure'))
        self.btnAbout.setIcon(kdeui.KIcon('help-about'))

        self.wdgControls.hide()

        for webcam in self.tools.captureDevices():
            self.cbxSetWebcam.addItem(webcam[1])

        self.webcamFrame = QtGui.QImage()

        self.infoTools = infotools.InfoTools(self)

        config = kdecore.KSharedConfig.openConfig('{0}rc'.format(QtCore.QCoreApplication.applicationName().toLower()))

        webcamConfigs = config.group('Webcam')

        self.tools.setProcessExecutable(str(webcamConfigs.
                readEntry('processExecutable', 'gst-launch-0.10').toString()))

        effectsConfigs = config.group('Effects')

        effcts = str(effectsConfigs.readEntry('effects', '').toString())

        if effcts != '':
            self.tools.setEffects(effcts.split('&'))

        videoFormatsConfigs = config.group('VideoRecordFormats')

        videoRecordFormats = str(videoFormatsConfigs.
                    readEntry('formats',
                              'webm::'
                              'vp8enc quality=10 speed=7 bitrate=1000000000::'
                              'vorbisenc::'
                              'webmmux&&'
                              'ogv, ogg::'
                              'theoraenc quality=63 bitrate=16777215::'
                              'vorbisenc::'
                              'oggmux').toString())

        if videoRecordFormats != '':
            for fmt in videoRecordFormats.split('&&'):
                params = fmt.split('::')
                self.tools.setVideoRecordFormat(params[0],
                                                params[1],
                                                params[2],
                                                params[3])

    def resolvePath(self, relpath=''):
        return os.path.normpath(os.path.join(os.path.
                                dirname(os.path.realpath(__file__)), relpath))

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
            self.btnStartStop.setIcon(kdeui.KIcon('media-playback-stop'))
        else:
            self.btnTakePhoto.setEnabled(False)
            self.btnVideoRecord.setEnabled(False)
            self.btnStartStop.setIcon(kdeui.KIcon('media-playback-start'))
            self.webcamFrame = QtGui.QImage()
            self.lblFrame.setPixmap(QtGui.QPixmap.fromImage(self.webcamFrame))

    @QtCore.pyqtSlot(bool)
    def recordingStateChanged(self, recording):
        if recording:
            self.btnVideoRecord.setIcon(kdeui.KIcon('media-playback-stop'))
        else:
            self.btnVideoRecord.setIcon(kdeui.KIcon('video-x-generic'))

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
            self.tools.startDevice(self.tools.
                        captureDevices()[self.cbxSetWebcam.currentIndex()][0])

    def addWebcamConfigDialog(self, configDialog):
        self.cfgWebcamDialog = webcamconfig.WebcamConfig(self, self.tools)

        configDialog.addPage(self.cfgWebcamDialog,
                             self.tr('Webcam Settings'),
                             'camera-web',
                             self.tr('Set webcam properties'),
                             False)

    def addEffectsConfigDialog(self, configDialog):
        self.cfgEffects = effects.Effects(self, self.tools)

        configDialog.\
               addPage(self.cfgEffects,
                       self.tr('Configure Webcam Effects'),
                       'tools-wizard',
                       self.tr('Add funny effects to the webcam'),
                       False)

    def addVideoFormatsConfigDialog(self, configDialog):
        self.cfgVideoFormats = videorecordconfig.\
                                            VideoRecordConfig(self, self.tools)

        configDialog.\
               addPage(self.cfgVideoFormats,
                       self.tr('Configure Video Recording Formats'),
                       'video-x-generic',
                       self.tr('Add or remove video formats for recording.'),
                       False)

    @QtCore.pyqtSlot()
    def on_btnConfigure_clicked(self):
        config = kdeui.KConfigSkeleton('', self)

        configDialog = kdeui.KConfigDialog(self,
                                           self.tr(b'{0} Settings').toUtf8().data().format(QtCore.QCoreApplication.applicationName()),
                                           config)

        configDialog.setWindowTitle(self.tr('{0} Settings').toUtf8().data().format(QtCore.QCoreApplication.applicationName()))

        self.addWebcamConfigDialog(configDialog)
        self.addEffectsConfigDialog(configDialog)
        self.addVideoFormatsConfigDialog(configDialog)

        configDialog.okClicked.connect(self.saveConfigs)
        configDialog.cancelClicked.connect(self.saveConfigs)

        configDialog.exec_()

    @QtCore.pyqtSlot()
    def saveConfigs(self):
        config = kdecore.KSharedConfig.openConfig('{0}rc'.format(QtCore.QCoreApplication.applicationName().toLower()))

        webcamConfigs = config.group('Webcam')

        webcamConfigs.writeEntry('processExecutable',
                                 self.tools.processExecutable())

        effectsConfigs = config.group('Effects')

        effectsConfigs.writeEntry('effects',
                                  '&&'.join(self.tools.currentEffects()))

        videoFormatsConfigs = config.group('VideoRecordFormats')

        videoRecordFormats = []

        for suffix, videoEncoder, audioEncoder, muxer in self.tools.\
                                                supportedVideoRecordFormats():
            videoRecordFormats.append('{0}::{1}::{2}::{3}'.format(suffix,
                                                                  videoEncoder,
                                                                  audioEncoder,
                                                                  muxer))

        videoFormatsConfigs.writeEntry('formats',
                                       '&&'.join(videoRecordFormats))

        config.sync()

    @QtCore.pyqtSlot()
    def on_btnAbout_clicked(self):
        aboutData = kdecore.\
            KAboutData(str(QtCore.QCoreApplication.applicationName()),
                       str(QtCore.QCoreApplication.applicationName()),
                       kdecore.ki18n(QtCore.QCoreApplication.applicationName()),
                       str(QtCore.QCoreApplication.applicationVersion()),
                       kdecore.ki18n(self.tr('webcam capture plasmoid.')),
                       kdecore.KAboutData.License_GPL_V3,
                       kdecore.ki18n(self.tr('Copyright (C) 2011-2012  Gonzalo Exequiel '
                              'Pedone')),
                       kdecore.ki18n(self.tr('A simple webcam plasmoid and stand alone app '
                              'for picture and video capture.')),
                       'http://github.com/hipersayanX/Webcamoid',
                       'submit@bugs.kde.org')

        aboutData.setProgramIconName('camera-web')

        aboutDialog = kdeui.KAboutApplicationDialog(aboutData, self)
        aboutDialog.exec_()

    @QtCore.pyqtSlot()
    def showGstError(self):
        cmd, isCmd = self.infoTools.gstInstallCommand()

        if isCmd:
            msg = self.tr('Please install GStreamer:\n\n')
        else:
            msg = self.tr('Please install the following packages:\n\n')

        kdeui.KNotification.event(
                    kdeui.KNotification.Error,
                    self.tr('GStreamer not installed or configured'),
                    msg + cmd,
                    QtGui.QPixmap(),
                    None,
                    kdeui.KNotification.Persistent)

    def saveFile(self, video=False):
        curTime = time.strftime('%Y-%m-%d %H-%M-%S')

        if video:
            videosPath = str(kdeui.KGlobalSettings.videosPath())

            videoRecordFormats = self.tools.supportedVideoRecordFormats()

            filters = []
            fst = True
            defaultSuffix = ''

            for suffix, videoEncoder, audioEncoder, muxer in \
                                                            videoRecordFormats:
                for s in suffix.split(','):
                    s = s.strip()
                    filters.append('{0} file (*.{1})'.
                                   format(s.upper(), s.lower()))

                    if fst:
                        defaultSuffix = s.lower()
                        fst = False

            filters = ';;'.join(filters)
            defaultFileName = os.path.join(videosPath,
                                           'Video {0}.{1}'.
                                                format(curTime, defaultSuffix))
        else:
            picturesPath = str(kdeui.KGlobalSettings.picturesPath())

            filters = 'PNG file (*.png);;'\
                      'JPEG file (*.jpg);;'\
                      'BMP file (*.bmp);;'\
                      'GIF file (*.gif)'

            defaultSuffix = 'png'
            defaultFileName = os.path.join(picturesPath,
                                           'Picture {0}.png'.format(curTime))

        if defaultSuffix == '':
            return ''

        saveFileDialog = QtGui.QFileDialog(None,
                                           self.tr('Save File As...'),
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
    mainWindow = MainWindow()
    mainWindow.show()
    app.exec_()
