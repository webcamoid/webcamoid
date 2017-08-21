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

TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS = src
CONFIG(config_dshow): SUBDIRS += src/dshow
CONFIG(config_ffmpeg): SUBDIRS += src/ffmpeg
CONFIG(config_gstreamer): SUBDIRS += src/gstreamer
CONFIG(config_syphon): SUBDIRS += src/syphonout
CONFIG(config_v4l2): SUBDIRS += src/v4l2sys
CONFIG(config_v4lutils): SUBDIRS += src/v4lutils
