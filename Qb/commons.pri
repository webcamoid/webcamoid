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

isEmpty(COMMONS_PRI_INCLUDE) {
    COMMONS_APPNAME = "Qb"
    COMMONS_TARGET = $${COMMONS_APPNAME}
    VER_MAJ = 5
    VER_MIN = 0
    VER_PAT = 0
    VERSION = $${VER_MAJ}.$${VER_MIN}.$${VER_PAT}
    COMMONS_PROJECT_URL = "http://github.com/hipersayanX/Webcamoid"
    COMMONS_PROJECT_LICENSE_URL = "https://raw.githubusercontent.com/hipersayanX/Webcamoid/master/COPYING"
    COMMONS_COPYRIGHT_NOTICE = "Copyright (C) 2011-2013  Gonzalo Exequiel Pedone"

    isEmpty(BUILDDOCS): BUILDDOCS = 0

    isEmpty(QDOCTOOL) {
        greaterThan(QT_MAJOR_VERSION, 4): QDOCTOOL = qdoc-qt5
        lessThan(QT_MAJOR_VERSION, 5): QDOCTOOL = qdoc3-qt4
    }

    isEmpty(QMAKE_LRELEASE) {
        win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\lrelease.exe
        else {
            greaterThan(QT_MAJOR_VERSION, 4): QMAKE_LRELEASE = lrelease-qt5
            lessThan(QT_MAJOR_VERSION, 5): QMAKE_LRELEASE = lrelease-qt4
        }
    }

    unix: isEmpty(PREFIX): PREFIX = /usr
    win32: isEmpty(PREFIX): PREFIX = "C:/Program Files/$${COMMONS_APPNAME}"

    isEmpty(EXECPREFIX): EXECPREFIX = $${PREFIX}
    isEmpty(BINDIR): BINDIR = $${EXECPREFIX}/bin
    isEmpty(SBINDIR): SBINDIR = $${EXECPREFIX}/sbin
    isEmpty(LIBEXECDIR): LIBEXECDIR = $${EXECPREFIX}/libexec
    isEmpty(DATAROOTDIR): DATAROOTDIR = $${PREFIX}/share
    isEmpty(DATADIR): DATADIR = $${DATAROOTDIR}/$${COMMONS_TARGET}
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

    unix {
        isEmpty(KDEINCLUDEDIR): KDEINCLUDEDIR = $$system(kde4-config --path include)
        isEmpty(KDELIBDIR): KDELIBDIR = $${PREFIX}/lib
    }

    DEFINES += \
        COMMONS_APPNAME=\"\\\"$$COMMONS_APPNAME\\\"\" \
        COMMONS_TARGET=\"\\\"$$COMMONS_TARGET\\\"\" \
        COMMONS_VERSION=\"\\\"$$VERSION\\\"\" \
        COMMONS_PROJECT_URL=\"\\\"$$COMMONS_PROJECT_URL\\\"\" \
        COMMONS_PROJECT_LICENSE_URL=\"\\\"$$COMMONS_PROJECT_LICENSE_URL\\\"\" \
        COMMONS_COPYRIGHT_NOTICE=\"\\\"$$COMMONS_COPYRIGHT_NOTICE\\\"\" \
        PREFIX=\"\\\"$$PREFIX\\\"\" \
        EXECPREFIX=\"\\\"$$EXECPREFIX\\\"\" \
        BINDIR=\"\\\"$$BINDIR\\\"\" \
        SBINDIR=\"\\\"$$SBINDIR\\\"\" \
        LIBEXECDIR=\"\\\"LIBEXECDIR\\\"\" \
        DATAROOTDIR=\"\\\"$$DATAROOTDIR\\\"\" \
        DATADIR=\"\\\"$$DATADIR\\\"\" \
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

    unix {
        DEFINES += \
            KDEINCLUDEDIR=\"\\\"$$KDEINCLUDEDIR\\\"\" \
            KDELIBDIR=\"\\\"$$KDELIBDIR\\\"\"
    }

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

    COMMONS_PRI_INCLUDE = 1
}
