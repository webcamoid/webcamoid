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

CONFIG += staticlib c++11
CONFIG -= qt

DESTDIR = $${OUT_PWD}

TARGET = "ipc"

TEMPLATE = lib

SOURCES = \
    src/mutex.cpp \
    src/waitcondition.cpp \
    src/ipcbridge.cpp

HEADERS =  \
    src/filtercommons.h \
    src/mutex.h \
    src/waitcondition.h \
    src/ipcbridge.h

isEmpty(STATIC_BUILD) | isEqual(STATIC_BUILD, 0) {
    win32-g++: QMAKE_LFLAGS = -static-libgcc -static-libstdc++
}
