# Webcamoid, webcam capture application.
# Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
# Email  : hipersayan DOT x AT gmail DOT com
# Web-Site: http://github.com/hipersayanX/webcamoid

exists(commons.pri) {
    include(commons.pri)
} else {
    exists(../../commons.pri) {
        include(../../commons.pri)
    } else {
        error("commons.pri file not found.")
    }
}

!win32: {
    include(src/v4l2/v4l2.pri)
    include(src/ffmpeg/ffmpeg.pri)
}
win32: include(src/dshow/dshow.pri)

CONFIG += plugin

HEADERS += \
    src/videocapture.h \
    src/videocaptureelement.h

INCLUDEPATH += \
    ../../Lib/src

!win32: LIBS += -L../../Lib/ -lQb
win32: LIBS += -L../../Lib/ -lQb$${VER_MAJ}

OTHER_FILES += pspec.json

QT += qml concurrent

RESOURCES += \
    VideoCapture.qrc

SOURCES += \
    src/videocapture.cpp \
    src/videocaptureelement.cpp

lupdate_only {
    SOURCES = share/qml/*.qml
}

DESTDIR = $${PWD}

TEMPLATE = lib

INSTALLS += target

unix: target.path = $${LIBDIR}/$${COMMONS_TARGET}
!unix: target.path = $${PREFIX}/Qb/Plugins
