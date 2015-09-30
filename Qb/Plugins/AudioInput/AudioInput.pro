# Webcamoid, webcam capture application.
# Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
# Email   : hipersayan DOT x AT gmail DOT com
# Web-Site: http://github.com/hipersayanX/webcamoid

exists(commons.pri) {
    include(commons.pri)
} else {
    exists(../../commons.pri) {
        include(../../commons.pri)
    } else {
        error("commons.pri file not found.")
    }
}

CONFIG += plugin

HEADERS += \
    src/audioinput.h \
    src/audioinputelement.h
!win32: HEADERS += src/pulseaudio/audiodevice.h
win32: HEADERS += src/wasapi/audiodevice.h

INCLUDEPATH += \
    ../../Lib/src

!win32: LIBS += -L../../Lib/ -lQb
win32: LIBS += -L../../Lib/ -lQb$${VER_MAJ}
win32: LIBS += -lole32 -lwinmm

OTHER_FILES += pspec.json

QT += qml concurrent

SOURCES += \
    src/audioinput.cpp \
    src/audioinputelement.cpp
!win32: SOURCES += src/pulseaudio/audiodevice.cpp
win32: SOURCES += src/wasapi/audiodevice.cpp

DESTDIR = $${PWD}

TEMPLATE = lib

unix {
    CONFIG += link_pkgconfig

    PKGCONFIG += libpulse-simple
}

INSTALLS += target

unix: target.path = $${LIBDIR}/$${COMMONS_TARGET}
!unix: target.path = $${PREFIX}/Qb/Plugins
