#!/usr/bin/env python
# -*- coding: utf-8 -*-

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

import os
import platform
import subprocess
import shutil

class Deploy:
    def __init__(self, rootDir, system, arch):
        self.scanPaths = ['StandAlone\\webcamoid.exe']
        self.rootDir = rootDir
        self.buildDir = os.environ['BUILD_DIR'] if 'BUILD_DIR' in os.environ else rootDir
        self.system = system
        self.arch = arch
        self.targetSystem = system
        self.targetArch = self.detectArch()
        self.qmake = self.detectQmake()
        self.programVersion = self.readVersion()
        self.make = self.detectMake()

    def detectArch(self):
        exeFile = os.path.join(self.buildDir, self.scanPaths[0])

        return platform.architecture(exeFile)[0]

    def readVersion(self):
        process = subprocess.Popen([self.qmake, '-query', 'QT_INSTALL_BINS'], stdout=subprocess.PIPE)
        stdout, stderr = process.communicate()
        os.environ['PATH'] = ';'.join([os.path.join(self.buildDir, 'libAvKys\\Lib'),
                                       stdout.strip().decode('utf-8'),
                                       os.environ['PATH']])
        programPath = os.path.join(self.buildDir, self.scanPaths[0])
        process = subprocess.Popen([programPath, '--version'],
                                   stdout=subprocess.PIPE)
        stdout, stderr = process.communicate()

        try:
            return stdout.split()[1].strip().decode('utf-8')
        except:
            return 'unknown'

    def detectQmake(self):
        with open(os.path.join(self.buildDir, 'StandAlone\\Makefile')) as f:
            for line in f:
                if line.startswith('QMAKE'):
                    return line.split('=')[1].strip()

        return ''

    def detectMake(self):
        if 'MAKE_PATH' in os.environ:
            return os.environ['MAKE_PATH']

        defaultPath = os.path.dirname(self.qmake)
        makePaths = [os.path.join(defaultPath, 'make.exe'),
                     'C:\\MinGW\\bin\\mingw32-make.exe']

        for path in makePaths:
            if os.path.exists(path):
                return path

        return ''

    def prepare(self):
        print(os.environ['PATH'])

        installDir = os.path.join(self.buildDir, 'ports\\deploy\\temp_priv\\root')
        previousDir = os.getcwd()
        os.chdir(self.buildDir)
        process = subprocess.Popen([self.make, 'INSTALL_ROOT={}'.format(installDir), 'install'],
                                   stdout=subprocess.PIPE)
        process.communicate()
        os.chdir(previousDir)

        if process.returncode != 0:
            return

        binPath = os.path.join(installDir, 'webcamoid\\bin')
        files = [directory for directory in os.listdir(binPath) if directory.endswith('.a')]

        for f in files:
            os.remove(os.path.join(binPath, f))

    def solvedeps(self):
        pass

    def package(self):
        pass

    def finish(self):
        pass
