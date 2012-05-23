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

#http://pygstdocs.berlios.de/

import sys

from PyQt4 import uic, QtGui, QtCore


class Effects(QtGui.QWidget):
    def __init__(self, parent=None, tools=None):
        QtGui.QWidget.__init__(self, parent)

        try:
            uic.loadUi('../ui/effects.ui', self)
        except:
            uic.loadUi(parent.package().filePath('ui', 'effects.ui'), self)

        self.tools = tools

    @QtCore.pyqtSlot()
    def on_btnAdd_clicked(self):
        for item in self.lswEffects.selectedItems():
            self.lswApply.addItem(self.lswEffects.takeItem(self.lswEffects.row(item)))

        self.update()

    @QtCore.pyqtSlot()
    def on_btnRemove_clicked(self):
        for item in self.lswApply.selectedItems():
            self.lswEffects.addItem(self.lswApply.takeItem(self.lswApply.row(item)))

        self.lswEffects.sortItems()
        self.update()

    @QtCore.pyqtSlot()
    def on_btnUp_clicked(self):
        for item in self.lswApply.selectedItems():
            row = self.lswApply.row(item)
            row_ = row - 1 if row >= 1 else 0
            item_ = self.lswApply.takeItem(row)
            self.lswApply.insertItem(row_, item_)
            item_.setSelected(True)

        self.update()

    @QtCore.pyqtSlot()
    def on_btnDown_clicked(self):
        for item in self.lswApply.selectedItems():
            row = self.lswApply.row(item)
            row_ = row + 1 if row < self.lswApply.count() - 1 else self.lswApply.count() - 1
            item_ = self.lswApply.takeItem(row)
            self.lswApply.insertItem(row_, item_)
            item_.setSelected(True)

        self.update()

    def update(self):
        if not self.tools:
            return

        effects = []

        for i in range(self.lswApply.count()):
            effects.append(self.lswApply.item(i).text())

if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    effects = Effects()
    effects.show()
    app.exec_()
