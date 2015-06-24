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
    error("commons.pri file not found.")
}

!isEmpty(BUILDDOCS):!isEqual(BUILDDOCS, 0) {
    DOCSOURCES += $${COMMONS_APPNAME}.qdocconf

    builddocs.input = DOCSOURCES
    builddocs.output = share/docs_auto/html/$$replace(COMMONS_APPNAME, Q, q).index
    builddocs.commands = $${QDOCTOOL} ${QMAKE_FILE_IN}
    builddocs.variable_out = DOCSOUTPUT
    builddocs.name = Docs ${QMAKE_FILE_IN}
    builddocs.CONFIG += target_predeps

    QMAKE_EXTRA_COMPILERS += builddocs
    PRE_TARGETDEPS += compiler_builddocs_make_all
}

CONFIG += qt

DEFINES += \
    COMMONS_LIBRARY \
    QT_INSTALL_QML=\"\\\"$$[QT_INSTALL_QML]\\\"\"

HEADERS = \
    include/qb.h \
    include/qbutils.h \
    include/qbcaps.h \
    include/qbelement.h \
    include/qbfrac.h \
    include/qbpacket.h \
    include/qbplugin.h \
    include/qbmultimediasourceelement.h

INCLUDEPATH += \
    include

QT += qml multimedia

SOURCES = \
    src/qb.cpp \
    src/qbutils.cpp \
    src/qbcaps.cpp \
    src/qbelement.cpp \
    src/qbfrac.cpp \
    src/qbpacket.cpp \
    src/qbmultimediasourceelement.cpp

DESTDIR = $${PWD}

TARGET = $${COMMONS_APPNAME}

TEMPLATE = lib

CODECFORTR = UTF-8
CODECFORSRC = UTF-8

INSTALLS += target
unix: target.path = $${LIBDIR}
!unix: target.path = $${PREFIX}

!isEmpty(INSTALLDEVHEADERS):!isEqual(INSTALLDEVHEADERS, 0) {
    INSTALLS += headers
    headers.files = include/*
    unix: headers.path = $${INCLUDEDIR}/$${COMMONS_TARGET}
    !unix: headers.path = $${PREFIX}/Qb/include
}

!isEmpty(BUILDDOCS):!isEqual(BUILDDOCS, 0) {
    INSTALLS += docs

    docs.files = share/docs_auto/html
    docs.path = $${HTMLDIR}
    docs.CONFIG += no_check_exist
}
