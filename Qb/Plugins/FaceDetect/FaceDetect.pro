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
# Email   : hipersayan DOT x AT gmail DOT com
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

CONFIG += plugin

HEADERS += \
    src/facedetect.h \
    src/facedetectelement.h \
    src/haar/haarcascade.h \
    src/haar/haardetector.h \
    src/haar/haarfeature.h \
    src/haar/haarstage.h \
    src/haar/haartree.h

INCLUDEPATH += \
    ../../Lib/src

!win32: LIBS += -L../../Lib/ -lQb
win32: LIBS += -L../../Lib/ -lQb$${VER_MAJ}

OTHER_FILES += pspec.json

QT += qml widgets concurrent

RESOURCES += \
    FaceDetect.qrc

SOURCES += \
    src/facedetect.cpp \
    src/facedetectelement.cpp \
    src/haar/haarcascade.cpp \
    src/haar/haardetector.cpp \
    src/haar/haarfeature.cpp \
    src/haar/haarstage.cpp \
    src/haar/haartree.cpp

lupdate_only {
    SOURCES = share/qml/*.qml
}

DESTDIR = $${PWD}

TEMPLATE = lib

INSTALLS += target

unix: target.path = $${LIBDIR}/$${COMMONS_TARGET}
!unix: target.path = $${PREFIX}/Qb/Plugins
