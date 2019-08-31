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

import os
import sys
import platform
import shutil

import tools.utils

class DeployBase(tools.utils.DeployToolsUtils):
    def __init__(self):
        super().__init__()
        self.rootDir = os.path.normpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), '../..'))
        self.buildDir = os.environ['BUILD_PATH'] if 'BUILD_PATH' in os.environ else self.rootDir
        self.installDir = os.path.join(self.rootDir, 'ports/deploy/temp_priv/root')
        self.rootInstallDir = ''
        self.pkgsDir = os.path.join(self.rootDir,
                                    'ports/deploy/packages_auto',
                                    sys.platform if os.name == 'posix' else os.name)
        self.programVersion = ''
        self.qmake = ''

    def __str__(self):
        deployInfo = 'Python version: {}\n' \
                     'Root directory: {}\n' \
                     'Build directory: {}\n' \
                     'Install directory: {}\n' \
                     'Packages directory: {}\n' \
                     'System: {}\n' \
                     'Architecture: {}\n' \
                     'Target system: {}\n' \
                     'Target architecture: {}\n' \
                     'Number of threads: {}\n' \
                     'Program version: {}\n' \
                     'Make executable: {}\n' \
                     'Qmake executable: {}'. \
                        format(platform.python_version(),
                               self.rootDir,
                               self.buildDir,
                               self.installDir,
                               self.pkgsDir,
                               self.system,
                               self.arch,
                               self.targetSystem,
                               self.targetArch,
                               self.njobs,
                               self.programVersion,
                               self.make,
                               self.qmake)

        return deployInfo

    def run(self):
        print('Deploy info\n')
        print(self)
        print('\nPreparing for software packaging\n')
        self.prepare()

        if not 'NO_SHOW_PKG_DATA_INFO' in os.environ \
            or os.environ['NO_SHOW_PKG_DATA_INFO'] != '1':
            print('\nPackaged data info\n')
            self.printPackageDataInfo()

        if 'PACKAGES_PREPARE_ONLY' in os.environ \
            and os.environ['PACKAGES_PREPARE_ONLY'] == '1':
            print('\nPackage data is ready for merging\n')
        else:
            print('\nCreating packages\n')
            self.package()
            print('\nCleaning up')
            self.cleanup()
            print('Deploy finnished\n')

    def printPackageDataInfo(self):
        packagedFiles = []

        for root, _, files in os.walk(self.rootInstallDir):
            for f in files:
                packagedFiles.append(os.path.join(root, f))

        packagedFiles = sorted(packagedFiles)

        for f in packagedFiles:
            print('    ' + f)

    def prepare(self):
        pass

    def package(self):
        pass

    def cleanup(self):
        shutil.rmtree(self.installDir, True)
