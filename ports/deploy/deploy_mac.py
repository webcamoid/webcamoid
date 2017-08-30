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
        self.targetSystem = self.system
        self.appPath = os.path.join(self.rootDir, self.scanPaths[0] + '.app')
        self.programName = os.path.basename(self.scanPaths[0])
        self.programVersion = self.readVersion()
        self.qmake = self.detectQmake()
        self.optPath = '/usr/local/opt'

    def readVersion(self):
        os.environ['DYLD_LIBRARY_PATH'] = os.path.join(self.rootDir, 'libAvKys/Lib')
        programPath = os.path.join(self.appPath,
                                   'Contents/MacOS',
                                   self.programName)
        result = subprocess.run([programPath, '--version'],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)

        return result.stdout.split()[1].strip().decode('utf-8')

    def detectQmake(self):
        with open(os.path.join(self.rootDir, 'StandAlone/Makefile')) as f:
            for line in f:
                if line.startswith('QMAKE'):
                    return line.split('=')[1].strip()

        return ''

    def prepare(self):
        macdeploy = os.path.join(self.optPath, 'qt5/bin/macdeployqt')
        libDir = os.path.join(self.rootDir, 'libAvKys/Lib')
        tempDir = os.path.join(self.rootDir, 'ports/deploy/temp_priv')

        if not os.path.exists(tempDir):
            os.makedirs(tempDir)

        appPath = os.path.join(tempDir, os.path.basename(self.appPath))

        if not os.path.exists(appPath):
            shutil.copytree(self.appPath, appPath)

        result = subprocess.run([macdeploy,
                                 appPath,
                                 '-always-overwrite',
                                 '-appstore-compliant',
                                 '-qmldir={}'.format(self.rootDir),
                                 '-libpath={}'.format(libDir)],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)

        if result.returncode != 0:
            print(result.stderr.decode('utf-8'))

            return

        contents = os.path.join(appPath, 'Contents')
        installDir = os.path.join(self.rootDir, 'ports/deploy/temp_priv/root')

        previousDir = os.getcwd()
        os.chdir(os.path.join(self.rootDir, 'libAvkys'))
        result = subprocess.run(['make',
                                 'INSTALL_ROOT={}'.format(installDir),
                                 'install'],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        os.chdir(previousDir)

        if result.returncode != 0:
            print(result.stderr.decode('utf-8'))

            return

        qmlPath = os.path.join(contents, 'Resources/qml')

        if not os.path.exists(qmlPath):
            os.makedirs(qmlPath)

        try:
            shutil.copytree(os.path.join(installDir,
                                         'usr/lib/qt/qml/AkQml'),
                            os.path.join(qmlPath, 'AkQml'))
        except:
            pass

        pluginsPath = os.path.join(contents, 'Plugins')

        if not os.path.exists(pluginsPath):
            os.makedirs(pluginsPath)

        try:
            shutil.copytree(os.path.join(installDir,
                                         'usr/lib/avkys'),
                            os.path.join(pluginsPath, 'avkys'))
        except:
            pass

    def solvedeps(self):
        pass

    def package(self):
        pass

    def finish(self):
        pass
