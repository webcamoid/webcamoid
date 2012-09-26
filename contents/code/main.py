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

from PyQt4 import QtCore, QtGui
from PyKDE4 import kdeui, plasma, plasmascript

import mainwindow


class Webcamoid(plasmascript.Applet):
    def __init__(self, parent, args=None):
        plasmascript.Applet.__init__(self, parent)

    def init(self):
        self.defaultPlasmoidSize = QtCore.QSizeF(320, 240)
        self.minimumPlasmoidSize = QtCore.QSizeF(32, 32)
        self.applet.setPassivePopup(True)
        self.setPopupIcon('camera-web')
        self.setHasConfigurationInterface(True)
        self.setAspectRatioMode(plasma.Plasma.IgnoreAspectRatio)
        self.resize(self.defaultPlasmoidSize)
        self.setMinimumSize(self.minimumPlasmoidSize)

        self.mainWindow = mainwindow.MainWindow(self)

        self.graphicsWidget = QtGui.QGraphicsWidget(self.applet)
        self.setGraphicsWidget(self.graphicsWidget)

        self.glyGraphicsWidget = QtGui.QGraphicsGridLayout(self.graphicsWidget)
        self.graphicsWidget.setLayout(self.glyGraphicsWidget)

        self.proxyWidget = QtGui.QGraphicsProxyWidget(self.graphicsWidget)
        self.proxyWidget.setWidget(self.mainWindow)
        self.proxyWidget.resize(self.defaultPlasmoidSize)
        self.proxyWidget.setMinimumSize(self.minimumPlasmoidSize)
        self.glyGraphicsWidget.addItem(self.proxyWidget, 0, 0, 1, 1)

    def createConfigurationInterface(self, configDialog):
        self.mainWindow.addWebcamConfigDialog(configDialog)
        self.mainWindow.addEffectsConfigDialog(configDialog)
        self.mainWindow.addVideoFormatsConfigDialog(configDialog)

        configDialog.okClicked.connect(self.mainWindow.saveConfigs)
        configDialog.cancelClicked.connect(self.mainWindow.saveConfigs)


def CreateApplet(parent):
    return Webcamoid(parent)
