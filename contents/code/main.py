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

from PyQt4 import QtCore, QtGui
from PyKDE4 import plasmascript
from PyKDE4.plasma import Plasma
from PyKDE4.kdeui import KDialog

import webcamoidgui
import config
import effects
import videorecordconfig
import translator


class Webcamoid(plasmascript.Applet):
    def __init__(self, parent, args=None):
        plasmascript.Applet.__init__(self, parent)

    def init(self):
        self.translator = translator.Translator('self.translator', self)
        self.defaultPlasmoidSize = QtCore.QSizeF(320, 240)
        self.minimumPlasmoidSize = QtCore.QSizeF(32, 32)
        self.applet.setPassivePopup(True)
        self.setPopupIcon('camera-web')
        self.setHasConfigurationInterface(True)
        self.setAspectRatioMode(Plasma.IgnoreAspectRatio)
        self.resize(self.defaultPlasmoidSize)
        self.setMinimumSize(self.minimumPlasmoidSize)

        self.webcamoidGui = webcamoidgui.WebcamoidGui(self)

        effects = str(self.config().readEntry('effects', '').toString())

        if effects != '':
            self.webcamoidGui.setEffects(effects.split(','))

        self.graphicsWidget = QtGui.QGraphicsWidget(self.applet)
        self.setGraphicsWidget(self.graphicsWidget)

        self.glyGraphicsWidget = QtGui.QGraphicsGridLayout(self.graphicsWidget)
        self.graphicsWidget.setLayout(self.glyGraphicsWidget)

        self.proxyWidget = QtGui.QGraphicsProxyWidget(self.graphicsWidget)
        self.proxyWidget.setWidget(self.webcamoidGui)
        self.proxyWidget.resize(self.defaultPlasmoidSize)
        self.proxyWidget.setMinimumSize(self.minimumPlasmoidSize)
        self.glyGraphicsWidget.addItem(self.proxyWidget, 0, 0, 1, 1)

    def createConfigurationInterface(self, parent):
        parent.setButtons(KDialog.ButtonCode(KDialog.Ok | KDialog.Cancel))

        self.cfgDialog = config.Config(self, self.webcamoidGui.v4l2Tools())

        parent.addPage(self.cfgDialog,
                       self.translator.tr('Webcam Settings'),
                       'camera-web',
                       self.translator.tr('Set webcam properties'),
                       False)

        self.cfgEffects = effects.Effects(self, self.webcamoidGui.v4l2Tools())

        parent.addPage(self.cfgEffects,
                       self.translator.tr('Configure Webcam Effects'),
                       'tools-wizard',
                       self.translator.tr('Add funny effects to the webcam'),
                       False)

        self.cfgVideoFormats = videorecordconfig.VideoRecordConfig(self, self.webcamoidGui.v4l2Tools())

        parent.addPage(self.cfgVideoFormats,
                       self.translator.tr('Configure Video Recording Formats'),
                       'video-x-generic',
                       self.translator.tr('Add or remove video formats for recording.'),
                       False)

        parent.okClicked.connect(self.saveConfigs)
        parent.cancelClicked.connect(self.saveConfigs)

    @QtCore.pyqtSlot()
    def saveConfigs(self):
        self.config().writeEntry('effects',
                                 ','.join(self.webcamoidGui.effects()))

        self.emit(QtCore.SIGNAL("configNeedsSaving()"))


def CreateApplet(parent):
    return Webcamoid(parent)
