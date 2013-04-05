# Webcamod, webcam capture plasmoid.
# Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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

PACKAGENAME=ffmpeg
PACKAGEVERSION=1.2
PACKAGEFOLDER=$${PACKAGENAME}-$${PACKAGEVERSION}
FILEEXT=tar.bz2
PACKAGEFILE=$${PACKAGEFOLDER}.$${FILEEXT}
INSTALLPREFIX=$${PWD}/$${PACKAGENAME}_priv/usr

TEMPLATE = lib

CONFIG -= qt
OTHER_FILES += ffmpeg.sh

configureMake.input = OTHER_FILES
configureMake.output = $${INSTALLPREFIX}
configureMake.commands = bash ./${QMAKE_FILE_IN} $${PACKAGENAME} $${PACKAGEVERSION} '$${INSTALLPREFIX}'
configureMake.variable_out = FFMPEGLIBS
configureMake.CONFIG += target_predeps

QMAKE_EXTRA_COMPILERS += configureMake
