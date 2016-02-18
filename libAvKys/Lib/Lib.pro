# Webcamoid, webcam capture application.
# Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
    exists(../commons.pri) {
        include(../commons.pri)
    } else {
        error("commons.pri file not found.")
    }
}

CONFIG += qt

DEFINES += \
    COMMONS_LIBRARY \
    QT_INSTALL_QML=\"\\\"$$[QT_INSTALL_QML]\\\"\"

HEADERS = \
    src/ak.h \
    src/akutils.h \
    src/akcaps.h \
    src/akelement.h \
    src/akfrac.h \
    src/akpacket.h \
    src/akplugin.h \
    src/akmultimediasourceelement.h \
    src/akvideocaps.h \
    src/akaudiocaps.h \
    src/akvideopacket.h \
    src/akaudiopacket.h

QT += qml

SOURCES = \
    src/ak.cpp \
    src/akutils.cpp \
    src/akcaps.cpp \
    src/akelement.cpp \
    src/akfrac.cpp \
    src/akpacket.cpp \
    src/akmultimediasourceelement.cpp \
    src/akvideocaps.cpp \
    src/akaudiocaps.cpp \
    src/akvideopacket.cpp \
    src/akaudiopacket.cpp

win32: LIBS += -lole32

DESTDIR = $${PWD}

TARGET = $${COMMONS_TARGET}

TEMPLATE = lib

CODECFORTR = UTF-8
CODECFORSRC = UTF-8

INSTALLS += target
unix: target.path = $${LIBDIR}
!unix: target.path = $${PREFIX}

!isEmpty(INSTALLDEVHEADERS):!isEqual(INSTALLDEVHEADERS, 0) {
    INSTALLS += headers
    headers.files = src/*.h
    unix: headers.path = $${INCLUDEDIR}/$${COMMONS_TARGET}
    !unix: headers.path = $${PREFIX}/${COMMONS_TARGET}/include
}
