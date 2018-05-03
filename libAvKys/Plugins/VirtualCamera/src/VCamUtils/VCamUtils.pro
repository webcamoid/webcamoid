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
    exists(../../../../akcommons.pri) {
        include(../../../../akcommons.pri)
    } else {
        error("akcommons.pri file not found.")
    }
}

include(VCamUtils.pri)

CONFIG += staticlib c++11
CONFIG -= qt

DESTDIR = $${OUT_PWD}

TARGET = VCamUtils

TEMPLATE = lib

SOURCES += \
    src/cstream/cstreamread.cpp \
    src/cstream/cstreamwrite.cpp \
    src/image/videoformat.cpp \
    src/image/videoframe.cpp \
    src/logger/logger.cpp \
    src/resources/rcdata.cpp \
    src/resources/rcloader.cpp \
    src/resources/rcname.cpp \
    src/resources/rcnode.cpp \
    src/utils.cpp 

HEADERS += \
    src/cstream/cstreamread.h \
    src/cstream/cstreamwrite.h \
    src/image/color.h \
    src/image/videoformat.h \
    src/image/videoframe.h \
    src/image/videoframetypes.h \
    src/image/videoformattypes.h \
    src/logger/logger.h \
    src/resources/rcdata.h \
    src/resources/rcloader.h \
    src/resources/rcname.h \
    src/resources/rcnode.h \
    src/utils.h

isEmpty(STATIC_BUILD) | isEqual(STATIC_BUILD, 0) {
    win32-g++: QMAKE_LFLAGS = -static -static-libgcc -static-libstdc++
}
