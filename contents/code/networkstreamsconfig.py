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

from PyQt4 import uic, QtGui, QtCore
from PyKDE4 import kdeui

import appenvironment

# Find IP Cameras -> http://www.google.com/search?q=filetype:mjpg
#
# Example:
#
# http://208.42.203.54:8588/mjpg/video.mjpg

class NetworkStreamsConfig(QtGui.QWidget):
    def __init__(self, parent=None, tools=None):
        QtGui.QWidget.__init__(self, parent)

        self.appEnvironment = appenvironment.AppEnvironment(self)
        uic.loadUi(self.resolvePath('../ui/networkstreamsconfig.ui'), self)

        self.setWindowIcon(kdeui.KIcon('camera-web'))
        self.btnAdd.setIcon(kdeui.KIcon('list-add'))
        self.btnRemove.setIcon(kdeui.KIcon('list-remove'))
        self.btnUp.setIcon(kdeui.KIcon('arrow-up'))
        self.btnDown.setIcon(kdeui.KIcon('arrow-down'))

        self.tools = tools

        if not self.tools:
            return

        self.isInit = True
        networkStreams = self.tools.customNetworkStreams()

        self.tbwNetworkStreams.setRowCount(len(networkStreams))

        for row, fmt in enumerate([(description, dev_name) for dev_name, description, streamType in networkStreams]):
            for column, param in enumerate(fmt):
                self.tbwNetworkStreams.setItem(row,
                                               column,
                                               QtGui.QTableWidgetItem(param))

        self.isInit = False

    def resolvePath(self, relpath=''):
        return os.path.normpath(os.path.join(os.path.
                                dirname(os.path.realpath(__file__)), relpath))

    @QtCore.pyqtSlot()
    def on_btnAdd_clicked(self):
        self.tbwNetworkStreams.insertRow(self.tbwNetworkStreams.rowCount())
        self.update()

    @QtCore.pyqtSlot()
    def on_btnRemove_clicked(self):
        self.tbwNetworkStreams.removeRow(self.tbwNetworkStreams.currentRow())
        self.update()

    @QtCore.pyqtSlot()
    def on_btnUp_clicked(self):
        currentRow = self.tbwNetworkStreams.currentRow()
        nextRow = currentRow - 1

        if nextRow < 0:
            return

        for column in range(self.tbwNetworkStreams.columnCount()):
            currentText = self.tbwNetworkStreams.item(currentRow, column).text()
            nextText = self.tbwNetworkStreams.item(nextRow, column).text()

            self.tbwNetworkStreams.item(currentRow, column).setText(nextText)
            self.tbwNetworkStreams.item(nextRow, column).setText(currentText)

        self.tbwNetworkStreams.\
                setCurrentCell(nextRow, self.tbwNetworkStreams.currentColumn())

        self.update()

    @QtCore.pyqtSlot()
    def on_btnDown_clicked(self):
        currentRow = self.tbwNetworkStreams.currentRow()
        nextRow = currentRow + 1

        if nextRow >= self.tbwNetworkStreams.rowCount():
            return

        for column in range(self.tbwNetworkStreams.columnCount()):
            currentText = self.tbwNetworkStreams.item(currentRow, column).text()
            nextText = self.tbwNetworkStreams.item(nextRow, column).text()

            self.tbwNetworkStreams.item(currentRow, column).setText(nextText)
            self.tbwNetworkStreams.item(nextRow, column).setText(currentText)

        self.tbwNetworkStreams.\
                setCurrentCell(nextRow, self.tbwNetworkStreams.currentColumn())

        self.update()

    @QtCore.pyqtSlot(int, int)
    def on_tbwNetworkStreams_cellChanged(self, row, column):
        self.update()

    def update(self):
        if not self.tools or self.isInit:
            return

        self.tools.clearNetworkStreams()

        for row in range(self.tbwNetworkStreams.rowCount()):
            try:
                description = str(self.tbwNetworkStreams.item(row, 0).text())
                dev_name = str(self.tbwNetworkStreams.item(row, 1).text())

                self.tools.setNetworkStream(dev_name, description)
            except:
                pass


if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    networkStreamsConfig = NetworkStreamsConfig()
    networkStreamsConfig.show()
    app.exec_()
