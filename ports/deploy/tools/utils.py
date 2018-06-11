#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Webcamoid, webcam capture application.
# Copyright (C) 2017  Gonzalo Exequiel Pedone
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

import fnmatch
import multiprocessing
import os
import subprocess
import sys

class DeployToolsUtils:
    def __init__(self):
        self.make = ''

        if os.name == 'posix' and sys.platform.startswith('darwin'):
            self.system = 'mac'
        elif os.name == 'nt' and sys.platform.startswith('win32'):
            self.system = 'windows'
        elif os.name == 'posix':
            self.system = 'posix'
        else:
            self.system = ''

        self.targetSystem = self.system
        pathSep = ';' if self.system == 'windows' else ':'
        self.sysBinsPath = []

        if 'PATH' in os.environ:
            self.sysBinsPath = \
                [path.strip() for path in os.environ['PATH'].split(pathSep)]

        self.njobs = multiprocessing.cpu_count()

        if self.njobs < 4:
            self.njobs = 4

    def whereBin(self, binary):
        for path in self.sysBinsPath:
            path = os.path.join(path, binary)

            if os.path.exists(path):
                return path

        return ''

    def copy(self, src, dst):
        if not os.path.exists(src):
            return

        basename = os.path.basename(src)
        dstpath = os.path.join(dst, basename) if os.path.isdir(dst) else dst
        dstdir = dst if os.path.isdir(dst) else os.path.dirname(dst)

        if os.path.islink(src):
            rsrc = os.path.realpath(src)
            rbasename = os.path.basename(rsrc)
            rdstpath = os.path.join(dstdir, rbasename)

            if not os.path.exists(rdstpath):
                shutil.copy(rsrc, rdstpath)

            if not os.path.exists(dstpath):
                os.symlink(os.path.join('.', rbasename), dstpath)
        elif not os.path.exists(dstpath):
            shutil.copy(src, dst)

    def detectMake(self):
        if 'MAKE_PATH' in os.environ:
            self.make = os.environ['MAKE_PATH']

            return

        makes = ['mingw32-make', 'make'] if self.system == 'windows' else ['make']
        ext = '.exe' if self.system == 'windows' else ''

        for make in makes:
            makePath = self.whereBin(make + ext)

            if makePath != '':
                self.make = makePath

                break

    def makeInstall(self, buildDir, installRoot=''):
        previousDir = os.getcwd()
        os.chdir(buildDir)

        if installRoot == '':
            process = subprocess.Popen([self.make, 'install'],
                                       stdout=subprocess.PIPE)
        else:
            process = subprocess.Popen([self.make, 'INSTALL_ROOT=' + installRoot, 'install'],
                                    stdout=subprocess.PIPE)

        process.communicate()
        os.chdir(previousDir)

        return process.returncode
