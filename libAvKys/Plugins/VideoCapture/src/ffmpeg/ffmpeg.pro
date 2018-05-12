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

CONFIG += plugin

HEADERS = \
    src/plugin.h \
    src/convertvideoffmpeg.h \
    src/clock.h \
    ../convertvideo.h

INCLUDEPATH += \
    ../../../../Lib/src \
    ../

LIBS += -L$${PWD}/../../../../Lib/ -l$${COMMONS_TARGET}

OTHER_FILES += pspec.json

DEFINES += __STDC_CONSTANT_MACROS

!isEmpty(FFMPEGINCLUDES): INCLUDEPATH += $${FFMPEGINCLUDES}
!isEmpty(FFMPEGLIBS): LIBS += $${FFMPEGLIBS}

isEmpty(FFMPEGLIBS) {
    CONFIG += link_pkgconfig

    PKGCONFIG += \
        libavcodec \
        libswscale \
        libavutil
}

CONFIG(config_ffmpeg_avcodec_contextframerate): \
    DEFINES += HAVE_CONTEXTFRAMERATE
CONFIG(config_ffmpeg_avcodec_extracodecformats): \
    DEFINES += HAVE_EXTRACODECFORMATS
CONFIG(config_ffmpeg_avcodec_sendrecv): \
    DEFINES += HAVE_SENDRECV
CONFIG(config_ffmpeg_avutil_framealloc): \
    DEFINES += HAVE_FRAMEALLOC
CONFIG(config_ffmpeg_avutil_extrapixformats): \
    DEFINES += HAVE_EXTRAPIXFORMATS

QT += qml concurrent

SOURCES = \
    src/plugin.cpp \
    src/convertvideoffmpeg.cpp \
    src/clock.cpp \
    ../convertvideo.cpp

DESTDIR = $${OUT_PWD}/../../submodules/VideoCapture

TEMPLATE = lib

INSTALLS += target

target.path = $${LIBDIR}/$${COMMONS_TARGET}/submodules/VideoCapture
