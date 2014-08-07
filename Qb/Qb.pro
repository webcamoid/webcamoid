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
    error("commons.pri file not found.")
}

unix: !isEmpty(USE3DPARTYLIBS): !isEqual(USE3DPARTYLIBS, 0) {
    !exists(/usr/bin/wget): error("USE3DPARTYLIBS option requires Wget.")
}

TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS += Lib.pro

win32 {
    SUBDIRS += 3dparty
} else: !isEmpty(USE3DPARTYLIBS):!isEqual(USE3DPARTYLIBS, 0): SUBDIRS += 3dparty

SUBDIRS += Plugins

win32 {
    Plugins.depends = 3dparty
} else: !isEmpty(USE3DPARTYLIBS):!isEqual(USE3DPARTYLIBS, 0): Plugins.depends = 3dparty

# Install rules

INSTALLS += \
    license

license.files = COPYING
license.path = $${LICENSEDIR}
