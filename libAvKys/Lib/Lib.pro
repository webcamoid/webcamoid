# Webcamoid, webcam capture application.
# Copyright (C) 2016  Gonzalo Exequiel Pedone
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
    exists(../akcommons.pri) {
        include(../akcommons.pri)
    } else {
        error("akcommons.pri file not found.")
    }
}

CONFIG += qt
!isEmpty(STATIC_BUILD):!isEqual(STATIC_BUILD, 0): CONFIG += static

DEFINES += \
    AKCOMMONS_LIBRARY \
    QT_INSTALL_QML=\"\\\"$$[QT_INSTALL_QML]\\\"\"

HEADERS = \
    src/ak.h \
    src/akutils.h \
    src/akcaps.h \
    src/akcommons.h \
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
    src/akplugin.cpp \
    src/akmultimediasourceelement.cpp \
    src/akvideocaps.cpp \
    src/akaudiocaps.cpp \
    src/akvideopacket.cpp \
    src/akaudiopacket.cpp

win32: LIBS += -lole32

DESTDIR = $${OUT_PWD}/$${BIN_DIR}

TARGET = $${COMMONS_TARGET}

TEMPLATE = lib

CODECFORTR = UTF-8
CODECFORSRC = UTF-8

INSTALLS += target
unix:target.path = $${LIBDIR}
!unix:target.path = $${BINDIR}

!isEmpty(INSTALLDEVHEADERS):!isEqual(INSTALLDEVHEADERS, 0) {
    INSTALLS += headers
    headers.files = src/*.h
    headers.path = $${INCLUDEDIR}/$${COMMONS_TARGET}
}
