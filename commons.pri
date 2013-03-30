# Carnival LiveCam, Augmented reality made easy.
# Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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
    COMMONS_COPYRIGHT_NOTICE = "Copyright (C) 2011-2012  Gonzalo Exequiel Pedone"

    unix {
        isEmpty(PREFIX) {
            PREFIX = /usr
        }

        COMMONS_BINS_INSTALL_PATH = $${PREFIX}/bin
        COMMONS_LIBS_INSTALL_PATH = $${PREFIX}/lib
        COMMONS_HEADERS_INSTALL_PATH = $${PREFIX}/include
        COMMONS_APP_HEADERS_INSTALL_PATH = $${COMMONS_HEADERS_INSTALL_PATH}/$${COMMONS_TARGET}
        COMMONS_DATA_INSTALL_PATH = $${PREFIX}/share
        COMMONS_APPS_INSTALL_PATH = $${COMMONS_DATA_INSTALL_PATH}/applications
        COMMONS_APP_DATA_INSTALL_PATH = $${COMMONS_DATA_INSTALL_PATH}/$${COMMONS_TARGET}
        COMMONS_APP_PLUGINS_INSTALL_PATH = $${COMMONS_LIBS_INSTALL_PATH}/$${COMMONS_TARGET}
        COMMONS_APP_TR_INSTALL_PATH = $${COMMONS_APP_DATA_INSTALL_PATH}/tr
        COMMONS_DOCS_INSTALL_PATH = $${COMMONS_DATA_INSTALL_PATH}/docs
        COMMONS_APP_DOCS_INSTALL_PATH = $${COMMONS_DOCS_INSTALL_PATH}/$${COMMONS_TARGET}
        COMMONS_LICENSE_INSTALL_PATH = $${COMMONS_DATA_INSTALL_PATH}/licenses/$${COMMONS_TARGET}
    }

    DEFINES += \
        COMMONS_APPNAME=\"\\\"$$COMMONS_APPNAME\\\"\" \
        COMMONS_TARGET=\"\\\"$$COMMONS_TARGET\\\"\" \
        COMMONS_VERSION=\"\\\"$$VERSION\\\"\" \
        COMMONS_PROJECT_URL=\"\\\"$$COMMONS_PROJECT_URL\\\"\" \
        COMMONS_PROJECT_BUG_URL=\"\\\"$$COMMONS_PROJECT_BUG_URL\\\"\" \
        COMMONS_COPYRIGHT_NOTICE=\"\\\"$$COMMONS_COPYRIGHT_NOTICE\\\"\" \
        COMMONS_BINS_INSTALL_PATH=\"\\\"$$COMMONS_BINS_INSTALL_PATH\\\"\" \
        COMMONS_LIBS_INSTALL_PATH=\"\\\"$$COMMONS_LIBS_INSTALL_PATH\\\"\" \
        COMMONS_HEADERS_INSTALL_PATH=\"\\\"$$COMMONS_HEADERS_INSTALL_PATH\\\"\" \
        COMMONS_APP_HEADERS_INSTALL_PATH=\"\\\"$$COMMONS_APP_HEADERS_INSTALL_PATH\\\"\" \
        COMMONS_DATA_INSTALL_PATH=\"\\\"$$COMMONS_DATA_INSTALL_PATH\\\"\" \
        COMMONS_APPS_INSTALL_PATH=\"\\\"$$COMMONS_APPS_INSTALL_PATH\\\"\" \
        COMMONS_APP_DATA_INSTALL_PATH=\"\\\"$$COMMONS_APP_DATA_INSTALL_PATH\\\"\" \
        COMMONS_APP_PLUGINS_INSTALL_PATH=\"\\\"$$COMMONS_APP_PLUGINS_INSTALL_PATH\\\"\" \
        COMMONS_APP_TR_INSTALL_PATH=\"\\\"$$COMMONS_APP_TR_INSTALL_PATH\\\"\" \
        COMMONS_DOCS_INSTALL_PATH=\"\\\"$$COMMONS_DOCS_INSTALL_PATH\\\"\" \
        COMMONS_APP_DOCS_INSTALL_PATH=\"\\\"$$COMMONS_APP_DOCS_INSTALL_PATH\\\"\" \
        COMMONS_LICENSE_INSTALL_PATH=\"\\\"$$COMMONS_LICENSE_INSTALL_PATH\\\"\"

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
