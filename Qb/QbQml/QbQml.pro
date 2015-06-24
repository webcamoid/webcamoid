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
    exists(../commons.pri) {
        include(../commons.pri)
    } else {
        error("commons.pri file not found.")
    }
}

TEMPLATE = lib

QT += qml quick
CONFIG += qt plugin

DESTDIR = $${PWD}

TARGET = $$qtLibraryTarget(QbQml)

# Input
SOURCES += \
    src/qbqml.cpp \
    src/qbqmlplugin.cpp

HEADERS += \
    include/qbqml.h \
    include/qbqmlplugin.h

INCLUDEPATH += \
    include \
    ../include

!win32: LIBS += -L../ -lQb
win32: LIBS += -L../ -lQb$${VER_MAJ}

DISTFILES = qmldir

INSTALLS += \
    target \
    qmldir

unix: installPath = $$[QT_INSTALL_QML]/QbQml
!unix: installPath = $${PREFIX}/qml/QbQml

target.path = $$installPath

qmldir.files = qmldir
qmldir.path = $$installPath
