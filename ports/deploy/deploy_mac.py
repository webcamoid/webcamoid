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

import math
import os
import platform
import shutil
import subprocess # nosec
import sys
import threading
import time

import deploy_base
import tools.binary_mach
import tools.qt5


class Deploy(deploy_base.DeployBase, tools.qt5.DeployToolsQt):
    def __init__(self):
        super().__init__()
        self.installDir = os.path.join(self.buildDir, 'ports/deploy/temp_priv')
        self.pkgsDir = os.path.join(self.buildDir, 'ports/deploy/packages_auto', self.targetSystem)
        self.detectQt(os.path.join(self.buildDir, 'StandAlone'))
        self.rootInstallDir = os.path.join(self.installDir, 'Applications')
        self.programName = 'webcamoid'
        self.appBundleDir = os.path.join(self.rootInstallDir, self.programName + '.app')
        self.execPrefixDir = os.path.join(self.appBundleDir, 'Contents')
        self.binaryInstallDir = os.path.join(self.execPrefixDir, 'MacOS')
        self.libInstallDir = os.path.join(self.execPrefixDir, 'Frameworks')
        self.qmlInstallDir = os.path.join(self.execPrefixDir, 'Resources/qml')
        self.pluginsInstallDir = os.path.join(self.execPrefixDir, 'Plugins')
        self.qtConf = os.path.join(self.execPrefixDir, 'Resources/qt.conf')
        self.qmlRootDirs = ['StandAlone/share/qml', 'libAvKys/Plugins']
        self.mainBinary = os.path.join(self.binaryInstallDir, self.programName)
        self.programVersion = self.detectVersion(os.path.join(self.rootDir, 'commons.pri'))
        self.detectMake()
        xspec = self.qmakeQuery(var='QMAKE_XSPEC')

        if 'android' in xspec:
            self.targetSystem = 'android'

        self.binarySolver = tools.binary_mach.DeployToolsBinary()
        self.binarySolver.readExcludeList(os.path.join(self.rootDir, 'ports/deploy/exclude.{}.{}.txt'.format(os.name, sys.platform)))
        self.packageConfig = os.path.join(self.rootDir, 'ports/deploy/package_info.conf')
        self.dependencies = []
        self.installerConfig = os.path.join(self.installDir, 'installer/config')
        self.installerPackages = os.path.join(self.installDir, 'installer/packages')
        self.appIcon = os.path.join(self.execPrefixDir, 'Resources/{0}.icns'.format(self.programName))
        self.licenseFile = os.path.join(self.rootDir, 'COPYING')
        self.installerRunProgram = '@TargetDir@/{0}.app/Contents/MacOS/{0}'.format(self.programName)
        self.installerTargetDir = '@ApplicationsDir@/' + self.programName
        self.installerScript = os.path.join(self.rootDir, 'ports/deploy/installscript.mac.qs')
        self.changeLog = os.path.join(self.rootDir, 'ChangeLog')
        self.outPackage = os.path.join(self.pkgsDir,
                                   '{}-{}.dmg'.format(self.programName,
                                                      self.programVersion))

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
        print('Removing unnecessary files')
        self.removeUnneededFiles(self.libInstallDir)
        print('Fixing rpaths\n')
        self.fixRpaths()
        print('\nWritting build system information\n')
        self.writeBuildInfo()

    def solvedepsLibs(self):
        deps = sorted(self.binarySolver.scanDependencies(self.installDir))

        for dep in deps:
            depPath = os.path.join(self.libInstallDir, os.path.basename(dep))

            if dep != depPath:
                print('    {} -> {}'.format(dep, depPath))
                self.copy(dep, depPath, not dep.endswith('.framework'))
                self.dependencies.append(dep)

    @staticmethod
    def removeUnneededFiles(path):
        adirs = set()
        afiles = set()

        for root, dirs, files in os.walk(path):
            for d in dirs:
                if d == 'Headers':
                    adirs.add(os.path.join(root, d))

            for f in files:
                if f == 'Headers' or f.endswith('.prl'):
                    afiles.add(os.path.join(root, f))

        for adir in adirs:
            try:
                shutil.rmtree(adir, True)
            except:
                pass

        for afile in afiles:
            try:
                if os.path.islink(afile):
                    os.unlink(afile)
                else:
                    os.remove(afile)
            except:
                pass

    def fixLibRpath(self, mutex, mach):
        rpath = os.path.join('@executable_path',
                             os.path.relpath(self.libInstallDir,
                                             self.binaryInstallDir))
        log = '\tFixed {}\n\n'.format(mach)
        machInfo = self.binarySolver.dump(mach)

        # Change rpath
        if mach.startswith(self.binaryInstallDir):
            log += '\t\tChanging rpath to {}\n'.format(rpath)

            for oldRpath in machInfo['rpaths']:
                process = subprocess.Popen(['install_name_tool', # nosec
                                            '-delete_rpath', oldRpath, mach],
                                           stdout=subprocess.PIPE)
                process.communicate()

            process = subprocess.Popen(['install_name_tool', # nosec
                                        '-add_rpath', rpath, mach],
                                       stdout=subprocess.PIPE)
            process.communicate()

        # Change ID
        if mach.startswith(self.binaryInstallDir):
            newMachId = machInfo['id']
        elif mach.startswith(self.libInstallDir):
            newMachId = mach.replace(self.libInstallDir, rpath)
        else:
            newMachId = os.path.basename(mach)

        if newMachId != machInfo['id']:
            log += '\t\tChanging ID to {}\n'.format(newMachId)

            process = subprocess.Popen(['install_name_tool', # nosec
                                        '-id', newMachId, mach],
                                       stdout=subprocess.PIPE)
            process.communicate()

        # Change library links
        for dep in machInfo['imports']:
            if dep.startswith(rpath):
                continue

            if self.binarySolver.isExcluded(dep):
                continue

            basename = os.path.basename(dep)
            framework = ''
            inFrameworkPath = ''

            if not basename.endswith('.dylib'):
                frameworkPath = dep[: dep.rfind('.framework')] + '.framework'
                framework = os.path.basename(frameworkPath)
                inFrameworkPath = os.path.join(framework, dep.replace(frameworkPath + '/', ''))

            newDepPath = os.path.join(rpath, basename if len(framework) < 1 else inFrameworkPath)

            if dep != newDepPath:
                log += '\t\t{} -> {}\n'.format(dep, newDepPath)

                process = subprocess.Popen(['install_name_tool', # nosec
                                            '-change', dep, newDepPath, mach],
                                           stdout=subprocess.PIPE)
                process.communicate()

        mutex.acquire()
        print(log)
        mutex.release()

    def fixRpaths(self):
        path = os.path.join(self.execPrefixDir)
        mutex = threading.Lock()
        threads = []

        for mach in self.binarySolver.find(path):
            thread = threading.Thread(target=self.fixLibRpath, args=(mutex, mach,))
            threads.append(thread)

            while threading.active_count() >= self.njobs:
                time.sleep(0.25)

            thread.start()

        for thread in threads:
            thread.join()

    @staticmethod
    def searchPackageFor(cellarPath, path):
        if not path.startswith(cellarPath):
            return ''

        return ' '.join(path.replace(cellarPath + os.sep, '').split(os.sep)[0: 2])

    def commitHash(self):
        try:
            process = subprocess.Popen(['git', 'rev-parse', 'HEAD'], # nosec
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE,
                                        cwd=self.rootDir)
            stdout, _ = process.communicate()

            if process.returncode != 0:
                return ''

            return stdout.decode(sys.getdefaultencoding()).strip()
        except:
            return ''

    @staticmethod
    def sysInfo():
        process = subprocess.Popen(['sw_vers'], # nosec
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
        stdout, _ = process.communicate()

        return stdout.decode(sys.getdefaultencoding()).strip()

    def writeBuildInfo(self):
        resourcesDir = os.path.join(self.execPrefixDir, 'Resources')
        os.makedirs(self.pkgsDir)
        depsInfoFile = os.path.join(resourcesDir, 'build-info.txt')

        # Write repository info.

        with open(depsInfoFile, 'w') as f:
            commitHash = self.commitHash()

            if len(commitHash) < 1:
                commitHash = 'Unknown'

            print('    Commit hash: ' + commitHash)
            f.write('Commit hash: ' + commitHash + '\n')

            buildLogUrl = ''

            if 'TRAVIS_BUILD_WEB_URL' in os.environ:
                buildLogUrl = os.environ['TRAVIS_BUILD_WEB_URL']
            elif 'APPVEYOR_ACCOUNT_NAME' in os.environ and 'APPVEYOR_PROJECT_NAME' in os.environ and 'APPVEYOR_JOB_ID' in os.environ:
                buildLogUrl = 'https://ci.appveyor.com/project/{}/{}/build/job/{}'.format(os.environ['APPVEYOR_ACCOUNT_NAME'],
                                                                                          os.environ['APPVEYOR_PROJECT_SLUG'],
                                                                                          os.environ['APPVEYOR_JOB_ID'])

            if len(buildLogUrl) > 0:
                print('    Build log URL: ' + buildLogUrl)
                f.write('Build log URL: ' + buildLogUrl + '\n')

            print()
            f.write('\n')

        # Write host info.

        info = self.sysInfo()

        with open(depsInfoFile, 'a') as f:
            for line in info.split('\n'):
                if len(line) > 0:
                    print('    ' + line)
                    f.write(line + '\n')

            print()
            f.write('\n')

        os.environ['LC_ALL'] = 'C'
        brew = self.whereBin('brew')

        if len(brew) < 1:
            return

        process = subprocess.Popen([brew, '--cellar'], # nosec
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
        stdout, _ = process.communicate()
        cellarPath = stdout.decode(sys.getdefaultencoding()).strip()

        # Write binary dependencies info.

        packages = set()

        for dep in self.dependencies:
            packageInfo = self.searchPackageFor(cellarPath, dep)

            if len(packageInfo) > 0:
                packages.add(packageInfo)

        packages = sorted(packages)

        with open(depsInfoFile, 'a') as f:
            for packge in packages:
                print('    ' + packge)
                f.write(packge + '\n')

    @staticmethod
    def hrSize(size):
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
            print('    sha256sum:', Deploy.sha256sum(path))
        else:
            print('   ',
                  os.path.basename(path),
                  'FAILED')

    @staticmethod
    def dirSize(path):
        size = 0

        for root, _, files in os.walk(path):
            for f in files:
                fpath = os.path.join(root, f)

                if not os.path.islink(fpath):
                    size += os.path.getsize(fpath)

        return size

    # https://asmaloney.com/2013/07/howto/packaging-a-mac-os-x-application-using-a-dmg/
    def createPortable(self, mutex):
        staggingDir = os.path.join(self.installDir, 'stagging')

        if not os.path.exists(staggingDir):
            os.makedirs(staggingDir)

        self.copy(self.appBundleDir,
                  os.path.join(staggingDir, self.programName + '.app'))
        imageSize = self.dirSize(staggingDir)
        tmpDmg = os.path.join(self.installDir, self.programName + '_tmp.dmg')
        volumeName = "{}-portable-{}".format(self.programName,
                                             self.programVersion)

        process = subprocess.Popen(['hdiutil', 'create', # nosec
                                    '-srcfolder', staggingDir,
                                    '-volname', volumeName,
                                    '-fs', 'HFS+',
                                    '-fsargs', '-c c=64,a=16,e=16',
                                    '-format', 'UDRW',
                                    '-size', str(math.ceil(imageSize * 1.1)),
                                    tmpDmg],
                                   stdout=subprocess.PIPE)
        process.communicate()

        process = subprocess.Popen(['hdiutil', # nosec
                                    'attach',
                                    '-readwrite',
                                    '-noverify',
                                    tmpDmg],
                                   stdout=subprocess.PIPE)
        stdout, _ = process.communicate()
        device = ''

        for line in stdout.split(b'\n'):
            line = line.strip()

            if len(line) < 1:
                continue

            dev = line.split()

            if len(dev) > 2:
                device = dev[0].decode(sys.getdefaultencoding())

                break

        time.sleep(2)
        volumePath = os.path.join('/Volumes', volumeName)
        volumeIcon = os.path.join(volumePath, '.VolumeIcon.icns')
        self.copy(self.appIcon, volumeIcon)

        process = subprocess.Popen(['SetFile', # nosec
                                    '-c', 'icnC',
                                    volumeIcon],
                                   stdout=subprocess.PIPE)
        process.communicate()

        process = subprocess.Popen(['SetFile', # nosec
                                    '-a', 'C',
                                    volumePath],
                                   stdout=subprocess.PIPE)
        process.communicate()

        appsShortcut = os.path.join(volumePath, 'Applications')

        if not os.path.exists(appsShortcut):
            os.symlink('/Applications', appsShortcut)

        os.sync()

        process = subprocess.Popen(['hdiutil', # nosec
                                    'detach',
                                    device],
                                   stdout=subprocess.PIPE)
        process.communicate()

        packagePath = \
            os.path.join(self.pkgsDir,
                         '{}-portable-{}-{}.dmg'.format(self.programName,
                                                        self.programVersion,
                                                        platform.machine()))

        if not os.path.exists(self.pkgsDir):
            os.makedirs(self.pkgsDir)

        if os.path.exists(packagePath):
            os.remove(packagePath)

        process = subprocess.Popen(['hdiutil', # nosec
                                    'convert',
                                    tmpDmg,
                                    '-format', 'UDZO',
                                    '-imagekey', 'zlib-level=9',
                                    '-o', packagePath],
                                   stdout=subprocess.PIPE)
        process.communicate()

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

    def package(self):
        mutex = threading.Lock()

        threads = [threading.Thread(target=self.createPortable, args=(mutex,))]
        packagingTools = ['dmg']

        if self.qtIFW != '':
            threads.append(threading.Thread(target=self.createAppInstaller, args=(mutex,)))
            packagingTools += ['Qt Installer Framework']

        if len(packagingTools) > 0:
            print('Detected packaging tools: {}\n'.format(', '.join(packagingTools)))

        for thread in threads:
            thread.start()

        for thread in threads:
            thread.join()
