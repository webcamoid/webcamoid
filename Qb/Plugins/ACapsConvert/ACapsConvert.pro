# Webcamod, webcam capture plasmoid.
# Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
#
# Webcamod is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Webcamod is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
#
# Email     : hipersayan DOT x AT gmail DOT com
# Web-Site 1: http://github.com/hipersayanX/Webcamoid
# Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796

exists(commons.pri) {
    include(commons.pri)
} else {
    exists(../../commons.pri) {
        include(../../commons.pri)
    } else {
        error("commons.pri file not found.")
    }
}

exists(../../3dparty/ffmpeg_auto.pri) {
    include(../../3dparty/ffmpeg_auto.pri)
}

CONFIG += plugin

DEFINES += __STDC_CONSTANT_MACROS

HEADERS += \
    include/acapsconvert.h \
    include/acapsconvertelement.h

INCLUDEPATH += \
    include \
    ../../include

LIBS += -L../../ -lQb

exists(../../3dparty/ffmpeg_auto.pri) {
    INCLUDEPATH += $${FFMPEGHEADERSPATH}

    LIBS += \
        -L$${FFMPEGLIBSPATH} \
        -lavdevice$${FFMPEGBUILDSUFFIX} \
        -lavfilter$${FFMPEGBUILDSUFFIX} \
        -lavformat$${FFMPEGBUILDSUFFIX} \
        -lavcodec$${FFMPEGBUILDSUFFIX} \
        -lavresample$${FFMPEGBUILDSUFFIX} \
        -lpostproc$${FFMPEGBUILDSUFFIX} \
        -lswresample$${FFMPEGBUILDSUFFIX} \
        -lswscale$${FFMPEGBUILDSUFFIX} \
        -lavutil$${FFMPEGBUILDSUFFIX}
}

QT += core gui

SOURCES += \
    src/acapsconvert.cpp \
    src/acapsconvertelement.cpp

TEMPLATE = lib

unix {
    ! exists(../../3dparty/ffmpeg_auto.pri) {
        CONFIG += link_pkgconfig

        PKGCONFIG += \
            libavdevice \
            libavfilter \
            libavformat \
            libavcodec \
            libavresample \
            libpostproc \
            libswresample \
            libswscale \
            libavutil
    }

    INSTALLS += target

    target.path = $${LIBDIR}/$${COMMONS_TARGET}
}
