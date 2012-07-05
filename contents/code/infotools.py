#!/usr/bin/env python2
# -*- coding: utf-8 -*-
#
# Webcamod, Show and take Photos with your webcam.
# Copyright (C) 2011  Gonzalo Exequiel Pedone
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with This program. If not, see <http://www.gnu.org/licenses/>.
#
# Email   : hipersayan.x@gmail.com
# Web-Site: http://hipersayanx.blogspot.com/

# /etc/os-release
# http://www.freedesktop.org/software/systemd/man/os-release.html
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

import platform

from PyQt4 import QtCore


class InfoTools(QtCore.QObject):
    def __init__(self, parent=None):
        QtCore.QObject.__init__(self, parent)

        self.distName, self.distVersion, self.distId = self.parseOsRelease()
        self.distNameF, self.distVersionF, self.distIdF = self.forcedDetection()

    def gstInstallCommand(self):
        # gst-launch
        # /usr/bin/gst-launch-0.10

        # gstreamer
        # /usr/lib/gstreamer-0.10/libgstcoreelements.so

        # gst-plugins-base
        # /usr/lib/gstreamer-0.10/libgstalsa.so
        # /usr/lib/gstreamer-0.10/libgstaudioconvert.so
        # /usr/lib/gstreamer-0.10/libgstffmpegcolorspace.so
        # /usr/lib/gstreamer-0.10/libgsttheora.so
        # /usr/lib/gstreamer-0.10/libgstvorbis.so

        # gst-plugins-good
        # /usr/lib/gstreamer-0.10/libgsteffectv.so
        # /usr/lib/gstreamer-0.10/libgstvideo4linux2.so

        # gst-plugins-bad
        # /usr/lib/gstreamer-0.10/libgstgaudieffects.so
        # /usr/lib/gstreamer-0.10/libgstgeometrictransform.so
        # /usr/lib/gstreamer-0.10/libgstvp8.so

        # http://pkgs.org/
        # http://www.rpmseek.com
        # http://rpm.pbone.net/
        # http://www.debian.org/distrib/packages
        # http://packages.ubuntu.com/
        # http://packages.trisquel.info/

        # Arch/Chakra/Parabola
        cmd = 'pacman -S gstreamer0.10 gstreamer0.10-good gstreamer0.10-bad'

        # Debian/Ubuntu/LinuxMint/Trisquel
        cmd = 'apt-get install gstreamer0.10-tools gstreamer0.10-plugins-good gstreamer0.10-plugins-bad'

        # Fedora/CentOS
        cmd = 'yum install gstreamer gstreamer-plugins-good gstreamer-plugins-bad-free'

        # OpenSuSE
        cmd = 'zypper install gstreamer-0_10-utils gstreamer-0_10-plugins-good gstreamer-0_10-plugins-bad'

        # Mandriva/Mageia
        cmd = 'urpmi gstreamer0.10-tools gstreamer0.10-plugins-good gstreamer0.10-plugins-bad'

        return cmd

    def parseOsRelease(self):
        osInfo = {}

        try:
            with open('/etc/os-release') as osRelease:
                for line in osRelease:
                    try:
                        pair = line.split('=', 1)
                        osInfo[pair[0].strip().replace('"', '')] = pair[1].strip().replace('"', '')
                    except:
                        pass
        except:
            pass

        distName = osInfo['NAME'] if 'NAME' in osInfo else ''
        distVersion = osInfo['VERSION'] if 'VERSION' in osInfo else ''
        distId = osInfo['ID'] if 'ID' in osInfo else ''

        return (distName, distVersion, distId)

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
