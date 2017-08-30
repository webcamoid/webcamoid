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
import sys
import platform

class Deploy:
    def __init__(self):
        self.rootDir = os.path.normpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), '../..'))
        self.arch = platform.architecture()[0]

        if os.name == 'posix':
            if sys.platform.startswith('darwin'):
                self.system = 'mac'
            else:
                self.system = 'posix'
        elif os.name == 'nt' and sys.platform.startswith('win32'):
            self.system = 'windows'
        else:
            self.system = 'unknown'

        fileName, ext = os.path.splitext(os.path.basename(__file__))
        self.platformDeploy = __import__('_'.join([fileName, self.system])) \
                                .Deploy(self.rootDir, self.system, self.arch)
        self.targetSystem = self.platformDeploy.targetSystem

        if self.targetSystem != self.system:
            self.platformDeploy = __import__('_'.join([fileName, self.system, self.targetSystem])) \
                                    .Deploy(self.rootDir, self.system, self.targetSystem, self.arch)

        self.scanPaths = self.platformDeploy.scanPaths
        self.targetArch = self.platformDeploy.targetArch
        self.programVersion = self.platformDeploy.programVersion
        self.qmake = self.platformDeploy.qmake

    def __str__(self):
        deployInfo = 'Root directory: {}\n' \
                     'System: {}\n' \
                     'Architecture: {}\n' \
                     'Target system: {}\n' \
                     'Target architecture: {}\n' \
                     'Scan paths: {}\n' \
                     'Program version: {}\n' \
                     'Qmake executable: {}'. \
                        format(self.rootDir,
                               self.system,
                               self.arch,
                               self.targetSystem,
                               self.targetArch,
                               self.scanPaths,
                               self.programVersion,
                               self.qmake)

        return deployInfo

    def prepare(self):
        print('\nPreparing for software packaging\n')
        self.platformDeploy.prepare()

    def solvedeps(self):
        self.platformDeploy.solvedeps()

    def package(self):
        self.platformDeploy.package()

    def finish(self):
        self.platformDeploy.finish()

deploy = Deploy()
print(deploy)
deploy.prepare()
deploy.solvedeps()
deploy.package()
deploy.finish()
