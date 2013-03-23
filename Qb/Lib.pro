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

DEFINES += COMMONS_LIBRARY

HEADERS = \
    include/qb.h \
    include/qbapplication.h \
    include/qbcaps.h \
    include/qbelement.h \
    include/qbfrac.h \
    include/qbpacket.h \
    include/qbplugin.h

INCLUDEPATH += \
    include

QT += core

SOURCES = \
    src/qb.cpp \
    src/qbapplication.cpp \
    src/qbcaps.cpp \
    src/qbelement.cpp \
    src/qbfrac.cpp \
    src/qbpacket.cpp

TARGET = $${COMMONS_APPNAME}

TEMPLATE = lib

CODECFORTR = UTF-8
CODECFORSRC = UTF-8

unix {
    INSTALLS += \
        target

    target.path = $${COMMONS_LIBS_INSTALL_PATH}
}
