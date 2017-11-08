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
    exists(../../../../../commons.pri) {
        include(../../../../../commons.pri)
    } else {
        error("commons.pri file not found.")
    }
}

CONFIG -= qt
CONFIG += unversioned_libname unversioned_soname

debug {
    DEFINES += QT_DEBUG
}

LIBS = \
    -framework CoreFoundation \
    -framework CoreMedia \
    -framework CoreMediaIO \
    -framework CoreVideo \
    -framework IOKit \
    -framework IOSurface

TARGET = AkVirtualCamera
TEMPLATE = lib

HEADERS += \
    src/plugin.h \
    src/plugininterface.h \
    src/logger.h \
    src/utils.h \
    src/device.h \
    src/object.h \
    src/stream.h \
    src/objectinterface.h \
    src/objectproperties.h \
    src/videoformat.h \
    src/clock.h \
    src/queue.h

SOURCES += \
    src/plugin.cpp \
    src/plugininterface.cpp \
    src/logger.cpp \
    src/utils.cpp \
    src/device.cpp \
    src/object.cpp \
    src/stream.cpp \
    src/objectinterface.cpp \
    src/objectproperties.cpp \
    src/videoformat.cpp \
    src/clock.cpp

OTHER_FILES = \
    Info.plist

#unix {
#    target.path = /Library/CoreMediaIO/Plug-Ins/DAL
#    INSTALLS += target
#}

QMAKE_POST_LINK = \
    rm -rvf $${TARGET}.plugin && \
    mkdir -p $${TARGET}.plugin/Contents/MacOS && \
    mkdir -p $${TARGET}.plugin/Contents/Resources && \
    cp -vf lib$${TARGET}.dylib $${TARGET}.plugin/Contents/MacOS/$${TARGET} && \
    cp -vf Info.plist $${TARGET}.plugin/Contents
