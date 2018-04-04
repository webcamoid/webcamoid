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
    -L$${OUT_PWD}/../VCamIPC -lVCamIPC \
    -ladvapi32 \
    -lkernel32 \
    -lole32 \
    -loleaut32 \
    -lstrmiids \
    -luser32 \
    -luuid \
    -lwinmm

TARGET = $${DSHOW_PLUGIN_NAME}
TEMPLATE = lib

HEADERS += \
    src/basefilter.h \
    src/classfactory.h \
    src/cunknown.h \
    src/enummediatypes.h \
    src/enumpins.h \
    src/filtermiscflags.h \
    src/latency.h \
    src/mediafilter.h \
    src/mediasample.h \
    src/mediasample2.h \
    src/memallocator.h \
    src/persist.h \
    src/persistpropertybag.h \
    src/pin.h \
    src/plugin.h \
    src/plugininterface.h \
    src/pushsource.h \
    src/referenceclock.h \
    src/specifypropertypages.h \
    src/streamconfig.h \
    src/utils.h \
    src/videoprocamp.h \
    src/propertyset.h \
    src/videocontrol.h \
    src/qualitycontrol.h

SOURCES += \
    src/basefilter.cpp \
    src/classfactory.cpp \
    src/cunknown.cpp \
    src/enummediatypes.cpp \
    src/enumpins.cpp \
    src/filtermiscflags.cpp \
    src/latency.cpp \
    src/mediafilter.cpp \
    src/mediasample.cpp \
    src/mediasample2.cpp \
    src/memallocator.cpp \
    src/persist.cpp \
    src/persistpropertybag.cpp \
    src/pin.cpp \
    src/plugin.cpp \
    src/plugininterface.cpp \
    src/pushsource.cpp \
    src/referenceclock.cpp \
    src/specifypropertypages.cpp \
    src/streamconfig.cpp \
    src/utils.cpp \
    src/videoprocamp.cpp \
    src/propertyset.cpp \
    src/videocontrol.cpp \
    src/qualitycontrol.cpp

DESTDIR = $${OUT_PWD}/bin/$${TARGET_ARCH}

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
