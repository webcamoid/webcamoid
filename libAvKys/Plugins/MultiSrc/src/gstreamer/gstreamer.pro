# Webcamoid, webcam capture application.
# Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

exists(commons.pri) {
    include(commons.pri)
} else {
    exists(../../../../commons.pri) {
        include(../../../../commons.pri)
    } else {
        error("commons.pri file not found.")
    }
}

CONFIG += plugin

HEADERS += \
    src/plugin.h \
    src/mediasourcegstreamer.h \
    src/stream.h \
    ../mediasource.h

INCLUDEPATH += \
    ../../../../Lib/src \
    ../

LIBS += -L$${PWD}/../../../../Lib/ -l$${COMMONS_TARGET}

OTHER_FILES += pspec.json

!isEmpty(GSTREAMERINCLUDES): INCLUDEPATH += $${GSTREAMERINCLUDES}
!isEmpty(GSTREAMERLIBS): LIBS += $${GSTREAMERLIBS}

isEmpty(GSTREAMERLIBS) {
    CONFIG += link_pkgconfig

    PKGCONFIG += \
        gstreamer-1.0 \
        gstreamer-app-1.0 \
        gstreamer-audio-1.0 \
        gstreamer-video-1.0
}

QT += qml concurrent

SOURCES += \
    src/plugin.cpp \
    src/mediasourcegstreamer.cpp \
    ../mediasource.cpp

DESTDIR = $${OUT_PWD}/../../submodules/MultiSrc

TEMPLATE = lib

INSTALLS += target

target.path = $${LIBDIR}/$${COMMONS_TARGET}/submodules/MultiSrc
