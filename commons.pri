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
    REQ_QT_MIN = 8
    REQ_QT_PAT = 4

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
    COMMONS_TARGET = "Webcamoid"
    VER_MAJ = 5
    VER_MIN = 0
    VER_PAT = 0
    VERSION = $${VER_MAJ}.$${VER_MIN}.$${VER_PAT}

    unix {
        COMMONS_BINS_INSTALL_PATH = /usr/bin
        COMMONS_LIBS_INSTALL_PATH = /usr/lib
        COMMONS_HEADERS_INSTALL_PATH = /usr/include/$${COMMONS_TARGET}
        COMMONS_DATA_INSTALL_PATH = /usr/share/$${COMMONS_TARGET}
        COMMONS_DOCS_INSTALL_PATH = /usr/share/docs/$${COMMONS_TARGET}
        COMMONS_LICENSE_INSTALL_PATH = /usr/share/licenses/$${COMMONS_TARGET}
    }

    DESTDIR = .
    COMMONS_BUILD_PATH = build
    COMMONS_DEBUG_BUILD_PATH = $${COMMONS_BUILD_PATH}/debug
    COMMONS_RELEASE_BUILD_PATH = $${COMMONS_BUILD_PATH}/release

    CONFIG(debug, debug|release) {
        MOC_DIR = $${COMMONS_DEBUG_BUILD_PATH}/moc
        OBJECTS_DIR = $${COMMONS_DEBUG_BUILD_PATH}/obj
        RCC_DIR = $${COMMONS_DEBUG_BUILD_PATH}/rcc
        UI_DIR = $${COMMONS_DEBUG_BUILD_PATH}/ui
    } else {
        MOC_DIR = $${COMMONS_RELEASE_BUILD_PATH}/moc
        OBJECTS_DIR = $${COMMONS_RELEASE_BUILD_PATH}/obj
        RCC_DIR = $${COMMONS_RELEASE_BUILD_PATH}/rcc
        UI_DIR = $${COMMONS_RELEASE_BUILD_PATH}/ui
    }

    COMMONS_PRI_INCLUDE = 1
}
