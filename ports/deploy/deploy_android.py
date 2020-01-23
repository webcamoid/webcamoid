#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Webcamoid, webcam capture application.
# Copyright (C) 2019  Gonzalo Exequiel Pedone
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

import json
import math
import os
import re
import shutil
import subprocess # nosec
import sys
import threading
import zipfile

import deploy_base
import tools.android
import tools.binary_elf
import tools.qt5


class Deploy(deploy_base.DeployBase,
             tools.qt5.DeployToolsQt,
             tools.android.AndroidTools):
    def __init__(self):
        super().__init__()
        self.targetSystem = 'android'
        self.installDir = os.path.join(self.buildDir, 'ports/deploy/temp_priv')
        self.pkgsDir = os.path.join(self.buildDir, 'ports/deploy/packages_auto/android')
        self.standAloneDir = os.path.join(self.buildDir, 'StandAlone')
        self.detectQt(self.standAloneDir)
        self.programName = 'webcamoid'
        self.binarySolver = tools.binary_elf.DeployToolsBinary()
        self.detectAndroidPlatform(self.standAloneDir)
        binary = self.detectTargetBinaryFromQt5Make(self.standAloneDir)
        self.targetArch = self.binarySolver.machineEMCode(binary)
        self.androidArchMap = {'AARCH64': 'arm64-v8a',
                               'ARM'    : 'armeabi-v7a',
                               '386'    : 'x86',
                               'X86_64' : 'x86_64'}

        if self.targetArch in self.androidArchMap:
            self.targetArch = self.androidArchMap[self.targetArch]

        self.binarySolver.sysBinsPath = self.detectBinPaths() + self.binarySolver.sysBinsPath
        self.binarySolver.libsSeachPaths = self.detectLibPaths()
        self.rootInstallDir = os.path.join(self.installDir, self.programName)
        self.libInstallDir = os.path.join(self.rootInstallDir,
                                          'libs',
                                          self.targetArch)
        self.binaryInstallDir = self.libInstallDir
        self.assetsIntallDir = os.path.join(self.rootInstallDir,
                                            'assets',
                                            'android_rcc_bundle')
        self.qmlInstallDir = os.path.join(self.assetsIntallDir, 'qml')
        self.pluginsInstallDir = os.path.join(self.assetsIntallDir, 'plugins')
        self.qtConf = os.path.join(self.binaryInstallDir, 'qt.conf')
        self.qmlRootDirs = ['StandAlone/share/qml', 'libAvKys/Plugins']
        self.mainBinary = os.path.join(self.binaryInstallDir, os.path.basename(binary))
        self.programVersion = self.detectVersion(os.path.join(self.rootDir, 'commons.pri'))
        self.detectMake()
        self.binarySolver.readExcludeList(os.path.join(self.rootDir, 'ports/deploy/exclude.android.txt'))
        self.binarySolver.libsSeachPaths += [self.qmakeQuery(var='QT_INSTALL_LIBS')]
        self.packageConfig = os.path.join(self.rootDir, 'ports/deploy/package_info.conf')
        self.dependencies = []

    @staticmethod
    def removeUnneededFiles(path):
        afiles = set()

        for root, _, files in os.walk(path):
            for f in files:
                if f.endswith('.jar'):
                    afiles.add(os.path.join(root, f))

        for afile in afiles:
            os.remove(afile)

    def prepare(self):
        print('Executing make install')
        self.makeInstall(self.buildDir, self.rootInstallDir)
        self.binarySolver.detectStrip()

        if 'PACKAGES_MERGE' in os.environ \
            and len(os.environ['PACKAGES_MERGE']) > 0:
            self.outPackage = \
                os.path.join(self.pkgsDir,
                             '{}-{}.apk'.format(self.programName,
                                                self.programVersion))
        else:
            self.outPackage = \
                os.path.join(self.pkgsDir,
                            '{}-{}-{}.apk'.format(self.programName,
                                                  self.programVersion,
                                                  self.targetArch))

        print('Copying Qml modules\n')
        self.solvedepsQml()
        print('\nCopying required plugins\n')
        self.solvedepsPlugins()
        print('\nRemoving unused architectures')
        self.removeInvalidArchs()
        print('Fixing Android libs\n')
        self.fixQtLibs()

        try:
            shutil.rmtree(self.pluginsInstallDir)
        except:
            pass

        print('\nCopying required libs\n')
        self.solvedepsLibs()
        print('\nSolving Android dependencies\n')
        self.solvedepsAndroid()
        print('\nCopying Android build templates')
        self.copyAndroidTemplates()
        print('Fixing libs.xml file')
        self.fixLibsXml()
        print('Creating .rcc bundle file')
        self.createRccBundle()
        print('Stripping symbols')
        self.binarySolver.stripSymbols(self.rootInstallDir)
        print('Removing unnecessary files')
        self.removeUnneededFiles(self.libInstallDir)
        print('Writting build system information\n')
        self.writeBuildInfo()

    def removeInvalidArchs(self):
        suffix = '_{}.so'.format(self.targetArch)

        if not self.mainBinary.endswith(suffix):
            return

        for root, dirs, files in os.walk(self.assetsIntallDir):
            for f in files:
                if f.endswith('.so') and not f.endswith(suffix):
                    os.remove(os.path.join(root, f))

    def solvedepsLibs(self):
        qtLibsPath = self.qmakeQuery(var='QT_INSTALL_LIBS')
        self.binarySolver.ldLibraryPath.append(qtLibsPath)
        self.qtLibs = sorted(self.binarySolver.scanDependencies(self.rootInstallDir))

        for dep in self.qtLibs:
            depPath = os.path.join(self.libInstallDir, os.path.basename(dep))

            if dep != depPath:
                print('    {} -> {}'.format(dep, depPath))
                self.copy(dep, depPath, True)
                self.dependencies.append(dep)

    def searchPackageFor(self, path):
        os.environ['LC_ALL'] = 'C'
        pacman = self.whereBin('pacman')

        if len(pacman) > 0:
            process = subprocess.Popen([pacman, '-Qo', path], # nosec
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE)
            stdout, _ = process.communicate()

            if process.returncode != 0:
                return ''

            info = stdout.decode(sys.getdefaultencoding()).split(' ')

            if len(info) < 2:
                return ''

            package, version = info[-2:]

            return ' '.join([package.strip(), version.strip()])

        dpkg = self.whereBin('dpkg')

        if len(dpkg) > 0:
            process = subprocess.Popen([dpkg, '-S', path], # nosec
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE)
            stdout, _ = process.communicate()

            if process.returncode != 0:
                return ''

            package = stdout.split(b':')[0].decode(sys.getdefaultencoding()).strip()

            process = subprocess.Popen([dpkg, '-s', package], # nosec
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE)
            stdout, _ = process.communicate()

            if process.returncode != 0:
                return ''

            for line in stdout.decode(sys.getdefaultencoding()).split('\n'):
                line = line.strip()

                if line.startswith('Version:'):
                    return ' '.join([package, line.split()[1].strip()])

            return ''

        rpm = self.whereBin('rpm')

        if len(rpm) > 0:
            process = subprocess.Popen([rpm, '-qf', path], # nosec
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE)
            stdout, _ = process.communicate()

            if process.returncode != 0:
                return ''

            return stdout.decode(sys.getdefaultencoding()).strip()

        return ''

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
        info = ''

        for f in os.listdir('/etc'):
            if f.endswith('-release'):
                with open(os.path.join('/etc' , f)) as releaseFile:
                    info += releaseFile.read()

        return info

    def writeBuildInfo(self):
        shareDir = os.path.join(self.rootInstallDir, 'assets')

        try:
            os.makedirs(self.pkgsDir)
        except:
            pass

        depsInfoFile = os.path.join(shareDir, 'build-info.txt')

        if not os.path.exists(shareDir):
            os.makedirs(shareDir)

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

        # Write SDK and NDK info.

        sdkInfoFile = os.path.join(self.androidSDK, 'tools', 'source.properties')
        ndkInfoFile = os.path.join(self.androidNDK, 'source.properties')

        with open(depsInfoFile, 'a') as f:
            platform = self.androidPlatform
            print('    Android Platform: {}'.format(platform))
            f.write('Android Platform: {}\n'.format(platform))
            print('    SDK Info: \n')
            f.write('SDK Info: \n\n')

            with open(sdkInfoFile) as sdkf:
                for line in sdkf:
                    if len(line) > 0:
                        print('        ' + line.strip())
                        f.write('    ' + line)

            print('\n    NDK Info: \n')
            f.write('\nNDK Info: \n\n')

            with open(ndkInfoFile) as ndkf:
                for line in ndkf:
                    if len(line) > 0:
                        print('        ' + line.strip())
                        f.write('    ' + line)

            print()
            f.write('\n')

        # Write binary dependencies info.

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

    def alignPackage(self, package):
        deploymentSettingsPath = ''

        for f in os.listdir(self.standAloneDir):
            if re.match('^android-.+-deployment-settings.json$' , f):
                deploymentSettingsPath = os.path.join(self.standAloneDir, f)

                break

        if len(deploymentSettingsPath) < 1:
            return

        with open(deploymentSettingsPath) as f:
            deploymentSettings = json.load(f)

        zipalign = os.path.join(self.androidSDK,
                                'build-tools',
                                deploymentSettings['sdkBuildToolsRevision'],
                                'zipalign')

        if self.system == 'windows':
            zipalign += '.exe'

        alignedPackage = os.path.join(os.path.dirname(package),
                                      'aligned-' + os.path.basename(package))
        process = subprocess.Popen([zipalign, # nosec
                                    '-v',
                                    '-f', '4',
                                    package,
                                    alignedPackage],
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
        process.communicate()

        if process.returncode != 0:
            return False

        self.move(alignedPackage, package)

        return True

    def apkSignPackage(self, package, keystore):
        if not self.alignPackage(package):
            return False

        deploymentSettingsPath = ''

        for f in os.listdir(self.standAloneDir):
            if re.match('^android-.+-deployment-settings.json$' , f):
                deploymentSettingsPath = os.path.join(self.standAloneDir, f)

                break

        if len(deploymentSettingsPath) < 1:
            return

        with open(deploymentSettingsPath) as f:
            deploymentSettings = json.load(f)

        apkSigner = os.path.join(self.androidSDK,
                                 'build-tools',
                                 deploymentSettings['sdkBuildToolsRevision'],
                                 'apksigner')

        if self.system == 'windows':
            apkSigner += '.exe'

        process = subprocess.Popen([apkSigner, # nosec
                                    'sign',
                                    '-v',
                                    '--ks', keystore,
                                    '--ks-pass', 'pass:android',
                                    '--ks-key-alias', 'androiddebugkey',
                                    '--key-pass', 'pass:android',
                                    package],
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
        process.communicate()

        return process.returncode == 0

    def jarSignPackage(self, package, keystore):
        jarSigner = 'jarsigner'

        if self.system == 'windows':
            jarSigner += '.exe'

        jarSignerPath = ''

        if 'JAVA_HOME' in os.environ:
            jarSignerPath = os.path.join(os.environ['JAVA_HOME'],
                                         'bin',
                                         jarSigner)

        if len(jarSignerPath) < 1 or not os.path.exists(jarSignerPath):
            jarSignerPath = self.whereBin(jarSigner)

            if len(jarSignerPath) < 1:
                return False

        signedPackage = os.path.join(os.path.dirname(package),
                                     'signed-' + os.path.basename(package))
        process = subprocess.Popen([jarSignerPath, # nosec
                                    '-verbose',
                                    '-keystore', keystore,
                                    '-storepass', 'android',
                                    '-keypass', 'android',
                                    '-sigalg', 'SHA1withRSA',
                                    '-digestalg', 'SHA1',
                                    '-sigfile', 'CERT',
                                    '-signedjar', signedPackage,
                                    package,
                                    'androiddebugkey'],
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
        process.communicate()

        if process.returncode != 0:
            return False

        self.move(signedPackage, package)

        return self.alignPackage(package)

    def signPackage(self, package):
        keytool = 'keytool'

        if self.system == 'windows':
            keytool += '.exe'

        keytoolPath = ''

        if 'JAVA_HOME' in os.environ:
            keytoolPath = os.path.join(os.environ['JAVA_HOME'], 'bin', keytool)

        if len(keytoolPath) < 1 or not os.path.exists(keytoolPath):
            keytoolPath = self.whereBin(keytool)

            if len(keytoolPath) < 1:
                return False

        keystore = os.path.join(self.rootInstallDir, 'debug.keystore')

        if 'KEYSTORE_PATH' in os.environ:
            keystore = os.environ['KEYSTORE_PATH']

        if not os.path.exists(keystore):
            try:
                os.makedirs(os.path.dirname(keystore))
            except:
                pass

            process = subprocess.Popen([keytoolPath, # nosec
                                        '-genkey',
                                        '-v',
                                        '-storetype', 'pkcs12',
                                        '-keystore', keystore,
                                        '-storepass', 'android',
                                        '-alias', 'androiddebugkey',
                                        '-keypass', 'android',
                                        '-keyalg', 'RSA',
                                        '-keysize', '2048',
                                        '-validity', '10000',
                                        '-dname', 'CN=Android Debug,O=Android,C=US'],
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE)
            process.communicate()

            if process.returncode != 0:
                return False

        if self.apkSignPackage(package, keystore):
            return True

        return self.jarSignPackage(package, keystore)

    def createApk(self, mutex):
        if 'PACKAGES_MERGE' in os.environ:
            print('Merging package data:\n')

            for path in os.environ['PACKAGES_MERGE'].split(':'):
                path = path.strip()

                if os.path.exists(path) and os.path.isdir(path):
                    if path == self.buildDir:
                        continue

                    standAlonePath = os.path.join(path,
                                                  os.path.relpath(self.standAloneDir,
                                                                  self.buildDir))
                    binary = self.detectTargetBinaryFromQt5Make(standAlonePath)
                    targetArch = self.binarySolver.machineEMCode(binary)

                    if targetArch in self.androidArchMap:
                        targetArch = self.androidArchMap[targetArch]

                    libsPath = os.path.join(path,
                                            os.path.relpath(self.rootInstallDir,
                                                            self.buildDir),
                                            'libs',
                                            targetArch)
                    dstLibPath = os.path.join(self.rootInstallDir,
                                              'libs',
                                              targetArch)
                    print('    {} -> {}'.format(libsPath, dstLibPath))
                    self.copy(libsPath, dstLibPath)

            print()

        if not os.path.exists(self.pkgsDir):
            os.makedirs(self.pkgsDir)

        gradleSript = os.path.join(self.rootInstallDir, 'gradlew')

        if self.system == 'windows':
            gradleSript += '.bat'

        os.chmod(gradleSript, 0o744)
        process = subprocess.Popen([gradleSript, # nosec
                                    '--no-daemon',
                                    '--info',
                                    'assembleRelease'],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE,
                                   cwd=self.rootInstallDir)
        process.communicate()
        apk = os.path.join(self.rootInstallDir,
                           'build',
                           'outputs',
                           'apk',
                           'release',
                           '{}-release-unsigned.apk'.format(self.programName))
        self.signPackage(apk)
        self.copy(apk, self.outPackage)

        print('Created APK package:')
        self.printPackageInfo(self.outPackage)

    def package(self):
        mutex = threading.Lock()

        threads = [threading.Thread(target=self.createApk, args=(mutex,))]
        packagingTools = ['apk']

        if len(packagingTools) > 0:
            print('Detected packaging tools: {}\n'.format(', '.join(packagingTools)))

        for thread in threads:
            thread.start()

        for thread in threads:
            thread.join()
