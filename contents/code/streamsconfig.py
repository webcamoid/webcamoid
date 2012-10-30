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


class StreamsConfig(QtGui.QWidget):
    def __init__(self, parent=None, tools=None):
        QtGui.QWidget.__init__(self, parent)

        self.appEnvironment = appenvironment.AppEnvironment(self)
        uic.loadUi(self.resolvePath('../ui/streamsconfig.ui'), self)

        self.setWindowIcon(kdeui.KIcon('camera-web'))
        self.btnAdd.setIcon(kdeui.KIcon('list-add'))
        self.btnRemove.setIcon(kdeui.KIcon('list-remove'))
        self.btnUp.setIcon(kdeui.KIcon('arrow-up'))
        self.btnDown.setIcon(kdeui.KIcon('arrow-down'))

        self.tools = tools if tools else v4l2tools.V4L2Tools(self, True)
        self.isInit = True
        streams = self.tools.streams

        self.tbwCustomStreams.setRowCount(len(streams))

        for row, fmt in enumerate([(description, dev_name) for dev_name, description, streamType in streams]):
            for column, param in enumerate(fmt):
                self.tbwCustomStreams.setItem(row,
                                              column,
                                              QtGui.QTableWidgetItem(param))

        self.isInit = False
        self.tbwCustomStreams.resizeRowsToContents()
        self.tbwCustomStreams.resizeColumnsToContents()

    def resolvePath(self, relpath=''):
        return os.path.normpath(os.path.join(os.path.
                                dirname(os.path.realpath(__file__)), relpath))

    @QtCore.pyqtSlot()
    def on_btnAdd_clicked(self):
        self.tbwCustomStreams.insertRow(self.tbwCustomStreams.rowCount())
        self.update()

    @QtCore.pyqtSlot()
    def on_btnRemove_clicked(self):
        self.tbwCustomStreams.removeRow(self.tbwCustomStreams.currentRow())
        self.update()

    @QtCore.pyqtSlot()
    def on_btnUp_clicked(self):
        currentRow = self.tbwCustomStreams.currentRow()
        nextRow = currentRow - 1

        if nextRow < 0:
            return

        for column in range(self.tbwCustomStreams.columnCount()):
            currentText = self.tbwCustomStreams.item(currentRow, column).text()
            nextText = self.tbwCustomStreams.item(nextRow, column).text()

            self.tbwCustomStreams.item(currentRow, column).setText(nextText)
            self.tbwCustomStreams.item(nextRow, column).setText(currentText)

        self.tbwCustomStreams.\
                setCurrentCell(nextRow, self.tbwCustomStreams.currentColumn())

        self.update()

    @QtCore.pyqtSlot()
    def on_btnDown_clicked(self):
        currentRow = self.tbwCustomStreams.currentRow()
        nextRow = currentRow + 1

        if nextRow >= self.tbwCustomStreams.rowCount():
            return

        for column in range(self.tbwCustomStreams.columnCount()):
            currentText = self.tbwCustomStreams.item(currentRow, column).text()
            nextText = self.tbwCustomStreams.item(nextRow, column).text()

            self.tbwCustomStreams.item(currentRow, column).setText(nextText)
            self.tbwCustomStreams.item(nextRow, column).setText(currentText)

        self.tbwCustomStreams.\
                setCurrentCell(nextRow, self.tbwCustomStreams.currentColumn())

        self.update()

    @QtCore.pyqtSlot(int, int)
    def on_tbwCustomStreams_cellChanged(self, row, column):
        self.update()

    def update(self):
        self.tbwCustomStreams.resizeRowsToContents()
        self.tbwCustomStreams.resizeColumnsToContents()

        if self.isInit:
            return

        self.tools.clearCustomStreams()

        for row in range(self.tbwCustomStreams.rowCount()):
            try:
                description = self.tbwCustomStreams.item(row, 0).text().toUtf8().data()
                dev_name = self.tbwCustomStreams.item(row, 1).text().toUtf8().data()

                self.tools.setCustomStream(dev_name, description)
            except:
                pass


if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    streamsConfig = StreamsConfig()
    streamsConfig.show()
    app.exec_()
