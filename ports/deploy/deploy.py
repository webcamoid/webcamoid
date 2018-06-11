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

import tools.utils

class Deploy(tools.utils.DeployToolsUtils):
    def __init__(self):
        super().__init__()
        self.rootDir = os.path.normpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), '../..'))
        self.buildDir = os.environ['BUILD_PATH'] if 'BUILD_PATH' in os.environ else self.rootDir
        self.installDir = os.path.join(self.rootDir, 'ports/deploy/temp_priv/root')
        self.pkgsDir = os.path.join(self.rootDir,
                                    'ports/deploy/packages_auto',
                                    sys.platform if os.name == 'posix' else os.name)
        self.arch = platform.architecture()[0]
        self.targetArch = self.arch
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

    def load(self, system=''):
        module = __import__('deploy_' + (self.system if system == '' else system))

        if not module:
            return None

        deploy = module.Deploy()

        if deploy.targetSystem != system:
            return self.load(deploy.targetSystem)

        return deploy

    def run(self):
        print('Deploy info\n')
        print(self)
        print('\nPreparing for software packaging\n')
        self.prepare()
        print('\nSolving package dependencies\n')
        self.solvedeps()
        #print('\nFinnishing preparation\n')
        #self.finish()
        #print('\nCreating packages\n')
        #self.package()
        #print('\nCleaningup...\n')
        #self.cleanup()
        #print('\nDeploy finnished\n')

    def redirect(self):
        return ''

    def prepare(self):
        pass

    def solvedeps(self):
        pass

    def finish(self):
        pass

    def package(self):
        pass

    def cleanup(self):
        pass

if __name__ =='__main__':
    deploy = Deploy().load()

    if deploy:
        deploy.run()
    else:
        print('No valid deploy script found.')
