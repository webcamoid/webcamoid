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
# Email   : hipersayan DOT x AT gmail DOT com
# Web-Site: http://github.com/hipersayanX/webcamoid

QT += concurrent widgets

DEFINES += __STDC_CONSTANT_MACROS

!isEmpty(FFMPEGINCLUDES): INCLUDEPATH += $${FFMPEGINCLUDES}
!isEmpty(FFMPEGLIBS): LIBS += $${FFMPEGLIBS}

isEmpty(FFMPEGLIBS) {
    CONFIG += link_pkgconfig

    PKGCONFIG += \
        libavdevice \
        libavfilter \
        libavformat \
        libavcodec \
        libpostproc \
        libswresample \
        libswscale \
        libavutil
}

HEADERS += \
    $$PWD/mediasource.h \
    $$PWD/abstractstream.h \
    $$PWD/audiostream.h \
    $$PWD/subtitlestream.h \
    $$PWD/videostream.h \
    $$PWD/framebuffer.h \
    $$PWD/clock.h

SOURCES += \
    $$PWD/mediasource.cpp \
    $$PWD/abstractstream.cpp \
    $$PWD/audiostream.cpp \
    $$PWD/subtitlestream.cpp \
    $$PWD/videostream.cpp \
    $$PWD/framebuffer.cpp \
    $$PWD/clock.cpp
