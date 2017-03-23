# Webcamoid, webcam capture application.
# Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

COMMONS_APPNAME = "Webcamoid"
COMMONS_TARGET = $$lower($${COMMONS_APPNAME})
VER_MAJ = 8
VER_MIN = 0
VER_PAT = 0
VERSION = $${VER_MAJ}.$${VER_MIN}.$${VER_PAT}

isEmpty(BUILDDOCS): BUILDDOCS = 0
isEmpty(QDOCTOOL): {
    unix: QDOCTOOL = $$[QT_INSTALL_BINS]/qdoc
    !unix: QDOCTOOL = $$[QT_INSTALL_LIBEXECS]/qdoc
}
isEmpty(QMAKE_LRELEASE) {
    unix: QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
    !unix: QMAKE_LRELEASE = $$[QT_INSTALL_LIBEXECS]/lrelease
}

win32 {
    host_name = $$lower($$QMAKE_HOST.os)

    !isEmpty(ProgramW6432) {
        DEFAULT_PREFIX = $(ProgramW6432)\\$${COMMONS_APPNAME}
    } else: !isEmpty(ProgramFiles) {
        DEFAULT_PREFIX = $(ProgramFiles)\\$${COMMONS_APPNAME}
    } else: contains(host_name, linux) {
        DEFAULT_PREFIX = /$${COMMONS_APPNAME}
    } else {
        DEFAULT_PREFIX = C:\\$${COMMONS_APPNAME}
    }
} else {
    DEFAULT_PREFIX = /usr
}

isEmpty(PREFIX): PREFIX = $${DEFAULT_PREFIX}
isEmpty(EXECPREFIX): EXECPREFIX = $${PREFIX}
isEmpty(BINDIR): BINDIR = $${EXECPREFIX}/bin
isEmpty(SBINDIR): SBINDIR = $${EXECPREFIX}/sbin
isEmpty(LIBEXECDIR): LIBEXECDIR = $${EXECPREFIX}/libexec
isEmpty(DATAROOTDIR): DATAROOTDIR = $${PREFIX}/share
isEmpty(DATDIR): DATDIR = $${DATAROOTDIR}/$${COMMONS_TARGET}
isEmpty(SYSCONFDIR): SYSCONFDIR = $${PREFIX}/etc
isEmpty(SHAREDSTATEDIR): SHAREDSTATEDIR = $${PREFIX}/com
isEmpty(LOCALSTATEDIR): LOCALSTATEDIR = $${PREFIX}/var
isEmpty(INCLUDEDIR): INCLUDEDIR = $${PREFIX}/include
isEmpty(DOCDIR): DOCDIR = $${DATAROOTDIR}/doc/$${COMMONS_TARGET}
isEmpty(INFODIR): INFODIR = $${DATAROOTDIR}/info
isEmpty(HTMLDIR): HTMLDIR = $${DOCDIR}/html
isEmpty(DVIDIR): DVIDIR = $${DOCDIR}/dvi
isEmpty(PDFDIR): PDFDIR = $${DOCDIR}/pdf
isEmpty(PSDIR): PSDIR = $${DOCDIR}/ps
isEmpty(LIBDIR): LIBDIR = $${EXECPREFIX}/lib
isEmpty(LOCALEDIR): LOCALEDIR = $${DATAROOTDIR}/locale
isEmpty(MANDIR): MANDIR = $${DATAROOTDIR}/man
isEmpty(LICENSEDIR): LICENSEDIR = $${DATAROOTDIR}/licenses/$${COMMONS_TARGET}
isEmpty(LOCALDIR): LOCALDIR = $${PREFIX}/local
isEmpty(LOCALLIBDIR): LOCALLIBDIR = $${LOCALDIR}/lib

DEFINES += \
    COMMONS_APPNAME=\"\\\"$$COMMONS_APPNAME\\\"\" \
    COMMONS_TARGET=\"\\\"$$COMMONS_TARGET\\\"\" \
    COMMONS_VER_MAJ=\"\\\"$$VER_MAJ\\\"\" \
    COMMONS_VERSION=\"\\\"$$VERSION\\\"\" \
    PREFIX=\"\\\"$$PREFIX\\\"\" \
    EXECPREFIX=\"\\\"$$EXECPREFIX\\\"\" \
    BINDIR=\"\\\"$$BINDIR\\\"\" \
    SBINDIR=\"\\\"$$SBINDIR\\\"\" \
    LIBEXECDIR=\"\\\"LIBEXECDIR\\\"\" \
    DATAROOTDIR=\"\\\"$$DATAROOTDIR\\\"\" \
    DATDIR=\"\\\"$$DATDIR\\\"\" \
    SYSCONFDIR=\"\\\"$$SYSCONFDIR\\\"\" \
    SHAREDSTATEDIR=\"\\\"$$SHAREDSTATEDIR\\\"\" \
    LOCALSTATEDIR=\"\\\"$$LOCALSTATEDIR\\\"\" \
    INCLUDEDIR=\"\\\"$$INCLUDEDIR\\\"\" \
    DOCDIR=\"\\\"$$DOCDIR\\\"\" \
    INFODIR=\"\\\"$$INFODIR\\\"\" \
    HTMLDIR=\"\\\"$$HTMLDIR\\\"\" \
    DVIDIR=\"\\\"$$DVIDIR\\\"\" \
    PDFDIR=\"\\\"$$PDFDIR\\\"\" \
    PSDIR=\"\\\"$$PSDIR\\\"\" \
    LIBDIR=\"\\\"$$LIBDIR\\\"\" \
    LOCALEDIR=\"\\\"$$LOCALEDIR\\\"\" \
    MANDIR=\"\\\"$$MANDIR\\\"\" \
    LICENSEDIR=\"\\\"$$LICENSEDIR\\\"\" \
    LOCALDIR=\"\\\"$$LOCALDIR\\\"\" \
    LOCALLIBDIR=\"\\\"$$LOCALLIBDIR\\\"\"

CONFIG(debug, debug|release) {
    COMMONS_BUILD_PATH = build/Qt$${QT_VERSION}/$${QMAKE_CC}/debug
    DEFINES += QT_DEBUG
} else {
    COMMONS_BUILD_PATH = build/Qt$${QT_VERSION}/$${QMAKE_CC}/release
}

MOC_DIR = $${COMMONS_BUILD_PATH}/moc
OBJECTS_DIR = $${COMMONS_BUILD_PATH}/obj
RCC_DIR = $${COMMONS_BUILD_PATH}/rcc
UI_DIR = $${COMMONS_BUILD_PATH}/ui

# Compile translations files.
!isEmpty(TRANSLATIONS): CONFIG(debug, debug|release) {
    compiletr.input = TRANSLATIONS
    compiletr.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
    compiletr.commands = $$QMAKE_LRELEASE -removeidentical -compress ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
    compiletr.CONFIG += no_link
    QMAKE_EXTRA_COMPILERS += compiletr
    PRE_TARGETDEPS += compiler_compiletr_make_all
}

win32 {
    CONFIG += skip_target_version_ext
    !isEmpty(STATIC_BUILD):!isEqual(STATIC_BUILD, 0) {
        win32-g++: QMAKE_LFLAGS = -static-libgcc -static-libstdc++
    }
}
macx: QT_CONFIG -= no-pkg-config

# Enable c++11 support in all platforms
!CONFIG(c++11): CONFIG += c++11
