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
from PyKDE4.kdeui import KIcon


class VideoRecordConfig(QtGui.QWidget):
    def __init__(self, parent=None, tools=None):
        QtGui.QWidget.__init__(self)

        try:
            uic.loadUi('../ui/videorecordconfig.ui', self)
        except:
            uic.loadUi(parent.package().filePath('ui', 'videorecordconfig.ui'), self)

        self.btnAdd.setIcon(KIcon('list-add'))
        self.btnRemove.setIcon(KIcon('list-remove'))
        self.btnUp.setIcon(KIcon('arrow-up'))
        self.btnDown.setIcon(KIcon('arrow-down'))

        self.tools = tools

        if not self.tools:
            return

    @QtCore.pyqtSlot()
    def on_btnAdd_clicked(self):
        self.update()

    @QtCore.pyqtSlot()
    def on_btnRemove_clicked(self):
        self.update()

    @QtCore.pyqtSlot()
    def on_btnUp_clicked(self):
        self.update()

    @QtCore.pyqtSlot()
    def on_btnDown_clicked(self):
        self.update()

    def update(self):
        if not self.tools:
            return

if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    videoRecordConfig = VideoRecordConfig()
    videoRecordConfig.show()
    app.exec_()
