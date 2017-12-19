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

include(VCamUtils.pri)

CONFIG += staticlib c++11
CONFIG -= qt

DESTDIR = $${OUT_PWD}

TARGET = VCamUtils

TEMPLATE = lib

SOURCES += \
    src/cstream/cstream.cpp \
    src/image/bitmap.cpp \
    src/image/videoformat.cpp \
    src/logger/logger.cpp \
    src/resources/rcdata.cpp \
    src/resources/rcloader.cpp \
    src/resources/rcname.cpp \
    src/resources/rcnode.cpp \
    src/image/videoframe.cpp

HEADERS += \
    src/utils.h \
    src/cstream/cstream.h \
    src/image/bitmap.h \
    src/image/color.h \
    src/image/videoformat.h \
    src/logger/logger.h \
    src/resources/rcdata.h \
    src/resources/rcloader.h \
    src/resources/rcname.h \
    src/resources/rcnode.h \
    src/image/videoframe.h
