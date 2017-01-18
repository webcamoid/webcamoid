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
# Web-Site: http://webcamoid.github.io/

exists(commons.pri) {
    include(commons.pri)
} else {
    exists(../../../../commons.pri) {
        include(../../../../commons.pri)
    } else {
        error("commons.pri file not found.")
    }
}

CONFIG += plugin

HEADERS = \
    src/plugin.h \
    src/capturelibuvc.h \
    ../capture.h \
    src/usbids.h \
    src/usbglobals.h

INCLUDEPATH += \
    ../../../../Lib/src \
    ../

LIBS += -L$${PWD}/../../../../Lib/ -l$${COMMONS_TARGET}

OTHER_FILES += pspec.json

!isEmpty(LIBUSBINCLUDES): INCLUDEPATH += $${LIBUSBINCLUDES}
!isEmpty(LIBUSBLIBS): LIBS += $${LIBUSBLIBS}

isEmpty(LIBUSBLIBS) {
    CONFIG += link_pkgconfig
    PKGCONFIG += libusb-1.0
}

!isEmpty(LIBUVCINCLUDES): INCLUDEPATH += $${LIBUVCINCLUDES}
!isEmpty(LIBUVCLIBS): LIBS += $${LIBUVCLIBS}

isEmpty(LIBUVCLIBS) {
    LIBS += -luvc
}

CONFIG(config_libuvcdev): DEFINES += HAVE_LIBUVCDEV

QT += qml concurrent

RESOURCES = \
    libuvc.qrc

SOURCES = \
    src/plugin.cpp \
    src/capturelibuvc.cpp \
    ../capture.cpp \
    src/usbids.cpp \
    src/usbglobals.cpp

DESTDIR = $${OUT_PWD}/../../submodules/VideoCapture

TEMPLATE = lib

INSTALLS += target

target.path = $${LIBDIR}/$${COMMONS_TARGET}/submodules/VideoCapture
