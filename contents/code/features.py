#!/usr/bin/python2 -B
# -*- coding: utf-8 -*-
#
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
#
# Since February of 2012 the /etc/os-release file becomes the standard way of
# detecting the current working distribution. See more at this link:

import os
import sys

from PyQt4 import QtCore

import appenvironment


class Features(QtCore.QObject):
    def __init__(self, parent=None):
        QtCore.QObject.__init__(self, parent)

        self.appEnvironment = appenvironment.AppEnvironment(self)
        self.pluginsPath = '/usr/lib/gstreamer-0.10'
        self.extraPluginsPath = '/usr/lib/frei0r-1'

    def matrix(self):
        features = {}

        libAvailable = True

        # GStreamer Core:
        for lib in ['libgstcoreelements.so']:
            if not os.path.exists(os.path.join(self.pluginsPath, lib)):
                libAvailable = False

                break

        features['gst-core'] = [libAvailable, 'GStreamer Core', self.tr('Basic functionality.')]

        libAvailable = True

        # GStreamer Base Plugins:
        for lib in ['libgstalsa.so',
                    'libgstaudioconvert.so',
                    'libgstdecodebin2.so',
                    'libgstffmpegcolorspace.so',
                    'libgsttheora.so',
                    'libgstvideoscale.so',
                    'libgstvorbis.so']:
            if not os.path.exists(os.path.join(self.pluginsPath, lib)):
                libAvailable = False

                break

        features['gst-base-plugins'] = [libAvailable, 'GStreamer Base Plugins', self.tr('Transcoding and audio source.')]

        libAvailable = True

        # GStreamer Good Plugins:
        for lib in ['libgsteffectv.so',
                    'libgstvideo4linux2.so',
                    'libgstvideofilter.so',
                    'libgstximagesrc.so']:
            if not os.path.exists(os.path.join(self.pluginsPath, lib)):
                libAvailable = False

                break

        features['gst-good-plugins'] = [libAvailable, 'GStreamer Good Plugins', self.tr('Basic sources and effects.')]

        libAvailable = True

        # GStreamer Bad Plugins:
        for lib in ['libgstcoloreffects.so',
                    'libgstfrei0r.so',
                    'libgstgaudieffects.so',
                    'libgstgeometrictransform.so',
                    'libgstvp8.so']:
            if not os.path.exists(os.path.join(self.pluginsPath, lib)):
                libAvailable = False

                break

        features['gst-bad-plugins'] = [libAvailable, 'GStreamer Bad Plugins', self.tr('Effects and some codecs.')]

        libAvailable = True

        # frei0r Plugins:
        for lib in ['cartoon.so',
                    'delaygrab.so',
                    'distort0r.so',
                    'equaliz0r.so',
                    'hqdn3d.so',
                    'invert0r.so',
                    'nervous.so',
                    'pixeliz0r.so',
                    'primaries.so',
                    'sobel.so',
                    'sopsat.so',
                    'threelay0r.so',
                    'twolay0r.so']:
            if not os.path.exists(os.path.join(self.extraPluginsPath, lib)):
                libAvailable = False

                break

        features['frei0r-plugins'] = [libAvailable, 'frei0r Plugins', self.tr('Extra effects.')]

        return features

    def resolvePath(self, relpath=''):
        return os.path.normpath(os.path.join(os.path.
                                dirname(os.path.realpath(__file__)), relpath))


if __name__ == '__main__':
    app = QtCore.QCoreApplication(sys.argv)
    features = Features()
    print(features.matrix())
