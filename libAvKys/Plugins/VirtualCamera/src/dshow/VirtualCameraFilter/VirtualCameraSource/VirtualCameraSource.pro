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

exists(commons.pri) {
    include(commons.pri)
} else {
    exists(../../../../../../commons.pri) {
        include(../../../../../../commons.pri)
    } else {
        error("commons.pri file not found.")
    }
}

CONFIG += c++11
CONFIG -= qt

DESTDIR = $${OUT_PWD}

TARGET = "VirtualCameraSource"

TEMPLATE = lib

SOURCES = \
    src/dllmain.cpp \
    src/vcguidef.cpp \
    src/virtualcamerasource.cpp \
    src/virtualcamerasourcestream.cpp \
    src/colorconv.cpp \
    src/imgfilters.cpp

HEADERS =  \
    src/vcguidef.h \
    src/virtualcamerasource.h \
    src/virtualcamerasourcestream.h \
    src/resources.h \
    src/colorconv.h \
    src/imgfilters.h

DEFINES += __STDC_CONSTANT_MACROS NO_DSHOW_STRSAFE

INCLUDEPATH += \
    ../BaseClasses/src \
    ../ipc/src

CONFIG(debug, debug|release) {
    LIBS += -L$${OUT_PWD}/../BaseClasses -lstrmbasd
} else {
    LIBS += -L$${OUT_PWD}/../BaseClasses -lstrmbase
}

LIBS += \
    -L$${OUT_PWD}/../ipc -lipc \
    -lstrmiids \
    -luuid \
    -lole32 \
    -loleaut32 \
    -ladvapi32 \
    -luser32 \
    -lwinmm \
    -lgdi32 \
    -lgdiplus
win32-g++: LIBS += -lksguid

OTHER_FILES = \
    VirtualCameraSource.def \
    VirtualCameraSource.rc

DEF_FILE = VirtualCameraSource.def
RC_FILE += VirtualCameraSource.rc

isEmpty(STATIC_BUILD) | isEqual(STATIC_BUILD, 0) {
    win32-g++: QMAKE_LFLAGS = -static-libgcc -static-libstdc++
}

INSTALLS += target

target.path = $${BINDIR}
