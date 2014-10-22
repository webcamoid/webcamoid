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

!isEmpty(OPENCVINCLUDES) {
    INCLUDEPATH += $${OPENCVINCLUDES}
}

!win32: !isEmpty(OPENCVLIBS): LIBS += $${OPENCVLIBS}

win32 {
    LIBS += \
        -lopencv_calib3d249 \
        -lopencv_contrib249 \
        -lopencv_core249 \
        -lopencv_features2d249 \
        -lopencv_flann249 \
        -lopencv_gpu249 \
        -lopencv_highgui249 \
        -lopencv_imgproc249 \
        -lopencv_legacy249 \
        -lopencv_ml249 \
        -lopencv_nonfree249 \
        -lopencv_objdetect249 \
        -lopencv_ocl249 \
        -lopencv_photo249 \
        -lopencv_stitching249 \
        -lopencv_superres249 \
        -lopencv_ts249 \
        -lopencv_video249 \
        -lopencv_videostab249
}

isEmpty(OPENCVHAARPATH): OPENCVHAARPATH = /usr/share/opencv/haarcascades
DEFINES += OPENCVHAARPATH=\"\\\"$$OPENCVHAARPATH\\\"\"

OTHER_FILES += pspec.json

QT += qml widgets

RESOURCES += \
    FaceDetect.qrc

SOURCES += \
    src/facedetect.cpp \
    src/facedetectelement.cpp

DESTDIR = $${PWD}

TEMPLATE = lib

unix {
    isEmpty(OPENCVLIBS) {
        CONFIG += link_pkgconfig

        PKGCONFIG += \
            opencv
    }
}

INSTALLS += target

target.path = $${LIBDIR}/$${COMMONS_TARGET}
