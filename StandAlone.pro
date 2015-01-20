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

FORMS = \
    share/ui/generalconfig.ui \
    share/ui/streamsconfig.ui \
    share/ui/videorecordconfig.ui \
    share/ui/cameraconfig.ui \
    share/ui/imagedisplay.ui \
    share/ui/mainwindow.ui \
    share/ui/about.ui \
    share/ui/configdialog.ui

HEADERS = \
    include/about.h \
    include/cameraconfig.h \
    include/configdialog.h \
    include/generalconfig.h \
    include/imagedisplay.h \
    include/mainwindow.h \
    include/mediatools.h \
    include/streamsconfig.h \
    include/videodisplay.h \
    include/videoframe.h \
    include/recordingformat.h

INCLUDEPATH += \
    include \
    Qb/include

!win32: LIBS += -L./Qb -lQb
win32: LIBS += -L./Qb -lQb$${VER_MAJ}

OTHER_FILES = \
    .gitignore \
    README.md \
    share/effects.xml

QT += qml quick opengl widgets xml

RESOURCES += \
    Webcamoid.qrc \
    icons.qrc \
    qml.qrc \
    translations.qrc

SOURCES = \
    src/main.cpp \
    src/about.cpp \
    src/cameraconfig.cpp \
    src/configdialog.cpp \
    src/generalconfig.cpp \
    src/imagedisplay.cpp \
    src/mainwindow.cpp \
    src/mediatools.cpp \
    src/streamsconfig.cpp \
    src/videodisplay.cpp \
    src/videoframe.cpp \
    src/recordingformat.cpp

TRANSLATIONS = $$files(share/ts/*.ts)

DESTDIR = $${PWD}

TARGET = $${COMMONS_TARGET}

TEMPLATE = app

CODECFORTR = UTF-8
CODECFORSRC = UTF-8

# http://www.loc.gov/standards/iso639-2/php/code_list.php

CODECFORTR = UTF-8
CODECFORSRC = UTF-8

unix {
    INSTALLS += \
        target \
        desktop

    target.path = $${BINDIR}

    desktop.files = $${COMMONS_TARGET}.desktop
    desktop.path = $${DATAROOTDIR}/applications/kde4

    !isEmpty(BUILDDOCS):!isEqual(BUILDDOCS, 0) {
        INSTALLS += docs

        docs.files = share/docs_auto/html
        docs.path = $${HTMLDIR}
        docs.CONFIG += no_check_exist
    }
}
