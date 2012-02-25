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

from PyQt4 import QtCore, QtGui, uic
from PyKDE4 import plasmascript
from PyKDE4.plasma import Plasma
from PyKDE4.kdeui import KIcon, KDialog, KNotification
from PyKDE4.kdecore import KConfigGroup
import webcamoidgui
import config

class Webcamoid(plasmascript.Applet):
    def __init__(self, parent, args=None):
        plasmascript.Applet.__init__(self, parent)

    def init(self):
        self.defaultPlasmoidSize = QtCore.QSizeF(320, 240)
        self.minimumPlasmoidSize = QtCore.QSizeF(128, 96)

        self.webcamoidGui = webcamoidgui.WebcamoidGui(self)
        self.webcamoidGui.setFFmpegExecutable(str(self.config().readEntry('ffmpegExecutable', 'ffmpeg').toString()))

        self.proxyWidget = QtGui.QGraphicsProxyWidget(self.applet)
        self.proxyWidget.setWidget(self.webcamoidGui)
        self.proxyWidget.resize(self.defaultPlasmoidSize)
        self.proxyWidget.setMinimumSize(self.minimumPlasmoidSize)

        self.applet.setPassivePopup(True)
        self.setGraphicsWidget(self.proxyWidget)
        self.setPopupIcon(KIcon("camera-web"))
        self.setHasConfigurationInterface(True)
        self.setAspectRatioMode(Plasma.IgnoreAspectRatio)
        self.resize(self.defaultPlasmoidSize)
        self.setMinimumSize(self.minimumPlasmoidSize)

    def createConfigurationInterface(self, parent):
        parent.setButtons(KDialog.ButtonCode(KDialog.Ok | KDialog.Cancel))
        self.cfgDialog = config.Config(self, self.webcamoidGui.v4l2Tools())
        parent.addPage(self.cfgDialog, 'Webcam Settings',  'camera-web', 'Set Webcam Properties',  False)

        parent.okClicked.connect(self.saveConfigs)
        parent.cancelClicked.connect(self.saveConfigs)

    @QtCore.pyqtSlot()
    def saveConfigs(self):
        self.config().writeEntry('ffmpegExecutable', self.webcamoidGui.ffmpegExecutable())
        self.emit(QtCore.SIGNAL("configNeedsSaving()"))

def CreateApplet(parent):
    return Webcamoid(parent)
