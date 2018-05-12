# Webcamoid, webcam capture application.
# Copyright (C) 2016  Gonzalo Exequiel Pedone
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

exists(akcommons.pri) {
    include(akcommons.pri)
} else {
    exists(../akcommons.pri) {
        include(../akcommons.pri)
    } else {
        error("akcommons.pri file not found.")
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

lupdate_only {
    SOURCES += $$files(share/qml/AkQmlControls/*.qml)
}

HEADERS = \
    src/akqml.h \
    src/akqmlplugin.h

INCLUDEPATH += \
    ../Lib/src

LIBS += -L$${PWD}/../Lib/ -l$${COMMONS_TARGET}
win32: LIBS += -lole32

RESOURCES += \
    qml.qrc

DISTFILES = qmldir

INSTALLS += \
    target \
    qmldir

installPath = $${INSTALLQMLDIR}/AkQml
target.path = $$installPath

qmldir.files = qmldir
qmldir.path = $$installPath
