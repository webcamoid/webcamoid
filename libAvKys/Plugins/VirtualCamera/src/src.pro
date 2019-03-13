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

INCLUDEPATH += \
    ../../../Lib/src

HEADERS = \
    virtualcamera.h \
    virtualcameraelement.h \
    ipcbridge.h

SOURCES = \
    virtualcamera.cpp \
    virtualcameraelement.cpp

LIBS += \
    -L$${OUT_PWD}/../../../Lib/$${BIN_DIR} -l$${COMMONS_TARGET}
win32: LIBS += \
    -L$${OUT_PWD}/dshow/VCamIPC/$${BIN_DIR} -lVCamIPC \
    -L$${OUT_PWD}/dshow/PlatformUtils/$${BIN_DIR} -lPlatformUtils \
    -ladvapi32 \
    -lgdi32 \
    -lstrmiids \
    -luuid \
    -lole32 \
    -loleaut32 \
    -lshell32
macx: LIBS += \
    -L$${OUT_PWD}/cmio/VCamIPC/$${BIN_DIR} -lVCamIPC \
    -framework CoreFoundation \
    -framework CoreMedia \
    -framework CoreMediaIO \
    -framework CoreVideo \
    -framework Foundation \
    -framework IOKit \
    -framework IOSurface
unix: !macx: LIBS += \
    -L$${OUT_PWD}/v4l2sys/VCamIPC/$${BIN_DIR} -lVCamIPC
LIBS += \
    -L$${OUT_PWD}/VCamUtils/$${BIN_DIR} -lVCamUtils

OTHER_FILES += pspec.json

QT += concurrent qml xml

RESOURCES = ../VirtualCamera.qrc
unix: !macx: RESOURCES += ../TestFrame.qrc

lupdate_only {
    SOURCES += $$files(../share/qml/*.qml)
}

DESTDIR = $${OUT_PWD}/../$${BIN_DIR}
TARGET = VirtualCamera
android: TARGET = $${COMMONS_TARGET}_lib$${TARGET}

TEMPLATE = lib

INSTALLS += target
target.path = $${INSTALLPLUGINSDIR}
