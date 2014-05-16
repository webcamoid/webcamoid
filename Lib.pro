# Webcamod, webcam capture plasmoid.
# Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
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

!isEmpty(BUILDDOCS):!isEqual(BUILDDOCS, 0) {
    DOCSOURCES += $${COMMONS_APPNAME}.qdocconf

    builddocs.input = DOCSOURCES
    builddocs.output = share/docs_auto/html/$${COMMONS_TARGET}.index
    builddocs.commands = $${QDOCTOOL} ${QMAKE_FILE_IN}
    builddocs.variable_out = DOCSOUTPUT
    builddocs.name = Docs ${QMAKE_FILE_IN}
    builddocs.CONFIG += target_predeps

    QMAKE_EXTRA_COMPILERS += builddocs
    PRE_TARGETDEPS += compiler_builddocs_make_all
}

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
    share/ui/generalconfig.ui \
    share/ui/mainwidget.ui \
    share/ui/streamsconfig.ui \
    share/ui/videorecordconfig.ui \
    share/ui/cameraconfig.ui \
    share/ui/imagedisplay.ui \
    share/ui/mainwindow.ui \
    share/ui/about.ui \
    share/ui/configdialog.ui

HEADERS = \
    include/appenvironment.h \
    include/commons.h \
    include/effects.h \
    include/generalconfig.h \
    include/mainwidget.h \
    include/mediatools.h \
    include/streamsconfig.h \
    include/videorecordconfig.h \
    include/cameraconfig.h \
    include/imagedisplay.h \
    include/mainwindow.h \
    include/about.h \
    include/configdialog.h

INCLUDEPATH += \
    include \
    Qb/include

LIBS += \
    -L./Qb -lQb

OTHER_FILES = \
    .gitignore \
    README.md \
    share/effects.xml

QT += core gui xml
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

RESOURCES += \
    Webcamoid.qrc

SOURCES = \
    src/appenvironment.cpp \
    src/effects.cpp \
    src/generalconfig.cpp \
    src/mainwidget.cpp \
    src/mediatools.cpp \
    src/streamsconfig.cpp \
    src/videorecordconfig.cpp \
    src/cameraconfig.cpp \
    src/imagedisplay.cpp \
    src/mainwindow.cpp \
    src/about.cpp \
    src/configdialog.cpp

TARGET = $${COMMONS_APPNAME}

TEMPLATE = lib

# http://www.loc.gov/standards/iso639-2/php/code_list.php

TRANSLATIONS = $$files(share/ts/*.ts)

CODECFORTR = UTF-8
CODECFORSRC = UTF-8

unix {
    INSTALLS += \
        target \
        translations

    target.path = $${LIBDIR}

    translations.files = share/ts/*.qm
    translations.path = $${DATADIR}/tr
    translations.CONFIG += no_check_exist

    !isEmpty(BUILDDOCS):!isEqual(BUILDDOCS, 0) {
        INSTALLS += docs

        docs.files = share/docs_auto/html
        docs.path = $${HTMLDIR}
        docs.CONFIG += no_check_exist
    }
}
