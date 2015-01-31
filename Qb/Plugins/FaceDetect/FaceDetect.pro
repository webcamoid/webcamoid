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
# Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796

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

HEADERS += \
    include/facedetect.h \
    include/facedetectelement.h

INCLUDEPATH += \
    include \
    ../../include

!win32: LIBS += -L../../ -lQb
win32: LIBS += -L../../ -lQb$${VER_MAJ}

!isEmpty(OPENCVINCLUDES): INCLUDEPATH += $${OPENCVINCLUDES}
!isEmpty(OPENCVLIBS): LIBS += $${OPENCVLIBS}

OTHER_FILES += pspec.json

QT += qml widgets

RESOURCES += \
    FaceDetect.qrc

SOURCES += \
    src/facedetect.cpp \
    src/facedetectelement.cpp

DESTDIR = $${PWD}

TEMPLATE = lib

isEmpty(OPENCVLIBS) {
    CONFIG += link_pkgconfig

    PKGCONFIG += \
        opencv
}

INSTALLS += target

unix: target.path = $${LIBDIR}/$${COMMONS_TARGET}
!unix: target.path = $${PREFIX}/Qb/Plugins
