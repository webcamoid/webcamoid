# Webcamod, webcam capture plasmoid.
# Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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
    error("commons.pri file not found.")
}

CONFIG += qt

FORMS = \
    share/ui/effects.ui \
    share/ui/featuresinfo.ui \
    share/ui/generalconfig.ui \
    share/ui/mainwindow.ui \
    share/ui/streamsconfig.ui \
    share/ui/videorecordconfig.ui \
    share/ui/webcamconfig.ui

HEADERS = \
    include/appenvironment.h \
    include/effects.h \
    include/featuresinfo.h \
    include/generalconfig.h \
    include/mainwindow.h \
    include/streamsconfig.h \
    include/v4l2tools.h \
    include/videorecordconfig.h \
    include/webcamconfig.h \
    include/commons.h

INCLUDEPATH += \
    include \
    /usr/include/KDE \
    /usr/include/gstreamer-0.10

LIBS += \
    -lkdecore \
    -lkdeui

QT += core gui

RESOURCES += \
    Webcamoid.qrc

SOURCES = \
    src/appenvironment.cpp \
    src/effects.cpp \
    src/featuresinfo.cpp \
    src/generalconfig.cpp \
    src/mainwindow.cpp \
    src/streamsconfig.cpp \
    src/v4l2tools.cpp \
    src/videorecordconfig.cpp \
    src/webcamconfig.cpp \
    src/main.cpp

TARGET = $${COMMONS_APPNAME}

TEMPLATE = app

# http://www.loc.gov/standards/iso639-2/php/code_list.php

CODECFORTR = UTF-8
CODECFORSRC = UTF-8

unix {
    CONFIG += link_pkgconfig

    PKGCONFIG += \
        libv4l2 \
        gstreamer-0.10 \
        gstreamer-app-0.10

    INSTALLS += target

    target.path = $${COMMONS_BINS_INSTALL_PATH}
}
