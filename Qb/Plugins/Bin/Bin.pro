# Webcamoid, webcam capture application.
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
    exists(../../commons.pri) {
        include(../../commons.pri)
    } else {
        error("commons.pri file not found.")
    }
}

CONFIG += plugin

HEADERS += \
    include/bin.h \
    include/binelement.h \
    include/pipeline.h

INCLUDEPATH += \
    include \
    ../../include

LIBS += -lfl -ly
!win32: LIBS += -L../../ -lQb
win32: LIBS += -L../../ -lQb$${VER_MAJ}

FLEXSOURCES = parser/lexer.l
BISONSOURCES = parser/parser.y

OTHER_FILES += \
    pspec.json \
    $$FLEXSOURCES \
    $$BISONSOURCES

QT += core gui

SOURCES += \
    src/bin.cpp \
    src/binelement.cpp \
    src/pipeline.cpp

DESTDIR = $${PWD}

TEMPLATE = lib

INSTALLS += target

target.path = $${LIBDIR}/$${COMMONS_TARGET}

flexsource.input = FLEXSOURCES
flexsource.output = src/${QMAKE_FILE_BASE}_auto.cpp
flexsource.commands = flex --header-file=include/${QMAKE_FILE_BASE}_auto.h -o src/${QMAKE_FILE_BASE}_auto.cpp ${QMAKE_FILE_IN}
flexsource.variable_out = SOURCES
flexsource.name = Flex Sources ${QMAKE_FILE_IN}
flexsource.CONFIG += target_predeps

QMAKE_EXTRA_COMPILERS += flexsource
PRE_TARGETDEPS += compiler_flexsource_make_all

flexheader.input = FLEXSOURCES
flexheader.output = include/${QMAKE_FILE_BASE}_auto.h
flexheader.commands = @true
flexheader.variable_out = HEADERS
flexheader.name = Flex Headers ${QMAKE_FILE_IN}
flexheader.CONFIG += target_predeps no_link

QMAKE_EXTRA_COMPILERS += flexheader
PRE_TARGETDEPS += compiler_flexheader_make_all

bisonsource.input = BISONSOURCES
bisonsource.output = src/${QMAKE_FILE_BASE}_auto.cpp
bisonsource.commands = bison -d --defines=include/${QMAKE_FILE_BASE}_auto.h -o src/${QMAKE_FILE_BASE}_auto.cpp ${QMAKE_FILE_IN}
bisonsource.variable_out = SOURCES
bisonsource.name = Bison Sources ${QMAKE_FILE_IN}
bisonsource.CONFIG += target_predeps

QMAKE_EXTRA_COMPILERS += bisonsource
PRE_TARGETDEPS += compiler_bisonsource_make_all

bisonheader.input = BISONSOURCES
bisonheader.output = include/${QMAKE_FILE_BASE}_auto.h
bisonheader.commands = @true
bisonheader.variable_out = HEADERS
bisonheader.name = Bison Headers ${QMAKE_FILE_IN}
bisonheader.CONFIG += target_predeps no_link

QMAKE_EXTRA_COMPILERS += bisonheader
PRE_TARGETDEPS += compiler_bisonheader_make_all
