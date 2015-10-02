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
    src/qb.h \
    src/qbutils.h \
    src/qbcaps.h \
    src/qbelement.h \
    src/qbfrac.h \
    src/qbpacket.h \
    src/qbplugin.h \
    src/qbmultimediasourceelement.h \
    src/qbvideocaps.h \
    src/qbaudiocaps.h \
    src/qbvideopacket.h \
    src/qbaudiopacket.h

QT += qml

SOURCES = \
    src/qb.cpp \
    src/qbutils.cpp \
    src/qbcaps.cpp \
    src/qbelement.cpp \
    src/qbfrac.cpp \
    src/qbpacket.cpp \
    src/qbmultimediasourceelement.cpp \
    src/qbvideocaps.cpp \
    src/qbaudiocaps.cpp \
    src/qbvideopacket.cpp \
    src/qbaudiopacket.cpp

DESTDIR = $${PWD}

TARGET = $${COMMONS_APPNAME}

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
    !unix: headers.path = $${PREFIX}/Qb/include
}
