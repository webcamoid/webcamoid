# Webcamoid, webcam capture application.
# Copyright (C) 2017  Gonzalo Exequiel Pedone
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

exists(../translations.qrc) {
    TRANSLATIONS = $$files(../share/ts/*.ts)
    RESOURCES += ../translations.qrc
}

exists(akcommons.pri) {
    include(akcommons.pri)
} else {
    exists(../../../akcommons.pri) {
        include(../../../akcommons.pri)
    } else {
        error("akcommons.pri file not found.")
    }
}

CONFIG += plugin

HEADERS = \
    desktopcapture.h \
    desktopcaptureelement.h \
    desktopcaptureelementsettings.h \
    desktopcaptureglobals.h \
    screendev.h

INCLUDEPATH += \
    ../../../Lib/src

LIBS += -L$${OUT_PWD}/../../../Lib/$${BIN_DIR} -l$${COMMONS_TARGET}
win32: LIBS += -lole32

OTHER_FILES += \
    ../pspec.json \
    $$files(../share/qml/*.qml)

QT += qml concurrent

RESOURCES = \
    ../DesktopCapture.qrc

SOURCES = \
    desktopcapture.cpp \
    desktopcaptureelement.cpp \
    desktopcaptureelementsettings.cpp \
    desktopcaptureglobals.cpp \
    screendev.cpp

lupdate_only {
    SOURCES += $$files(../share/qml/*.qml)
}

DESTDIR = $${OUT_PWD}/../$${BIN_DIR}
TARGET = DesktopCapture
android: TARGET = $${COMMONS_TARGET}_lib$${TARGET}

TEMPLATE = lib

INSTALLS += target
target.path = $${INSTALLPLUGINSDIR}
