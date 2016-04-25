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

TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS += \
    libAvKys \
    StandAlone

OTHER_FILES = \
    .gitignore \
    README.md

# Install rules

INSTALLS += \
    license

license.files = COPYING
license.path = $${LICENSEDIR}

unix {
    INSTALLS += desktop
    desktop.files = $${COMMONS_TARGET}.desktop
    desktop.path = $${DATAROOTDIR}/applications
}
!unix {
    INSTALLS += launcher
    launcher.files = webcamoid.bat
    launcher.path = $${PREFIX}
}
