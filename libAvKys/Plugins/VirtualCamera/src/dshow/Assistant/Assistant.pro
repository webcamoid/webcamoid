# Webcamoid, webcam capture application.
# Copyright (C) 2018  Gonzalo Exequiel Pedone
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
    exists(../../../../../akcommons.pri) {
        include(../../../../../akcommons.pri)
    } else {
        error("akcommons.pri file not found.")
    }
}

include(../dshow.pri)
include(../../VCamUtils/VCamUtils.pri)

TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DESTDIR = $${OUT_PWD}/$${BIN_DIR}

TARGET = $${DSHOW_PLUGIN_ASSISTANT_NAME}

TEMPLATE = app

LIBS += \
    -L$${OUT_PWD}/../PlatformUtils/$${BIN_DIR} -lPlatformUtils \
    -L$${OUT_PWD}/../../VCamUtils/$${BIN_DIR} -lVCamUtils \
    -ladvapi32 \
    -lgdi32 \
    -lole32 \
    -lshell32 \
    -lstrmiids \
    -luuid

SOURCES += \
    src/main.cpp \
    src/service.cpp

HEADERS += \
    src/service.h

INCLUDEPATH += \
    .. \
    ../..

isEmpty(STATIC_BUILD) | isEqual(STATIC_BUILD, 0) {
    win32-g++: QMAKE_LFLAGS = -static -static-libgcc -static-libstdc++
}

QMAKE_POST_LINK = \
    $$sprintf($$QMAKE_MKDIR_CMD, $$shell_path($${OUT_PWD}/../VirtualCamera/$${DSHOW_PLUGIN_NAME}_plugin/$$normalizedArch(TARGET_ARCH))) $${CMD_SEP} \
    $(COPY) $$shell_path($${OUT_PWD}/$${BIN_DIR}/$${DSHOW_PLUGIN_ASSISTANT_NAME}.exe) $$shell_path($${OUT_PWD}/../VirtualCamera/$${DSHOW_PLUGIN_NAME}_plugin/$$normalizedArch(TARGET_ARCH))
