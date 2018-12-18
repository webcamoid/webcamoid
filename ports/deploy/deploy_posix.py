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

import configparser
import math
import os
import platform
import subprocess
import sys
import tarfile
import threading

import deploy_base
import tools.binary_elf
import tools.qt5


class Deploy(deploy_base.DeployBase, tools.qt5.DeployToolsQt):
    def __init__(self):
        super().__init__()
        self.installDir = os.path.join(self.rootDir, 'ports/deploy/temp_priv')
        self.pkgsDir = os.path.join(self.rootDir, 'ports/deploy/packages_auto', sys.platform)
        self.rootInstallDir = os.path.join(self.installDir, 'usr')
        self.binaryInstallDir = os.path.join(self.rootInstallDir, 'bin')
        self.libInstallDir = os.path.join(self.rootInstallDir, 'lib')
        self.libQtInstallDir = os.path.join(self.libInstallDir, 'qt')
        self.qmlInstallDir = os.path.join(self.libQtInstallDir, 'qml')
        self.pluginsInstallDir = os.path.join(self.libQtInstallDir, 'plugins')
        self.qtConf = os.path.join(self.binaryInstallDir, 'qt.conf')
        self.qmlRootDirs = ['StandAlone/share/qml', 'libAvKys/Plugins']
        self.mainBinary = os.path.join(self.binaryInstallDir, 'webcamoid')
        self.programName = os.path.basename(self.mainBinary)
        self.detectQt(os.path.join(self.buildDir, 'StandAlone'))
        self.programVersion = self.detectVersion(os.path.join(self.rootDir, 'commons.pri'))
        self.detectMake()
        self.targetSystem = 'posix_windows' if 'win32' in self.qmakeQuery(var='QMAKE_XSPEC') else 'posix'
        self.binarySolver = tools.binary_elf.DeployToolsBinary()
        self.binarySolver.readExcludeList(os.path.join(self.rootDir, 'ports/deploy/exclude.{}.{}.txt'.format(os.name, sys.platform)))
        self.appImage = self.detectAppImage()
        self.packageConfig = os.path.join(self.rootDir, 'ports/deploy/package_info.conf')
        self.dependencies = []
        self.installerConfig = os.path.join(self.installDir, 'installer/config')
        self.installerPackages = os.path.join(self.installDir, 'installer/packages')
        self.installerIconSize = 128
        self.appIcon = os.path.join(self.installDir,
                                    'usr/share/icons/hicolor/{1}x{1}/apps/{0}.png'.format(self.programName,
                                                                                          self.installerIconSize))
        self.licenseFile = os.path.join(self.rootDir, 'COPYING')
        self.installerRunProgram = '@TargetDir@/' + self.programName + '.sh'
        self.installerTargetDir = '@HomeDir@/' + self.programName
        self.installerScript = os.path.join(self.rootDir, 'ports/deploy/installscript.posix.qs')
        self.changeLog = os.path.join(self.rootDir, 'ChangeLog')
        self.outPackage = os.path.join(self.pkgsDir,
                                       'webcamoid-installer-{}-{}.run'.format(self.programVersion,
                                                                              platform.machine()))

    def detectAppImage(self):
        if 'APPIMAGETOOL' in os.environ:
            return os.environ['APPIMAGETOOL']

        return self.whereBin('appimagetool')

    def solvedepsLibs(self):
        for dep in self.binarySolver.scanDependencies(self.installDir):
            depPath = os.path.join(self.libInstallDir, os.path.basename(dep))
            print('    {} -> {}'.format(dep, depPath))
            self.copy(dep, depPath, True)
            self.dependencies.append(dep)

    def prepare(self):
        print('Executing make install')
        self.makeInstall(self.buildDir, self.installDir)
        self.detectTargetArch()
        print('Copying Qml modules\n')
        self.solvedepsQml()
        print('\nCopying required plugins\n')
        self.solvedepsPlugins()
        print('\nCopying required libs\n')
        self.solvedepsLibs()
        print('\nWritting qt.conf file')
        self.writeQtConf()
        print('Stripping symbols')
        self.binarySolver.stripSymbols(self.installDir)
        print('Resetting file permissions')
        self.binarySolver.resetFilePermissions(self.rootInstallDir,
                                               self.binaryInstallDir)
        print('Writting launcher file')
        self.createLauncher()
        print('\nWritting build system information\n')
        self.writeBuildInfo()

    def searchPackageFor(self, path):
        os.environ['LC_ALL'] = 'C'
        pacman = self.whereBin('pacman')

        if len(pacman) > 0:
            process = subprocess.Popen([pacman, '-Qo', path],
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE)
            stdout, stderr = process.communicate()

            if process.returncode != 0:
                return ''

            info = stdout.decode(sys.getdefaultencoding()).split(' ')

            if len(info) < 2:
                return ''

            package, version = info[-2:]

            return ' '.join([package.strip(), version.strip()])

        dpkg = self.whereBin('dpkg')

        if len(dpkg) > 0:
            process = subprocess.Popen([dpkg, '-S', path],
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE)
            stdout, stderr = process.communicate()

            if process.returncode != 0:
                return ''

            package = stdout.split(b':')[0].decode(sys.getdefaultencoding()).strip()

            process = subprocess.Popen([dpkg, '-s', package],
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE)
            stdout, stderr = process.communicate()

            if process.returncode != 0:
                return ''

            for line in stdout.decode(sys.getdefaultencoding()).split('\n'):
                line = line.strip()

                if line.startswith('Version:'):
                    return ' '.join([package, line.split()[1].strip()])

            return ''

        rpm = self.whereBin('rpm')

        if len(rpm) > 0:
            process = subprocess.Popen([rpm, '-qf', path],
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE)
            stdout, stderr = process.communicate()

            if process.returncode != 0:
                return ''

            return stdout.decode(sys.getdefaultencoding()).strip()

        return ''

    def sysInfo(self):
        info = ''

        for f in os.listdir('/etc'):
            if f.endswith('-release'):
                with open(os.path.join('/etc' , f)) as releaseFile:
                    info += releaseFile.read()

        return info

    def writeBuildInfo(self):
        depsInfoFile = os.path.join(self.rootInstallDir, 'share/build-info.txt')
        info = self.sysInfo()

        with open(depsInfoFile, 'w') as f:
            for line in info.split('\n'):
                if len(line) > 0:
                    print('    ' + line)
                    f.write(line + '\n')

            print()
            f.write('\n')

        packages = set()

        for dep in self.dependencies:
            packageInfo = self.searchPackageFor(dep)

            if len(packageInfo) > 0:
                packages.add(packageInfo)

        packages = sorted(packages)

        with open(depsInfoFile, 'a') as f:
            for packge in packages:
                print('    ' + packge)
                f.write(packge + '\n')

    def createLauncher(self):
        path = os.path.join(self.rootInstallDir, self.programName) + '.sh'

        with open(path, 'w') as launcher:
            launcher.write('#!/bin/sh\n')
            launcher.write('\n')
            launcher.write('rootdir() {\n')
            launcher.write('    case "$1" in\n')
            launcher.write('        /*) dirname "$1"\n')
            launcher.write('            ;;\n')
            launcher.write('        *)  dir=$(dirname "$PWD/$1")\n')
            launcher.write('            cwd=$PWD\n')
            launcher.write('            cd "$dir" 1>/dev/null\n')
            launcher.write('                echo $PWD\n')
            launcher.write('            cd "$cwd" 1>/dev/null\n')
            launcher.write('            ;;\n')
            launcher.write('    esac\n')
            launcher.write('}\n')
            launcher.write('\n')
            launcher.write('ROOTDIR=$(rootdir "$0")\n')
            launcher.write('export PATH="${ROOTDIR}/bin:$PATH"\n')
            launcher.write('export LD_LIBRARY_PATH="${ROOTDIR}/lib:$LD_LIBRARY_PATH"\n')
            launcher.write('#export QT_DEBUG_PLUGINS=1\n')
            launcher.write('{} "$@"\n'.format(self.programName))

        os.chmod(path, 0o744)

    def hrSize(self, size):
        i = int(math.log(size) // math.log(1024))

        if i < 1:
            return '{} B'.format(size)

        units = ['KiB', 'MiB', 'GiB', 'TiB']
        sizeKiB = size / (1024 ** i)

        return '{:.2f} {}'.format(sizeKiB, units[i - 1])

    def printPackageInfo(self, path):
        if os.path.exists(path):
            print('   ',
                  os.path.basename(path),
                  self.hrSize(os.path.getsize(path)))
        else:
            print('   ',
                  os.path.basename(path),
                  'FAILED')

    def createPortable(self, mutex):
        packagePath = \
            os.path.join(self.pkgsDir,
                         '{}-portable-{}-{}.tar.xz'.format(self.programName,
                                                           self.programVersion,
                                                           platform.machine()))

        if not os.path.exists(self.pkgsDir):
            os.makedirs(self.pkgsDir)

        with tarfile.open(packagePath, 'w:xz') as tar:
            tar.add(self.rootInstallDir, self.programName)

        mutex.acquire()
        print('Created portable package:')
        self.printPackageInfo(packagePath)
        mutex.release()

    def createAppInstaller(self, mutex):
        packagePath = self.createInstaller()

        if not packagePath:
            return

        mutex.acquire()
        print('Created installable package:')
        self.printPackageInfo(self.outPackage)
        mutex.release()

    def createAppImage(self, mutex):
        if not os.path.exists(self.appImage):
            return

        appDir = \
            os.path.join(self.installDir,
                         '{}-{}-{}.AppDir'.format(self.programName,
                                                  self.programVersion,
                                                  platform.machine()))

        self.copy(self.rootInstallDir, appDir)
        launcher = os.path.join(appDir, 'AppRun')

        if not os.path.exists(launcher):
            os.replace(os.path.join(appDir, 'webcamoid.sh'), launcher)

        desktopFile = os.path.join(appDir, 'webcamoid.desktop')

        if os.path.exists(desktopFile):
            os.remove(desktopFile)

        self.copy(os.path.join(appDir, 'share/applications/webcamoid.desktop'), desktopFile)
        config = configparser.ConfigParser()
        config.optionxform=str
        config.read(desktopFile, 'utf-8')
        config['Desktop Entry']['Exec'] = 'AppRun'

        with open(desktopFile, 'w', encoding='utf-8') as configFile:
            config.write(configFile, space_around_delimiters=False)

        icon = os.path.join(appDir, 'webcamoid.png')

        if not os.path.exists(icon):
            os.symlink('./share/icons/hicolor/256x256/apps/webcamoid.png',
                       icon)

        # Remove old file
        packagePath = \
            os.path.join(self.pkgsDir,
                         '{}-{}-{}.AppImage'.format(self.programName,
                                                    self.programVersion,
                                                    platform.machine()))

        if not os.path.exists(self.pkgsDir):
            os.makedirs(self.pkgsDir)

        if os.path.exists(packagePath):
            os.remove(packagePath)

        penv = os.environ.copy()
        penv['ARCH'] = platform.machine()

        libPaths = []

        if 'LD_LIBRARY_PATH' in penv:
            libPaths = penv['LD_LIBRARY_PATH'].split(':')

        penv['LD_LIBRARY_PATH'] = \
            ':'.join([os.path.abspath(os.path.join(self.appImage, '../../lib'))]
                     + libPaths)

        process = subprocess.Popen([self.appImage,
                                    '-v',
                                    '--no-appstream',
                                    '--comp', 'xz',
                                    appDir,
                                    packagePath],
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE,
                                    env=penv)
        process.communicate()

        mutex.acquire()
        print('Created AppImage package:')
        self.printPackageInfo(packagePath)
        mutex.release()

    def package(self):
        mutex = threading.Lock()

        threads = [threading.Thread(target=self.createPortable, args=(mutex,))]

        if self.qtIFW != '':
            threads.append(threading.Thread(target=self.createAppInstaller, args=(mutex,)))

        if self.appImage != '':
            threads.append(threading.Thread(target=self.createAppImage, args=(mutex,)))

        for thread in threads:
            thread.start()

        for thread in threads:
            thread.join()
