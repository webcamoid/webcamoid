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

        self.isInit = True
        videoRecordFormats = self.tools.supportedVideoRecordFormats()

        self.tbwVideoFormats.setRowCount(len(videoRecordFormats))

        for row, fmt in enumerate(videoRecordFormats):
            for column, param in enumerate(fmt):
                self.tbwVideoFormats.setItem(row, column, QtGui.QTableWidgetItem(param))

        self.isInit = False

    @QtCore.pyqtSlot()
    def on_btnAdd_clicked(self):
        self.tbwVideoFormats.insertRow(self.tbwVideoFormats.rowCount())
        self.update()

    @QtCore.pyqtSlot()
    def on_btnRemove_clicked(self):
        self.tbwVideoFormats.removeRow(self.tbwVideoFormats.currentRow())
        self.update()

    @QtCore.pyqtSlot()
    def on_btnUp_clicked(self):
        currentRow = self.tbwVideoFormats.currentRow()
        nextRow = currentRow - 1

        if nextRow < 0:
            return

        for column in range(self.tbwVideoFormats.columnCount()):
            currentText = self.tbwVideoFormats.item(currentRow, column).text()
            nextText = self.tbwVideoFormats.item(nextRow, column).text()

            self.tbwVideoFormats.item(currentRow, column).setText(nextText)
            self.tbwVideoFormats.item(nextRow, column).setText(currentText)

        self.tbwVideoFormats.setCurrentCell(nextRow, self.tbwVideoFormats.currentColumn())
        self.update()

    @QtCore.pyqtSlot()
    def on_btnDown_clicked(self):
        currentRow = self.tbwVideoFormats.currentRow()
        nextRow = currentRow + 1

        if nextRow >= self.tbwVideoFormats.rowCount():
            return

        for column in range(self.tbwVideoFormats.columnCount()):
            currentText = self.tbwVideoFormats.item(currentRow, column).text()
            nextText = self.tbwVideoFormats.item(nextRow, column).text()

            self.tbwVideoFormats.item(currentRow, column).setText(nextText)
            self.tbwVideoFormats.item(nextRow, column).setText(currentText)

        self.tbwVideoFormats.setCurrentCell(nextRow, self.tbwVideoFormats.currentColumn())
        self.update()

    @QtCore.pyqtSlot(int, int)
    def on_tbwVideoFormats_cellChanged(self, row, column):
        self.update()

    def update(self):
        if not self.tools or self.isInit:
            return

        self.tools.clearVideoRecordFormats()

        for row in range(self.tbwVideoFormats.rowCount()):
            try:
                suffix = str(self.tbwVideoFormats.item(row, 0).text())
                videoEncoder = str(self.tbwVideoFormats.item(row, 1).text())
                audioEncoder = str(self.tbwVideoFormats.item(row, 2).text())
                muxer = str(self.tbwVideoFormats.item(row, 3).text())

                self.tools.setVideoRecordFormat(suffix, videoEncoder, audioEncoder, muxer)
            except:
                pass


if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    videoRecordConfig = VideoRecordConfig()
    videoRecordConfig.show()
    app.exec_()
