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

from PyQt4 import QtCore, QtGui, uic
from PyKDE4 import kdeui
import v4l2tools

import appenvironment


class GeneralConfig(QtGui.QWidget):
    def __init__(self, parent=None, tools=None):
        QtGui.QWidget.__init__(self, parent)

        self.appEnvironment = appenvironment.AppEnvironment(self)
        uic.loadUi(self.resolvePath('../ui/generalconfig.ui'), self)

        self.setWindowIcon(kdeui.KIcon('camera-web'))

        self.tools = tools if tools else v4l2tools.V4L2Tools(self, True)
        self.txtLauncher.setText(self.tools.launcher)
        self.txtPluginsPath.setText(self.tools.pluginsPath)
        self.txtExtraPlugins.setText(self.tools.extraPluginsPath)
        self.spnFps.setValue(self.tools.fps)
        self.chkAudioRecord.setCheckState(QtCore.Qt.Checked if self.tools.recordAudio else QtCore.Qt.Unchecked)

    def resolvePath(self, relpath=''):
        return os.path.normpath(os.path.join(os.path.
                                dirname(os.path.realpath(__file__)), relpath))

    @QtCore.pyqtSlot(str)
    def on_txtLauncher_textChanged(self, text):
        self.tools.setLauncher(text.toUtf8().data())

    @QtCore.pyqtSlot(str)
    def on_txtPluginsPath_textChanged(self, text):
        self.tools.setPluginsPath(text.toUtf8().data())

    @QtCore.pyqtSlot(str)
    def on_txtExtraPlugins_textChanged(self, text):
        self.tools.setExtraPluginsPath(text.toUtf8().data())

    @QtCore.pyqtSlot()
    def on_btnLauncher_clicked(self):
        saveFileDialog = QtGui.QFileDialog(self,
                                           self.tr('Select GStreamer '
                                                   'Executable'),
                                           '/usr/bin/gst-launch-0.10')

        saveFileDialog.setModal(True)
        saveFileDialog.setFileMode(QtGui.QFileDialog.ExistingFile)
        saveFileDialog.setAcceptMode(QtGui.QFileDialog.AcceptOpen)
        saveFileDialog.exec_()

        selected_files = saveFileDialog.selectedFiles()

        if not selected_files.isEmpty():
            self.txtLauncher.setText(selected_files[0].toUtf8().data())

    @QtCore.pyqtSlot()
    def on_btnPluginsPath_clicked(self):
        saveFileDialog = QtGui.QFileDialog(self,
                                           self.tr('Select GStreamer '
                                                   'Plugins Path'),
                                           '/usr/lib/gstreamer-0.10')

        saveFileDialog.setModal(True)
        saveFileDialog.setFileMode(QtGui.QFileDialog.Directory)
        saveFileDialog.setAcceptMode(QtGui.QFileDialog.AcceptOpen)
        saveFileDialog.exec_()

        selected_files = saveFileDialog.selectedFiles()

        if not selected_files.isEmpty():
            self.txtPluginsPath.setText(selected_files[0].toUtf8().data())

    @QtCore.pyqtSlot()
    def on_btnExtraPlugins_clicked(self):
        saveFileDialog = QtGui.QFileDialog(self,
                                           self.tr('Select Frei0r '
                                                   'Plugins Path'),
                                           '/usr/lib/frei0r-1')

        saveFileDialog.setModal(True)
        saveFileDialog.setFileMode(QtGui.QFileDialog.Directory)
        saveFileDialog.setAcceptMode(QtGui.QFileDialog.AcceptOpen)
        saveFileDialog.exec_()

        selected_files = saveFileDialog.selectedFiles()

        if not selected_files.isEmpty():
            self.txtExtraPlugins.setText(selected_files[0].toUtf8().data())

    @QtCore.pyqtSlot(int)
    def on_spnFps_valueChanged(self, i):
        self.tools.setFps(i)

    @QtCore.pyqtSlot(int)
    def on_chkAudioRecord_stateChanged(self, state):
        self.tools.enableAudioRecording(True if state == QtCore.Qt.Checked else False)


if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    generalConfig = GeneralConfig()
    generalConfig.show()
    app.exec_()
