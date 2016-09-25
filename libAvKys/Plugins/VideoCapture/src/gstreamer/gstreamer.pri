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

DEFINES += USE_GSTREAMER

!isEmpty(GSTREAMERINCLUDES): INCLUDEPATH += $${GSTREAMERINCLUDES}
!isEmpty(GSTREAMERLIBS): LIBS += $${GSTREAMERLIBS}

isEmpty(GSTREAMERLIBS) {
    macx {
        LIBS += \
            -lgstapp-1.0 \
            -lgstvideo-1.0 \
            -lgstbase-1.0 \
            -lgstpbutils-1.0 \
            -lgstreamer-1.0 \
            -lgobject-2.0 \
            -lglib-2.0
    } else {
        CONFIG += link_pkgconfig

        PKGCONFIG += \
            gstreamer-1.0 \
            gstreamer-app-1.0 \
            gstreamer-video-1.0 \
            gstreamer-pbutils-1.0
    }
}

HEADERS += \
    $$PWD/convertvideo.h

SOURCES += \
    $$PWD/convertvideo.cpp
