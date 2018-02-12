# Webcamoid, webcam capture application.
# Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

CONFIG -= qt
CONFIG += \
    resources

INCLUDEPATH += \
    .. \
    ../..

LIBS += \
    -L$${OUT_PWD}/../../VCamUtils -lVCamUtils \
    -ladvapi32 \
    -lkernel32 \
    -lole32 \
    -loleaut32 \
    -lstrmiids \
    -luser32 \
    -luuid \
    -lwinmm

TARGET = $${DSHOW_PLUGIN_NAME}_$${TARGET_ARCH}
TEMPLATE = lib

HEADERS += \
    src/utils.h \
    src/plugininterface.h \
    src/plugin.h \
    src/classfactory.h \
    src/persistpropertybag.h \
    src/basefilter.h \
    src/cunknown.h \
    src/persist.h \
    src/mediafilter.h \
    src/enumpins.h \
    src/pin.h \
    src/enummediatypes.h \
    src/filtermiscflags.h \
    src/streamconfig.h \
    src/latency.h \
    src/pushsource.h \
    src/cameracontrol.h \
    src/videoprocamp.h \
    src/referenceclock.h \
    src/memallocator.h \
    src/mediasample.h \
    src/mediasample2.h

SOURCES += \
    src/plugin.cpp \
    src/utils.cpp \
    src/plugininterface.cpp \
    src/classfactory.cpp \
    src/persistpropertybag.cpp \
    src/basefilter.cpp \
    src/cunknown.cpp \
    src/persist.cpp \
    src/mediafilter.cpp \
    src/enumpins.cpp \
    src/pin.cpp \
    src/enummediatypes.cpp \
    src/filtermiscflags.cpp \
    src/streamconfig.cpp \
    src/latency.cpp \
    src/pushsource.cpp \
    src/cameracontrol.cpp \
    src/videoprocamp.cpp \
    src/referenceclock.cpp \
    src/memallocator.cpp \
    src/mediasample.cpp \
    src/mediasample2.cpp

DESTDIR = $${OUT_PWD}

RESOURCES += \
    ../../../TestFrame.qrc

QMAKE_RESOURCE_FLAGS += \
    --no-compress

OTHER_FILES = \
    VirtualCamera.def

DEF_FILE = VirtualCamera.def

isEmpty(STATIC_BUILD) | isEqual(STATIC_BUILD, 0) {
    win32-g++: QMAKE_LFLAGS = -static -static-libgcc -static-libstdc++
}

INSTALLS += target

target.path = $${BINDIR}
