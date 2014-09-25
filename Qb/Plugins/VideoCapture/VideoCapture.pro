# Webcamoid, webcam capture application.
# Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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

CONFIG += plugin

DEFINES += __STDC_CONSTANT_MACROS NO_DSHOW_STRSAFE

HEADERS += \
    include/videocapture.h \
    include/videocaptureelement.h

!win32: HEADERS += \
    include/platform/capturebuffer.h \
    include/platform/capturelinux.h

win32: HEADERS += \
    include/platform/capturewin.h \
    include/platform/framegrabber.h

INCLUDEPATH += \
    include \
    ../../include

!win32: LIBS += -L../../ -lQb
win32: LIBS += -L../../ -lQb$${VER_MAJ}
win32: LIBS += -lstrmiids -lole32 -loleaut32

OTHER_FILES += pspec.json

QT += core gui

SOURCES += \
    src/videocapture.cpp \
    src/videocaptureelement.cpp

!win32: SOURCES += src/platform/capturelinux.cpp

win32: SOURCES += \
    src/platform/capturewin.cpp \
    src/platform/framegrabber.cpp

DESTDIR = $${PWD}

TEMPLATE = lib

unix {
    CONFIG += link_pkgconfig

    PKGCONFIG += libv4l2
}

INSTALLS += target

target.path = $${LIBDIR}/$${COMMONS_TARGET}
