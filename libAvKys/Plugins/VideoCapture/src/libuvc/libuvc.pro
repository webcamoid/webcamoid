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
    exists(../../../../akcommons.pri) {
        include(../../../../akcommons.pri)
    } else {
        error("akcommons.pri file not found.")
    }
}

CONFIG += plugin link_prl

HEADERS = \
    ../capture.h \
    src/plugin.h \
    src/capturelibuvc.h \
    src/usbids.h \
    src/usbglobals.h

INCLUDEPATH += \
    ../../../../Lib/src \
    ../

LIBS += -L$${OUT_PWD}/../../../../Lib/$${BIN_DIR} -l$$qtLibraryTarget($${COMMONS_TARGET})

OTHER_FILES += pspec.json

isEmpty(LIBUSBLIBS) | isEmpty(LIBUVCLIBS): CONFIG += link_pkgconfig

!isEmpty(LIBUSBINCLUDES): INCLUDEPATH += $${LIBUSBINCLUDES}
!isEmpty(LIBUSBLIBS): LIBS += $${LIBUSBLIBS}
isEmpty(LIBUSBLIBS): PKGCONFIG += libusb-1.0

!isEmpty(LIBUVCINCLUDES): INCLUDEPATH += $${LIBUVCINCLUDES}
!isEmpty(LIBUVCLIBS): LIBS += $${LIBUVCLIBS}
isEmpty(LIBUVCLIBS): PKGCONFIG += libuvc

QT += qml concurrent

RESOURCES = \
    libuvc.qrc

SOURCES = \
    src/plugin.cpp \
    src/capturelibuvc.cpp \
    ../capture.cpp \
    src/usbids.cpp \
    src/usbglobals.cpp

CONFIG(config_libusb_has_hotplug): \
    DEFINES += HAS_HOTPLUG

akModule = VideoCapture
DESTDIR = $${OUT_PWD}/../../$${BIN_DIR}/submodules/$${akModule}

TEMPLATE = lib

INSTALLS += target

android {
    TARGET = $${COMMONS_TARGET}_submodules_$${akModule}_lib$${TARGET}
    target.path = $${LIBDIR}
} else {
    target.path = $${INSTALLPLUGINSDIR}/submodules/$${akModule}
}
