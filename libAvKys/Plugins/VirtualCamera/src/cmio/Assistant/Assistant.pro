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

exists(akcommons.pri) {
    include(akcommons.pri)
} else {
    exists(../../../../../akcommons.pri) {
        include(../../../../../akcommons.pri)
    } else {
        error("akcommons.pri file not found.")
    }
}

include(../cmio.pri)
include(../../VCamUtils/VCamUtils.pri)

CONFIG += console link_prl
CONFIG -= app_bundle
CONFIG -= qt

DESTDIR = $${OUT_PWD}/$${BIN_DIR}

TARGET = $${CMIO_PLUGIN_ASSISTANT_NAME}

TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/assistant.cpp

LIBS += \
    -L$${OUT_PWD}/../../VCamUtils/$${BIN_DIR} -lVCamUtils \
    -framework CoreFoundation

HEADERS += \
    src/assistantglobals.h \
    src/assistant.h

INCLUDEPATH += \
    ../..

QMAKE_POST_LINK = \
    $$sprintf($$QMAKE_MKDIR_CMD, $$shell_path($${OUT_PWD}/../VirtualCamera/$${CMIO_PLUGIN_NAME}.plugin/Contents/Resources)) $${CMD_SEP} \
    $(COPY) $$shell_path($${OUT_PWD}/$${BIN_DIR}/$${CMIO_PLUGIN_ASSISTANT_NAME}) $$shell_path($${OUT_PWD}/../VirtualCamera/$${CMIO_PLUGIN_NAME}.plugin/Contents/Resources)
