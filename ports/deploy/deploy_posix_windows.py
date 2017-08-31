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
    def __init__(self, rootDir, system, targetSystem, arch):
        self.scanPaths = ['StandAlone/webcamoid.exe']
        self.rootDir = rootDir
        self.system = system
        self.arch = arch
        self.targetSystem = targetSystem
        self.targetArch = self.detectArch()
        self.qmake = self.detectQmake()
        self.programVersion = self.readVersion()

    def detectArch(self):
        exeFile = os.path.join(self.rootDir, self.scanPaths[0])
        process = subprocess.Popen(['file', '-b', exeFile],
                                   stdout=subprocess.PIPE)
        stdout, stderr = process.communicate()

        return '64bit' if b'x86-64' in stdout else '32bit'

    def readVersion(self):
        tempDir = os.path.join(self.rootDir, 'ports/deploy/temp_priv')
        wineRootDir = 'Z:{}'.format(self.rootDir).replace('/', '\\')
        wineTempDir = 'Z:{}'.format(tempDir).replace('/', '\\')

        if not os.path.exists(tempDir):
            os.makedirs(tempDir)

        process = subprocess.Popen([self.qmake, '-query', 'QT_INSTALL_BINS'],
                                   stdout=subprocess.PIPE)
        stdout, stderr = process.communicate()
        where = 'Z:{}'.format(stdout.strip().decode('utf-8').replace('/', '\\'))
        versionBat = os.path.join(tempDir, 'version.bat')
        programName = os.path.basename(self.scanPaths[0]).replace('.exe', '')

        with open(versionBat, 'w') as f:
            f.write('@echo off\n')
            f.write('SET PATH={0}\\StandAlone;{0}\\libAvKys\Lib;{1};%PATH%\n'.format(wineRootDir, where))
            f.write('@echo on\n')
            f.write('{0} --version > "{1}\\version.txt"\n'.format(programName, wineTempDir))

        subprocess.run(['wineconsole', versionBat], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        with open(os.path.join(tempDir, 'version.txt')) as f:
            version = f.readline();

        shutil.rmtree(tempDir)

        try:
            return version.split()[1].strip()
        except:
            return 'unknown'

    def detectQmake(self):
        with open(os.path.join(self.rootDir, 'StandAlone/Makefile')) as f:
            for line in f:
                if line.startswith('QMAKE'):
                    return line.split('=')[1].strip()

        return ''

    def prepare(self):
        installDir = os.path.join(self.rootDir, 'ports/deploy/temp_priv/root')
        previousDir = os.getcwd()
        os.chdir(self.rootDir)
        process = subprocess.Popen(['make', 'INSTALL_ROOT={}'.format(installDir), 'install'],
                                   stdout=subprocess.PIPE)
        process.communicate()
        os.chdir(previousDir)

        if process.returncode != 0:
            return

        binPath = os.path.join(installDir, 'webcamoid/bin')

        if os.path.exists(binPath):
            files = [directory for directory in os.listdir(binPath) if directory.endswith('.a')]

            for f in files:
                os.remove(os.path.join(binPath, f))

    def solvedeps(self):
        #cp -vf ${SYSDIR}/bin/libeay32.dll "${ROOTDIR}/build/bundle-data/${APPNAME}/bin/"
        #cp -vf ${SYSDIR}/bin/ssleay32.dll "${ROOTDIR}/build/bundle-data/${APPNAME}/bin/"
        #cp -vf ${SYSDIR}/bin/libEGL.dll "${ROOTDIR}/build/bundle-data/${APPNAME}/bin/"
        #cp -vf ${SYSDIR}/bin/libGLESv2.dll "${ROOTDIR}/build/bundle-data/${APPNAME}/bin/"
        #cp -vf ${SYSDIR}/bin/D3DCompiler_*.dll "${ROOTDIR}/build/bundle-data/${APPNAME}/bin/"
        pass

    def package(self):
        pass

    def finish(self):
        pass
