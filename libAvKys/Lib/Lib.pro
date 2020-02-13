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

CONFIG += \
    qt \
    create_prl \
    no_install_prl

!isEmpty(STATIC_BUILD):!isEqual(STATIC_BUILD, 0): CONFIG += static

DEFINES += \
    AKCOMMONS_LIBRARY \
    QT_INSTALL_QML=\"\\\"$$[QT_INSTALL_QML]\\\"\" \
    PREFIX_SHLIB=\"\\\"$$QMAKE_PREFIX_SHLIB\\\"\" \
    PLATFORM_TARGET_SUFFIX=\"\\\"$$qtPlatformTargetSuffix()\\\"\" \
    EXTENSION_SHLIB=\"\\\"$$QMAKE_EXTENSION_SHLIB\\\"\" \

HEADERS = \
    src/ak.h \
    src/akaudiocaps.h \
    src/akaudiopacket.h \
    src/akcaps.h \
    src/akcommons.h \
    src/akelement.h \
    src/akfrac.h \
    src/akmultimediasourceelement.h \
    src/akpacket.h \
    src/akplugin.h \
    src/akunit.h \
    src/akvideocaps.h \
    src/akvideopacket.h \
    src/qml/akcolorizedimage.h \
    src/qml/akpalette.h \
    src/qml/akpalettegroup.h \
    src/qml/aktheme.h

QT += gui qml quick widgets

SOURCES = \
    src/ak.cpp \
    src/akaudiocaps.cpp \
    src/akaudiopacket.cpp \
    src/akcaps.cpp \
    src/akelement.cpp \
    src/akfrac.cpp \
    src/akmultimediasourceelement.cpp \
    src/akpacket.cpp \
    src/akunit.cpp \
    src/akvideocaps.cpp \
    src/akvideopacket.cpp \
    src/qml/akcolorizedimage.cpp \
    src/qml/akpalette.cpp \
    src/qml/akpalettegroup.cpp \
    src/qml/aktheme.cpp

lupdate_only {
    SOURCES += $$files(share/qml/AkControls/*.qml)
}

OTHER_FILES = $$files(share/qml/AkControls/*.qml)

win32: LIBS += -lole32

RESOURCES += \
    AvKys.qrc

DESTDIR = $${OUT_PWD}/$${BIN_DIR}

android {
    TARGET = $${COMMONS_TARGET}
} else {
    TARGET = $$qtLibraryTarget($${COMMONS_TARGET})
}

TEMPLATE = lib

CODECFORTR = UTF-8
CODECFORSRC = UTF-8

INSTALLS += target
win32: target.path = $${BINDIR}
!win32: target.path = $${LIBDIR}

!isEmpty(INSTALLDEVHEADERS):!isEqual(INSTALLDEVHEADERS, 0) {
    CONFIG += create_pc

    INSTALLS += headers
    headers.files = src/*.h
    headers.path = $${INCLUDEDIR}/$${COMMONS_TARGET}
}
