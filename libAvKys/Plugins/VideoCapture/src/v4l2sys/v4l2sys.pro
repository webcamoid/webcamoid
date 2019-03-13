# Webcamoid, webcam capture application.
# Copyright (C) 2017  Gonzalo Exequiel Pedone
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
    exists(../../../../akcommons.pri) {
        include(../../../../akcommons.pri)
    } else {
        error("akcommons.pri file not found.")
    }
}

CONFIG += plugin

HEADERS = \
    src/plugin.h \
    src/capturev4l2.h \
    src/capturebuffer.h \
    ../capture.h

INCLUDEPATH += \
    ../../../../Lib/src \
    ../

LIBS += -L$${OUT_PWD}/../../../../Lib/$${BIN_DIR} -l$${COMMONS_TARGET}

OTHER_FILES += pspec.json

CONFIG(config_v4l2_extendedcontrols): DEFINES += HAVE_EXTENDEDCONTROLS

QT += qml

SOURCES = \
    src/plugin.cpp \
    src/capturev4l2.cpp \
    ../capture.cpp

akModule = VideoCapture
DESTDIR = $${OUT_PWD}/../../$${BIN_DIR}/submodules/$${akModule}

TEMPLATE = lib

INSTALLS += target

android {
    TARGET = $${COMMONS_TARGET}_submodules_$${akModule}_lib$${TARGET}
    target.path = $${LIBDIR}
} else {
    target.path = $${INSTALLPLUGINSDIR}/submodules/$${akModule}
}
