#!/usr/bin/env python
#
# Webcamoid, camera capture application.
# Copyright (C) 2025  Gonzalo Exequiel Pedone
#
# Webcamoid is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Webcamoid is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
#
# Web-Site: http://webcamoid.github.io/

import sys
from PyQt6 import QtCore, QtWidgets, QtQml


if __name__ =='__main__':
    app = QtWidgets.QApplication(sys.argv);
    engine = QtQml.QQmlApplicationEngine()
    engine.load("Banner.qml")

    def capture():
        for obj in engine.rootObjects():
            image = obj.screen().grabWindow(obj.winId())
            image.save("banner.png")
            QtCore.QTimer.singleShot(1000, obj.close)

    for obj in engine.rootObjects():
        obj.sceneGraphInitialized.connect(capture)

    app.exec()
