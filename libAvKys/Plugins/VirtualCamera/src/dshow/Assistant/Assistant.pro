# Webcamoid, webcam capture application.
# Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

include(../dshow.pri)
include(../../VCamUtils/VCamUtils.pri)

TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

DESTDIR = $${OUT_PWD}/../VirtualCamera/bin/$${TARGET_ARCH}

TARGET = $${DSHOW_PLUGIN_ASSISTANT_NAME}

TEMPLATE = app

LIBS += \
    -L$${OUT_PWD}/../../VCamUtils -lVCamUtils \
    -ladvapi32 \
    -lole32 \
    -lshell32 \
    -lstrmiids \
    -luuid

SOURCES += \
    src/main.cpp \
    src/service.cpp \
    ../VirtualCamera/src/utils.cpp

HEADERS += \
    src/service.h \
    ../VirtualCamera/src/utils.h

INCLUDEPATH += \
    ../.. \
    ../VirtualCamera/src

isEmpty(STATIC_BUILD) | isEqual(STATIC_BUILD, 0) {
    win32-g++: QMAKE_LFLAGS = -static -static-libgcc -static-libstdc++
}
