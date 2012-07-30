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
#
# http://www.freedesktop.org/software/systemd/man/os-release.html
#
# Some common distros ID are or could be:
#
# ID=arch
# ID=chakra
# ID=parabola
# ID=debian
# ID=ubuntu
# ID=fedora
# ID=centos
# ID=mageia
# ID=mandriva
# ID=opensuse
#
# If you want to know what are the dependencies of Webcamoid you must
# find wich packages contains these files:
#
# gst-launch:
#
# /usr/bin/gst-launch-0.10
#
# gstreamer:
#
# /usr/lib/gstreamer-0.10/libgstcoreelements.so
#
# gst-plugins-base:
#
# /usr/lib/gstreamer-0.10/libgstalsa.so
# /usr/lib/gstreamer-0.10/libgstaudioconvert.so
# /usr/lib/gstreamer-0.10/libgstffmpegcolorspace.so
# /usr/lib/gstreamer-0.10/libgsttheora.so
# /usr/lib/gstreamer-0.10/libgstvorbis.so
#
# gst-plugins-good:
#
# /usr/lib/gstreamer-0.10/libgsteffectv.so
# /usr/lib/gstreamer-0.10/libgstvideo4linux2.so
#
# gst-plugins-bad:
#
# /usr/lib/gstreamer-0.10/libgstgaudieffects.so
# /usr/lib/gstreamer-0.10/libgstgeometrictransform.so
# /usr/lib/gstreamer-0.10/libgstvp8.so
#
# These links are useful to know what package contains what file:
#
# http://pkgs.org/
# http://www.rpmseek.com
# http://rpm.pbone.net/
# http://www.debian.org/distrib/packages
# http://packages.ubuntu.com/
# http://packages.trisquel.info/

import platform

from PyQt4 import QtCore


class InfoTools(QtCore.QObject):
    def __init__(self, parent=None):
        QtCore.QObject.__init__(self, parent)

        self.distName, self.distVersion, self.distId = self.parseOsRelease()
        self.distNameF, self.distVersionF, self.distIdF = self.forcedDetection()

    def gstInstallCommand(self):
        isCmd = True

        if self.distId == 'arch' or \
           self.distId == 'chakra' or \
           self.distId == 'parabola':
            # Arch/Chakra/Parabola
            cmd = 'pacman -S gstreamer0.10 gstreamer0.10-good gstreamer0.10-bad'
        elif self.distId == 'debian' or \
             self.distNameF == 'Ubuntu':
            # Debian/Ubuntu
            cmd = 'apt-get install gstreamer0.10-tools '\
                  'gstreamer0.10-plugins-good gstreamer0.10-plugins-bad'
        elif self.distId == 'fedora' or \
             self.distNameF == 'CentOS':
            # Fedora/CentOS
            cmd = 'yum install gstreamer gstreamer-plugins-good '\
                  'gstreamer-plugins-bad-free'
        elif self.distId == 'opensuse':
            # OpenSuSE
            cmd = 'zypper install gstreamer-0_10-utils '\
                  'gstreamer-0_10-plugins-good gstreamer-0_10-plugins-bad'
        elif self.distId == 'mandriva' or \
             self.distId == 'mageia':
            # Mandriva/Mageia
            cmd = 'urpmi gstreamer0.10-tools gstreamer0.10-plugins-good '\
                  'gstreamer0.10-plugins-bad'
        else:
            cmd = 'gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad'
            isCmd = False

        return cmd, isCmd

    # For newest distributions.
    def parseOsRelease(self):
        osInfo = {}

        try:
            with open('/etc/os-release') as osRelease:
                for line in osRelease:
                    try:
                        pair = line.split('=', 1)
                        osInfo[pair[0].strip().replace('"', '')] = \
                                                pair[1].strip().replace('"', '')
                    except:
                        pass
        except:
            pass

        distName = osInfo['NAME'] if 'NAME' in osInfo else ''
        distVersion = osInfo['VERSION'] if 'VERSION' in osInfo else ''
        distId = osInfo['ID'] if 'ID' in osInfo else ''

        return (distName, distVersion, distId)

    # For oldest distributions.
    def forcedDetection(self):
        return platform. \
            linux_distribution(supported_dists=('SuSE',
                                                'UnitedLinux',
                                                'annvix',
                                                'arch',
                                                'arklinux',
                                                'aurox',
                                                'blackcat',
                                                'centos',
                                                'cobalt',
                                                'conectiva',
                                                'debian',
                                                'e-smith',
                                                'fedora',
                                                'gentoo',
                                                'immunix',
                                                'knoppix',
                                                'lfs',
                                                'linuxppc',
                                                'lsb',
                                                'mandakelinux',
                                                'mandrake',
                                                'mandriva',
                                                'mklinux',
                                                'nld',
                                                'novell',
                                                'pld',
                                                'redhat',
                                                'rocks',
                                                'slackware',
                                                'sles',
                                                'tinysofa',
                                                'turbolinux',
                                                'ultrapenguin',
                                                'va',
                                                'yellowdog'))


if __name__ == '__main__':
    it = InfoTools()

    print([it.distName, it.distVersion, it.distId])
    print([it.distNameF, it.distVersionF, it.distIdF])
