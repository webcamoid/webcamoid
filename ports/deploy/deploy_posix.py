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
import fnmatch
import math
import multiprocessing
import os
import platform
import re
import shutil
import struct
import subprocess
import sys
import tarfile
import threading
import time

import deploy
import tools.binary_elf
import tools.qt5


class Deploy(deploy.Deploy, tools.qt5.DeployToolsQt):
    def __init__(self):
        super().__init__()
        self.installDir = os.path.join(self.rootDir, 'ports/deploy/temp_priv/root')
        self.pkgsDir = os.path.join(self.rootDir, 'ports/deploy/packages_auto', sys.platform)
        self.qmlInstallDir = os.path.join(self.installDir, 'usr/lib/qt/qml')
        self.pluginsInstallDir = os.path.join(self.installDir, 'usr/lib/qt/plugins')
        self.qmlRootDirs = ['StandAlone/share/qml',
                            'libAvKys/Plugins']
        self.detectQt(os.path.join(self.buildDir, 'StandAlone'))
        self.programVersion = self.detectVersion(os.path.join(self.rootDir, 'commons.pri'))
        self.detectMake()
        self.targetSystem = 'posix_windows' if self.qmakeQuery(var='QMAKE_XSPEC') == 'win32' else 'posix'
        self.binarySolver = tools.binary_elf.DeployToolsBinary()
        self.appImage = self.detectAppImage()
        self.excludes = self.readExcludeList()
        self.dependencies = []
        self.installerIconSize = 128

    def detectAppImage(self):
        if 'APPIMAGETOOL' in os.environ:
            return os.environ['APPIMAGETOOL']

        return self.whereBin('appimagetool')

    def prepare(self):
        self.makeInstall(self.buildDir, self.installDir)

    def readExcludeList(self):
        excludeFile = 'exclude.{}.{}.txt'.format(os.name, sys.platform)
        excludePath = os.path.join(self.rootDir, 'ports/deploy', excludeFile)
        excludes = []

        if os.path.exists(excludePath):
            with open(excludePath) as f:
                for line in f:
                    line = line.strip()

                    if len(line) > 0 and line[0] != '#':
                        i = line.find('#')

                        if i >= 0:
                            line = line[: i]

                        line = line.strip()

                        if len(line) > 0:
                            excludes.append(line)

        return excludes

    def isExcluded(self, path):
        for exclude in self.excludes:
            if re.search(exclude, path):
                return True

        return False

    def solvedepsLibs(self):
        print('\nCopying required libs\n')
        deps = set()

        for elfPath in self.findElfs(self.installDir):
            for dep in self.listDependencies(elfPath):
                deps.add(dep)

        _deps = set()

        for dep in deps:
            if not self.isExcluded(dep):
                _deps.add(dep)

        solved = set()

        while len(_deps) > 0:
            dep = _deps.pop()
            libPath = os.path.join(self.installDir, 'usr/lib', os.path.basename(dep))
            print('    {} -> {}'.format(dep, libPath))

            self.copy(dep, libPath)

            for elfDep in self.listDependencies(dep):
                if elfDep != dep \
                   and not elfDep in solved \
                   and not self.isExcluded(elfDep):
                    _deps.add(elfDep)

            solved.add(dep)
            self.dependencies.append(dep)

    def solvedeps(self):
        self.solvedepsQml()
        self.solvedepsPlugins()
        self.solvedepsLibs()

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
        print('\nWritting build system information\n')

        depsInfoFile = os.path.join(self.installDir, 'usr/share/build-info.txt')
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

    def writeQtConf(self):
        print('Writting qt.conf file')

        paths = {'Plugins': '../lib/qt/plugins',
                 'Imports': '../lib/qt/qml',
                 'Qml2Imports': '../lib/qt/qml'}

        with open(os.path.join(self.installDir, 'usr/bin/qt.conf'), 'w') as qtconf:
            qtconf.write('[Paths]\n')

            for path in paths:
                qtconf.write('{} = {}\n'.format(path, paths[path]))

    def resetFilePermissions(self):
        print('Resetting file permissions')
        rootPath = os.path.join(self.installDir, 'usr')
        binariesPath = os.path.join(rootPath, 'bin')

        for root, dirs, files in os.walk(rootPath):
            for d in dirs:
                os.chmod(os.path.join(root, d), 0o755)

            for f in files:
                permissions = 0o644
                path = os.path.join(root, f)

                if root == binariesPath and self.isElf(path):
                    permissions = 0o744

                os.chmod(path, permissions)

    def createLauncher(self):
        print('Writting launcher file')
        path = os.path.join(self.installDir, 'usr/webcamoid')

        with open(path + '.sh', 'w') as launcher:
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
            launcher.write('webcamoid "$@"\n')

        os.chmod(path + '.sh', 0o744)

    def finish(self):
        print('\nCompleting final package structure\n')
        self.writeQtConf()
        print('Stripping symbols')
        self.stripSymbols()
        self.resetFilePermissions()
        self.createLauncher()
        self.writeBuildInfo()

    def hrSize(self, size):
        i = int(math.log(size) // math.log(1024))

        if i < 1:
            return '{} B'.format(size)

        units = ['KiB', 'MiB', 'GiB', 'TiB']
        sizeKiB = size / (1024 ** i)

        return '{:.2f} {}'.format(sizeKiB, units[i - 1])

    def printPackageInfo(self, path):
        print('   ', os.path.basename(path),
              self.hrSize(os.path.getsize(path)))

    def createPortable(self, mutex):
        path = os.path.join(self.installDir, 'usr')
        packagePath = \
            os.path.join(self.pkgsDir,
                         'webcamoid-portable-{}-{}.tar.xz'.format(self.programVersion,
                                                                  platform.machine()))

        if not os.path.exists(self.pkgsDir):
            os.makedirs(self.pkgsDir)

        with tarfile.open(packagePath, 'w:xz') as tar:
            tar.add(path, 'webcamoid')

        mutex.acquire()
        print('Created portable package:')
        self.printPackageInfo(packagePath)
        mutex.release()

    def readChangeLog(self, changeLog, version):
        if os.path.exists(changeLog):
            with open(changeLog) as f:
                for line in f:
                    if not line.startswith('Webcamoid {}:'.format(version)):
                        continue

                    # Skip first line.
                    f.readline()
                    changeLogText = ''

                    for line in f:
                        if re.match('Webcamoid \d+\.\d+\.\d+:', line):
                            # Remove last line.
                            i = changeLogText.rfind('\n')

                            if i >= 0:
                                changeLogText = changeLogText[: i]

                            return changeLogText

                        changeLogText += line

        return ''

    def createInstaller(self, mutex):
        if not os.path.exists(self.qtIFW):
            return

        # Create layout
        configDir = os.path.join(self.installDir, 'installer/config')
        packageDir = os.path.join(self.installDir, 'installer/packages/com.webcamoidprj.webcamoid')

        if not os.path.exists(configDir):
            os.makedirs(configDir)

        dataDir = os.path.join(packageDir, 'data')
        metaDir = os.path.join(packageDir, 'meta')

        if not os.path.exists(metaDir):
            os.makedirs(metaDir)

        appIconSrc = os.path.join(self.installDir, 'usr/share/icons/hicolor/{0}x{0}/apps/webcamoid.png'.format(self.installerIconSize))
        self.copy(appIconSrc, configDir)
        self.copy(os.path.join(self.rootDir, 'COPYING'),
                  os.path.join(metaDir, 'COPYING.txt'))

        try:
            shutil.copytree(os.path.join(self.installDir, 'usr'), dataDir, True)
        except:
            pass

        configXml = os.path.join(configDir, 'config.xml')

        with open(configXml, 'w') as config:
            config.write('<?xml version="1.0" encoding="UTF-8"?>\n')
            config.write('<Installer>\n')
            config.write('    <Name>Webcamoid</Name>\n')
            config.write('    <Version>{}</Version>\n'.format(self.programVersion))
            config.write('    <Title>Webcamoid, The ultimate webcam suite!</Title>\n')
            config.write('    <Publisher>Webcamoid</Publisher>\n')
            config.write('    <ProductUrl>https://webcamoid.github.io/</ProductUrl>\n')
            config.write('    <InstallerWindowIcon>webcamoid</InstallerWindowIcon>\n')
            config.write('    <InstallerApplicationIcon>webcamoid</InstallerApplicationIcon>\n')
            config.write('    <Logo>webcamoid</Logo>\n')
            config.write('    <TitleColor>#3F1F7F</TitleColor>\n')
            config.write('    <RunProgram>@TargetDir@/webcamoid.sh</RunProgram>\n')
            config.write('    <RunProgramDescription>Launch Webcamoid now!</RunProgramDescription>\n')
            config.write('    <StartMenuDir>Webcamoid</StartMenuDir>\n')
            config.write('    <MaintenanceToolName>WebcamoidMaintenanceTool</MaintenanceToolName>\n')
            config.write('    <AllowNonAsciiCharacters>true</AllowNonAsciiCharacters>\n')
            config.write('    <TargetDir>@HomeDir@/webcamoid</TargetDir>\n')
            config.write('</Installer>\n')

        self.copy(os.path.join(self.rootDir, 'ports/deploy/installscript.posix.qs'),
                  os.path.join(metaDir, 'installscript.qs'))

        with open(os.path.join(metaDir, 'package.xml'), 'w') as f:
            f.write('<?xml version="1.0"?>\n')
            f.write('<Package>\n')
            f.write('    <DisplayName>Webcamoid</DisplayName>\n')
            f.write('    <Description>The ultimate webcam suite</Description>\n')
            f.write('    <Version>{}</Version>\n'.format(self.programVersion))
            f.write('    <ReleaseDate>{}</ReleaseDate>\n'.format(time.strftime('%Y-%m-%d')))
            f.write('    <Name>com.webcamoidprj.webcamoid</Name>\n')
            f.write('    <Licenses>\n')
            f.write('        <License name="GNU General Public License v3.0" file="COPYING.txt" />\n')
            f.write('    </Licenses>\n')
            f.write('    <Script>installscript.qs</Script>\n')
            f.write('    <UpdateText>\n')
            f.write(self.readChangeLog(os.path.join(self.rootDir, 'ChangeLog'),
                                       self.programVersion))
            f.write('    </UpdateText>\n')
            f.write('    <Default>true</Default>\n')
            f.write('    <ForcedInstallation>true</ForcedInstallation>\n')
            f.write('    <Essential>false</Essential>\n')
            f.write('</Package>\n')

        # Remove old file
        packagePath = os.path.join(self.pkgsDir,
                                   'webcamoid-{}-{}.run'.format(self.programVersion,
                                                                platform.machine()))

        if not os.path.exists(self.pkgsDir):
            os.makedirs(self.pkgsDir)

        if os.path.exists(packagePath):
            os.remove(packagePath)

        process = subprocess.Popen([self.qtIFW,
                                    '-c', configXml,
                                    '-p', os.path.join(self.installDir,
                                                       'installer/packages'),
                                    packagePath],
                                   stdout=subprocess.PIPE)
        process.communicate()

        mutex.acquire()
        print('Created installable package:')
        self.printPackageInfo(packagePath)
        mutex.release()

    def createAppImage(self, mutex):
        if not os.path.exists(self.appImage):
            return

        appDir = \
            os.path.join(self.installDir,
                         'webcamoid-{}-{}.AppDir'.format(self.programVersion,
                                                         platform.machine()))

        try:
            shutil.copytree(os.path.join(self.installDir, 'usr'),
                            appDir,
                            True)
        except:
            pass

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
                         'webcamoid-{}-{}.AppImage'.format(self.programVersion,
                                                           platform.machine()))

        if not os.path.exists(self.pkgsDir):
            os.makedirs(self.pkgsDir)

        if os.path.exists(packagePath):
            os.remove(packagePath)

        process = subprocess.Popen([self.appImage,
                                    '-v',
                                    '--no-appstream',
                                    '--comp', 'xz',
                                    appDir,
                                    packagePath],
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
        process.communicate()

        mutex.acquire()
        print('Created AppImage package:')
        self.printPackageInfo(packagePath)
        mutex.release()

    def package(self):
        print('\nCreating packages\n')
        mutex = threading.Lock()

        threads = [threading.Thread(target=self.createPortable, args=(mutex,)),
                   threading.Thread(target=self.createInstaller, args=(mutex,)),
                   threading.Thread(target=self.createAppImage, args=(mutex,))]

        for thread in threads:
            thread.start()

        for thread in threads:
            thread.join()

    def cleanup(self):
        shutil.rmtree(os.path.join(self.rootDir, 'ports/deploy/temp_priv'),
                      True)
