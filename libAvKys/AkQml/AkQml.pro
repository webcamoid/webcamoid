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
# Web-Site: http://webcamoid.github.io/

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

DESTDIR = $${OUT_PWD}

TARGET = $$qtLibraryTarget(AkQml)

# Input
SOURCES = \
    src/akqml.cpp \
    src/akqmlplugin.cpp

HEADERS = \
    src/akqml.h \
    src/akqmlplugin.h

INCLUDEPATH += \
    ../Lib/src

LIBS += -L$${PWD}/../Lib/ -l$${COMMONS_TARGET}
win32: LIBS += -lole32

DISTFILES = qmldir

INSTALLS += \
    target \
    qmldir

unix:!macx: installPath = $$[QT_INSTALL_QML]/AkQml
else: installPath = $${LIBDIR}/qt/qml/AkQml

target.path = $$installPath

qmldir.files = qmldir
qmldir.path = $$installPath
