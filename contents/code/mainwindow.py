#!/usr/bin/env python2
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

from PyQt4 import QtCore, QtGui, uic
from PyKDE4 import kdecore, kdeui, plasmascript

import effects
import appenvironment
import v4l2tools
import videorecordconfig
import webcamconfig
import streamsconfig
import generalconfig
import featuresinfo


class MainWindow(QtGui.QWidget):
    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self)

        self.appEnvironment = appenvironment.AppEnvironment(self)
        uic.loadUi(self.resolvePath('../ui/mainwindow.ui'), self)

        if isinstance(parent, plasmascript.Applet):
            self.setStyleSheet('QWidget#MainWindow{background-color: '
                               'rgba(0, 0, 0, 0);}')

        self.setWindowTitle('{0} {1}'.
                        format(QtCore.QCoreApplication.applicationName(),
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

    def closeEvent(self, event):
        QtGui.QWidget.closeEvent(self, event)

        self.tools.stopVideoRecord()
        event.accept()

    @QtCore.pyqtSlot(QtGui.QImage)
    def showFrame(self, webcamFrame):
        if self.tools.playing:
            self.lblFrame.setPixmap(QtGui.QPixmap.fromImage(webcamFrame))

    @QtCore.pyqtSlot()
    def updateWebcams(self):
        oldDevice = self.tools.curDevName
        timer_isActive = self.tools.playing
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
        if self.tools.recording:
            self.tools.stopVideoRecord()
        else:
            self.tools.startVideoRecord(self.saveFile(True))

    @QtCore.pyqtSlot(int)
    def on_cbxSetWebcam_currentIndexChanged(self, index):
        if self.tools.playing:
            self.tools.startDevice(self.tools.captureDevices()[index][0])

    @QtCore.pyqtSlot()
    def on_btnStartStop_clicked(self):
        if self.tools.playing:
            self.tools.stopCurrentDevice()
        else:
            self.tools.startDevice(self.tools.
                        captureDevices()[self.cbxSetWebcam.currentIndex()][0])

    def addWebcamConfigDialog(self, configDialog):
        self.cfgWebcamDialog = webcamconfig.WebcamConfig(self, self.tools)

        configDialog.addPage(self.cfgWebcamDialog,
                             self.tr('Webcam Settings'),
                             'camera-web',
                             self.tr('Configure the parameters of the webcam.'),
                             False)

    def addEffectsConfigDialog(self, configDialog):
        self.cfgEffects = effects.Effects(self, self.tools)
        self.tools.previewFrameReady.connect(self.cfgEffects.setEffectPreview)

        configDialog.addPage(self.cfgEffects,
                             self.tr('Configure Webcam Effects'),
                             'tools-wizard',
                             self.tr('Add funny effects to the webcam'),
                             False)

        configDialog.finished.connect(lambda: self.tools.previewFrameReady.disconnect(self.cfgEffects.setEffectPreview))

    def addVideoFormatsConfigDialog(self, configDialog):
        self.cfgVideoFormats = videorecordconfig.\
                                            VideoRecordConfig(self, self.tools)

        configDialog.\
               addPage(self.cfgVideoFormats,
                       self.tr('Configure Video Recording Formats'),
                       'video-x-generic',
                       self.tr('Add or remove video formats for recording.'),
                       False)

    def addStreamsConfigDialog(self, configDialog):
        self.cfgStreams = streamsconfig.StreamsConfig(self, self.tools)

        configDialog.\
               addPage(self.cfgStreams,
                       self.tr('Configure Custom Streams'),
                       'network-workgroup',
                       self.tr('Add or remove local or network live streams.'),
                       False)

    def addGeneralConfigsDialog(self, configDialog):
        self.cfgGeneralConfig = generalconfig.GeneralConfig(self, self.tools)

        configDialog.\
               addPage(self.cfgGeneralConfig,
                       self.tr('General Options'),
                       'configure',
                       self.tr('Setup the basic capture options.'),
                       False)

    def addFeaturesInfoDialog(self, configDialog):
        self.cfgFeaturesInfo = featuresinfo.FeaturesInfo(self, self.tools)

        configDialog.\
               addPage(self.cfgFeaturesInfo,
                       self.tr('Features'),
                       'dialog-information',
                       self.tr('This table will show you what packages you need.'),
                       False)

    @QtCore.pyqtSlot()
    def on_btnConfigure_clicked(self):
        config = kdeui.KConfigSkeleton('', self)

        configDialog = kdeui.\
                         KConfigDialog(self,
                                       self.tr(b'{0} Settings').toUtf8().
                                       data().format(QtCore.QCoreApplication.
                                                     applicationName()),
                                       config)

        configDialog.setWindowTitle(self.tr('{0} Settings').toUtf8().data().
                            format(QtCore.QCoreApplication.applicationName()))

        self.addWebcamConfigDialog(configDialog)
        self.addEffectsConfigDialog(configDialog)
        self.addVideoFormatsConfigDialog(configDialog)
        self.addStreamsConfigDialog(configDialog)
        self.addGeneralConfigsDialog(configDialog)
        self.addFeaturesInfoDialog(configDialog)

        configDialog.okClicked.connect(self.saveConfigs)
        configDialog.cancelClicked.connect(self.saveConfigs)

        configDialog.show()

    @QtCore.pyqtSlot()
    def saveConfigs(self):
        self.tools.saveConfigs()

    @QtCore.pyqtSlot()
    def on_btnAbout_clicked(self):
        aboutData = kdecore.\
            KAboutData(QtCore.QCoreApplication.applicationName().toUtf8().data(),
                       QtCore.QCoreApplication.applicationName().toUtf8().data(),
                       kdecore.ki18n(QtCore.QCoreApplication.
                                            applicationName()),
                       QtCore.QCoreApplication.applicationVersion().toUtf8().data(),
                       kdecore.ki18n(self.tr('webcam capture plasmoid.')),
                       kdecore.KAboutData.License_GPL_V3,
                       kdecore.ki18n(self.tr('Copyright (C) 2011-2012  '
                                             'Gonzalo Exequiel Pedone')),
                       kdecore.ki18n(self.tr('A simple webcam plasmoid and '
                                             'stand alone app for picture and '
                                             'video capture.')),
                       'http://github.com/hipersayanX/Webcamoid',
                       'submit@bugs.kde.org')

        aboutData.setProgramIconName('camera-web')

        aboutDialog = kdeui.KAboutApplicationDialog(aboutData, self)
        aboutDialog.show()

    @QtCore.pyqtSlot()
    def showGstError(self):
        kdeui.KNotification.\
                        event(kdeui.KNotification.Error,
                              self.tr('An error has occurred'),
                              self.tr('Please, check the "Features" section.'),
                              QtGui.QPixmap(),
                              None,
                              kdeui.KNotification.Persistent)

    def saveFile(self, video=False):
        curTime = time.strftime('%Y-%m-%d %H-%M-%S')

        if video:
            videosPath = kdeui.KGlobalSettings.videosPath().toUtf8().data()
            videoRecordFormats = self.tools.videoRecordFormats

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
            picturesPath = kdeui.KGlobalSettings.picturesPath().toUtf8().data()

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

        return '' if selected_files.isEmpty() else selected_files[0].toUtf8().data()


if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    mainWindow = MainWindow()
    mainWindow.show()
    app.exec_()
