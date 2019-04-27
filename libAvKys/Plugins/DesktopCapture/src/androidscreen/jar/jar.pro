# Webcamoid, webcam capture application.
# Copyright (C) 2019  Gonzalo Exequiel Pedone
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

exists(akcommons.pri) {
    include(akcommons.pri)
} else {
    exists(../../../../../akcommons.pri) {
        include(../../../../../akcommons.pri)
    } else {
        error("akcommons.pri file not found.")
    }
}

AK_PLUGIN = DesktopCapture
AK_SUBMODULE = androidscreen

TARGET = Ak$${AK_PLUGIN}_$${AK_SUBMODULE}

CONFIG += java
DESTDIR = $${PWD}/../../../../../../StandAlone/share/android/libs

JAVACLASSPATH += \
    src

JAVASOURCES += \
    src/org/webcamoid/plugins/$${AK_PLUGIN}/submodules/$${AK_SUBMODULE}/AkAndroidScreenCallbacks.java

# install
target.path = $${JARDIR}
INSTALLS += target
