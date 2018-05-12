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

CONFIG -= qt
CONFIG += \
    unversioned_libname \
    unversioned_soname \
    resources

INCLUDEPATH += \
    .. \
    ../..

LIBS = \
    -L$${OUT_PWD}/../../VCamUtils -lVCamUtils \
    -L$${OUT_PWD}/../VCamIPC -lVCamIPC \
    -framework CoreFoundation \
    -framework CoreMedia \
    -framework CoreMediaIO \
    -framework CoreVideo \
    -framework Foundation \
    -framework IOKit \
    -framework IOSurface

TARGET = $${CMIO_PLUGIN_NAME}
TEMPLATE = lib

HEADERS += \
    src/plugin.h \
    src/plugininterface.h \
    src/utils.h \
    src/device.h \
    src/object.h \
    src/stream.h \
    src/objectinterface.h \
    src/objectproperties.h \
    src/clock.h \
    src/queue.h

SOURCES += \
    src/plugin.cpp \
    src/plugininterface.cpp \
    src/utils.cpp \
    src/device.cpp \
    src/object.cpp \
    src/stream.cpp \
    src/objectinterface.cpp \
    src/objectproperties.cpp \
    src/clock.cpp

RESOURCES += \
    ../../../TestFrame.qrc

QMAKE_RESOURCE_FLAGS += \
    --no-compress

OTHER_FILES = \
    Info.plist

INSTALLS += plugin

plugin.files = $${TARGET}.plugin
plugin.path = $${DATAROOTDIR}/$${COMMONS_TARGET}
plugin.CONFIG += no_check_exist

QMAKE_POST_LINK = \
    rm -rvf $${TARGET}.plugin && \
    mkdir -p $${TARGET}.plugin/Contents/MacOS && \
    mkdir -p $${TARGET}.plugin/Contents/Resources && \
    cp -vf lib$${TARGET}.dylib $${TARGET}.plugin/Contents/MacOS/$${TARGET} && \
    cp -vf Info.plist $${TARGET}.plugin/Contents && \
    cp -rvf ../Assistant/$${CMIO_PLUGIN_ASSISTANT_NAME} $${TARGET}.plugin/Contents/Resources
