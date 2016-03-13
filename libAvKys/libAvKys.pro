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
    error("commons.pri file not found.")
}

!isEmpty(BUILDDOCS):!isEqual(BUILDDOCS, 0) {
    DOCSOURCES += $${COMMONS_APPNAME}.qdocconf

    builddocs.input = DOCSOURCES
    builddocs.output = share/docs_auto/html/$${COMMONS_APPNAME}.index
    builddocs.commands = $${QDOCTOOL} ${QMAKE_FILE_IN}
    builddocs.variable_out = DOCSOUTPUT
    builddocs.name = Docs ${QMAKE_FILE_IN}
    builddocs.CONFIG += target_predeps

    QMAKE_EXTRA_COMPILERS += builddocs
    PRE_TARGETDEPS += compiler_builddocs_make_all
}

TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS += \
    Lib \
    AkQml \
    Plugins

# Install rules

INSTALLS += \
    license

license.files = ../COPYING
unix: license.path = $${LICENSEDIR}
!unix: license.path = $${PREFIX}/$${COMMONS_TARGET}

!isEmpty(BUILDDOCS):!isEqual(BUILDDOCS, 0) {
    INSTALLS += docs

    docs.files = share/docs_auto/html
    docs.path = $${HTMLDIR}
    docs.CONFIG += no_check_exist
}
