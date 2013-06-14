# Carnival LiveCam, Augmented reality made easy.
# Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
#
# Carnival LiveCam is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Carnival LiveCam is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Carnival LiveCam.  If not, see <http://www.gnu.org/licenses/>.
#
# Email   : hipersayan DOT x AT gmail DOT com
# Web-Site: https://github.com/hipersayanX/Carnival-LiveCam

isEmpty(COMMONS_PRI_INCLUDE) {
    REQ_QT_MAJ = 4
    REQ_QT_MIN = 7
    REQ_QT_PAT = 0

    isEqual(QT_MAJOR_VERSION, $$REQ_QT_MAJ) {
        lessThan(QT_MINOR_VERSION, $$REQ_QT_MIN) {
            REQ_QT_NOTEXISTS = 1
        } else {
            lessThan(QT_PATCH_VERSION, $$REQ_QT_PAT) {
                REQ_QT_NOTEXISTS = 1
            }
        }
    } else {
        REQ_QT_NOTEXISTS = 1
    }

    !isEmpty(REQ_QT_NOTEXISTS): error("Your Qt version is $${QT_VERSION}. \
                                       Please, install Qt $${REQ_QT_MAJ}.$${REQ_QT_MIN}.$${REQ_QT_PAT} or later.")

    COMMONS_APPNAME = "Webcamoid"
    COMMONS_TARGET = $$replace(COMMONS_APPNAME, W, w)
    VER_MAJ = 5
    VER_MIN = 0
    VER_PAT = 0
    VERSION = $${VER_MAJ}.$${VER_MIN}.$${VER_PAT}
    COMMONS_PROJECT_URL = "http://github.com/hipersayanX/Webcamoid"
    COMMONS_PROJECT_BUG_URL = "https://github.com/hipersayanX/Webcamoid/issues"
    COMMONS_COPYRIGHT_NOTICE = "Copyright (C) 2011-2013  Gonzalo Exequiel Pedone"

    isEmpty(BUILDDOCS): BUILDDOCS = 0
    isEmpty(QDOCTOOL): QDOCTOOL = qdoc3-qt4

    unix {
        isEmpty(PREFIX): PREFIX = /usr
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
        isEmpty(KDEINCLUDEDIR): KDEINCLUDEDIR = $${INCLUDEDIR}/KDE
        isEmpty(DOCDIR): DOCDIR = $${DATAROOTDIR}/doc/$${COMMONS_TARGET}
        isEmpty(INFODIR): INFODIR = $${DATAROOTDIR}/info
        isEmpty(HTMLDIR): HTMLDIR = $${DOCDIR}/html
        isEmpty(DVIDIR): DVIDIR = $${DOCDIR}/dvi
        isEmpty(PDFDIR): PDFDIR = $${DOCDIR}/pdf
        isEmpty(PSDIR): PSDIR = $${DOCDIR}/ps
        isEmpty(LIBDIR): LIBDIR = $${EXECPREFIX}/lib
        isEmpty(LOCALEDIR): LOCALEDIR = $${DATAROOTDIR}/locale
        isEmpty(MANDIR): MANDIR = $${DATAROOTDIR}/man

        isEmpty(INSTALLPREFIX): INSTALLPREFIX = $${PREFIX}
        isEmpty(INSTALLEXECPREFIX): INSTALLEXECPREFIX = $${INSTALLPREFIX}
        isEmpty(INSTALLBINDIR): INSTALLBINDIR = $${INSTALLEXECPREFIX}/bin
        isEmpty(INSTALLSBINDIR): INSTALLSBINDIR = $${INSTALLEXECPREFIX}/sbin
        isEmpty(INSTALLLIBEXECDIR): INSTALLLIBEXECDIR = $${INSTALLEXECPREFIX}/libexec
        isEmpty(INSTALLDATAROOTDIR): INSTALLDATAROOTDIR = $${INSTALLPREFIX}/share
        isEmpty(INSTALLDATADIR): INSTALLDATADIR = $${INSTALLDATAROOTDIR}/$${COMMONS_TARGET}
        isEmpty(INSTALLSYSCONFDIR): INSTALLSYSCONFDIR = $${INSTALLPREFIX}/etc
        isEmpty(INSTALLSHAREDSTATEDIR): INSTALLSHAREDSTATEDIR = $${INSTALLPREFIX}/com
        isEmpty(INSTALLLOCALSTATEDIR): INSTALLLOCALSTATEDIR = $${INSTALLPREFIX}/var
        isEmpty(INSTALLINCLUDEDIR): INSTALLINCLUDEDIR = $${INSTALLPREFIX}/include
        isEmpty(INSTALLKDEINCLUDEDIR): INSTALLKDEINCLUDEDIR = $${INSTALLINCLUDEDIR}/KDE
        isEmpty(INSTALLDOCDIR): INSTALLDOCDIR = $${INSTALLDATAROOTDIR}/doc/$${COMMONS_TARGET}
        isEmpty(INSTALLINFODIR): INSTALLINFODIR = $${INSTALLDATAROOTDIR}/info
        isEmpty(INSTALLHTMLDIR): INSTALLHTMLDIR = $${INSTALLDOCDIR}/html
        isEmpty(INSTALLDVIDIR): INSTALLDVIDIR = $${INSTALLDOCDIR}/dvi
        isEmpty(INSTALLPDFDIR): INSTALLPDFDIR = $${INSTALLDOCDIR}/pdf
        isEmpty(INSTALLPSDIR): INSTALLPSDIR = $${INSTALLDOCDIR}/ps
        isEmpty(INSTALLLIBDIR): INSTALLLIBDIR = $${INSTALLEXECPREFIX}/lib
        isEmpty(INSTALLLOCALEDIR): INSTALLLOCALEDIR = $${INSTALLDATAROOTDIR}/locale
        isEmpty(INSTALLMANDIR): INSTALLMANDIR = $${INSTALLDATAROOTDIR}/man
    }

    DEFINES += \
        COMMONS_APPNAME=\"\\\"$$COMMONS_APPNAME\\\"\" \
        COMMONS_TARGET=\"\\\"$$COMMONS_TARGET\\\"\" \
        COMMONS_VERSION=\"\\\"$$VERSION\\\"\" \
        COMMONS_PROJECT_URL=\"\\\"$$COMMONS_PROJECT_URL\\\"\" \
        COMMONS_PROJECT_BUG_URL=\"\\\"$$COMMONS_PROJECT_BUG_URL\\\"\" \
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
        KDEINCLUDEDIR=\"\\\"$$KDEINCLUDEDIR\\\"\" \
        DOCDIR=\"\\\"$$DOCDIR\\\"\" \
        INFODIR=\"\\\"$$INFODIR\\\"\" \
        HTMLDIR=\"\\\"$$HTMLDIR\\\"\" \
        DVIDIR=\"\\\"$$DVIDIR\\\"\" \
        PDFDIR=\"\\\"$$PDFDIR\\\"\" \
        PSDIR=\"\\\"$$PSDIR\\\"\" \
        LIBDIR=\"\\\"$$LIBDIR\\\"\" \
        LOCALEDIR=\"\\\"$$LOCALEDIR\\\"\" \
        MANDIR=\"\\\"$$MANDIR\\\"\" \
        INSTALLPREFIX=\"\\\"$$INSTALLPREFIX\\\"\" \
        INSTALLEXECPREFIX=\"\\\"$$INSTALLEXECPREFIX\\\"\" \
        INSTALLBINDIR=\"\\\"$$INSTALLBINDIR\\\"\" \
        INSTALLSBINDIR=\"\\\"$$INSTALLSBINDIR\\\"\" \
        INSTALLLIBEXECDIR=\"\\\"INSTALLLIBEXECDIR\\\"\" \
        INSTALLDATAROOTDIR=\"\\\"$$INSTALLDATAROOTDIR\\\"\" \
        INSTALLDATADIR=\"\\\"$$INSTALLDATADIR\\\"\" \
        INSTALLSYSCONFDIR=\"\\\"$$INSTALLSYSCONFDIR\\\"\" \
        INSTALLSHAREDSTATEDIR=\"\\\"$$INSTALLSHAREDSTATEDIR\\\"\" \
        INSTALLLOCALSTATEDIR=\"\\\"$$INSTALLLOCALSTATEDIR\\\"\" \
        INSTALLINCLUDEDIR=\"\\\"$$INSTALLINCLUDEDIR\\\"\" \
        INSTALLKDEINCLUDEDIR=\"\\\"$$INSTALLKDEINCLUDEDIR\\\"\" \
        INSTALLDOCDIR=\"\\\"$$INSTALLDOCDIR\\\"\" \
        INSTALLINFODIR=\"\\\"$$INSTALLINFODIR\\\"\" \
        INSTALLHTMLDIR=\"\\\"$$INSTALLHTMLDIR\\\"\" \
        INSTALLDVIDIR=\"\\\"$$INSTALLDVIDIR\\\"\" \
        INSTALLPDFDIR=\"\\\"$$INSTALLPDFDIR\\\"\" \
        INSTALLPSDIR=\"\\\"$$INSTALLPSDIR\\\"\" \
        INSTALLLIBDIR=\"\\\"$$INSTALLLIBDIR\\\"\" \
        INSTALLLOCALEDIR=\"\\\"$$INSTALLLOCALEDIR\\\"\" \
        INSTALLMANDIR=\"\\\"$$INSTALLMANDIR\\\"\"

    DESTDIR = .

    CONFIG(debug, debug|release) {
        COMMONS_BUILD_PATH = build/debug
        DEFINES += QT_DEBUG
    } else {
        COMMONS_BUILD_PATH = build/release
    }

    MOC_DIR = $${COMMONS_BUILD_PATH}/moc
    OBJECTS_DIR = $${COMMONS_BUILD_PATH}/obj
    RCC_DIR = $${COMMONS_BUILD_PATH}/rcc
    UI_DIR = $${COMMONS_BUILD_PATH}/ui

    COMMONS_PRI_INCLUDE = 1
}
