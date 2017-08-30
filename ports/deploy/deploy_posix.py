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
        self.scanPaths = ['StandAlone/webcamoid']
        self.rootDir = rootDir
        self.system = system
        self.arch = arch
        self.targetArch = self.arch
        self.targetSystem = self.detectSystem()
        self.programVersion = self.readVersion()
        self.qmake = self.detectQmake()

    def detectSystem(self):
        exeFile = os.path.join(self.rootDir, self.scanPaths[0] + '.exe')

        return 'windows' if os.path.exists(exeFile) else self.system

    def readVersion(self):
        if self.targetSystem != 'posix':
            return ''

        os.environ['LD_LIBRARY_PATH'] = os.path.join(self.rootDir, 'libAvKys/Lib')
        programPath = os.path.join(self.rootDir, self.scanPaths[0])
        process = subprocess.Popen([programPath, '--version'],
                                   stdout=subprocess.PIPE)
        stdout, stderr = process.communicate()

        try:
            return stdout.split()[1].strip().decode('utf-8')
        except:
            return 'unknown'

    def detectQmake(self):
        with open(os.path.join(self.rootDir, 'StandAlone/Makefile')) as f:
            for line in f:
                if line.startswith('QMAKE'):
                    return line.split('=')[1].strip()

        return ''

    def prepare(self):
        process = subprocess.Popen([self.qmake, '-query', 'QT_INSTALL_QML'],
                                   stdout=subprocess.PIPE)
        stdout, stderr = process.communicate()
        self.sysQmlPath = stdout.strip().decode('utf-8')
        installDir = os.path.join(self.rootDir, 'ports/deploy/temp_priv/root')
        insQmlDir = os.path.join(installDir, self.sysQmlPath)
        dstQmlDir = os.path.join(installDir, 'usr/lib/qt/qml')

        previousDir = os.getcwd()
        os.chdir(self.rootDir)
        process = subprocess.Popen(['make', 'INSTALL_ROOT={}'.format(installDir), 'install'],
                                   stdout=subprocess.PIPE)
        process.communicate()
        os.chdir(previousDir)

        if process.returncode != 0:
            return

        if insQmlDir != dstQmlDir:
            if not os.path.exists(dstQmlDir):
                os.makedirs(dstQmlDir)

            try:
                shutil.copy(insQmlDir, dstQmlDir)
            except:
                pass

    def solvedeps(self):
        pass

    def package(self):
        pass

    def finish(self):
        pass
