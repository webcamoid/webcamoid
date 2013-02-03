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

isEmpty(QMAKE_LRELEASE) {
    win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\lrelease.exe
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}

# http://www.qtcentre.org/wiki/index.php?title=Undocumented_qmake

compiletr.input = TRANSLATIONS
compiletr.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
compiletr.commands = $$QMAKE_LRELEASE -removeidentical -compress ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
compiletr.CONFIG += no_link

QMAKE_EXTRA_COMPILERS += compiletr

PRE_TARGETDEPS += compiler_compiletr_make_all

CONFIG += qt

DEFINES += COMMONS_LIBRARY

FORMS = \
    share/ui/effects.ui \
    share/ui/featuresinfo.ui \
    share/ui/generalconfig.ui \
    share/ui/mainwidget.ui \
    share/ui/streamsconfig.ui \
    share/ui/videorecordconfig.ui \
    share/ui/webcamconfig.ui

HEADERS = \
    include/appenvironment.h \
    include/commons.h \
    include/effects.h \
    include/featuresinfo.h \
    include/generalconfig.h \
    include/mainwidget.h \
    include/mediatools.h \
    include/streamsconfig.h \
    include/videorecordconfig.h \
    include/webcamconfig.h \
    include/qbelement.h \
    include/qbplugin.h \
    include/qbpipeline.h \
    include/qbpacket.h \
    include/qbcaps.h \
    include/qbpluginloader.h \
    include/qbfrac.h

INCLUDEPATH += \
    include \
    /usr/include/KDE \
    /usr/include/gstreamer-1.0

LIBS += \
    -lkdecore \
    -lkdeui

QT += core gui

SOURCES = \
    src/appenvironment.cpp \
    src/effects.cpp \
    src/featuresinfo.cpp \
    src/generalconfig.cpp \
    src/mainwidget.cpp \
    src/mediatools.cpp \
    src/streamsconfig.cpp \
    src/videorecordconfig.cpp \
    src/webcamconfig.cpp \
    src/qbpipeline.cpp \
    src/qbpacket.cpp \
    src/qbcaps.cpp \
    src/qbelement.cpp \
    src/qbpluginloader.cpp \
    src/qbfrac.cpp

TARGET = $${COMMONS_APPNAME}

TEMPLATE = lib

# http://www.loc.gov/standards/iso639-2/php/code_list.php

TRANSLATIONS = \
    share/ts/ca.ts \
    share/ts/de.ts \
    share/ts/el.ts \
    share/ts/es.ts \
    share/ts/fr.ts \
    share/ts/gl.ts \
    share/ts/it.ts \
    share/ts/ja.ts \
    share/ts/ko.ts \
    share/ts/pt.ts \
    share/ts/ru.ts \
    share/ts/zh_CN.ts \
    share/ts/zh_TW.ts

CODECFORTR = UTF-8
CODECFORSRC = UTF-8

unix {
    CONFIG += link_pkgconfig

    PKGCONFIG += \
        libv4l2 \
        gstreamer-1.0 \
        gstreamer-app-1.0

    INSTALLS += \
        target \
        translations

    target.path = $${COMMONS_LIBS_INSTALL_PATH}

    translations.files = share/ts/*.qm
    translations.path = $${COMMONS_APP_TR_INSTALL_PATH}
}
