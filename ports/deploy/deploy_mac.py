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
import threading
import time


class Deploy:
    def __init__(self, rootDir, system, arch):
        self.scanPaths = ['StandAlone/webcamoid',
                          'StandAlone/share/qml',
                          'libAvKys/Plugins']
        self.rootDir = rootDir
        self.system = system
        self.arch = arch
        self.targetArch = self.arch
        self.targetSystem = self.system
        self.appPath = os.path.join(self.rootDir, self.scanPaths[0] + '.app')
        self.programName = os.path.basename(self.scanPaths[0])
        self.programVersion = self.readVersion()
        self.qmake = self.detectQmake()
        self.qtIFW = self.detectQtIFW()
        self.njobs = multiprocessing.cpu_count()

        # 32 bits magic number.
        self.MH_MAGIC = 0xfeedface # Native endian
        self.MH_CIGAM = 0xcefaedfe # Reverse endian

        # 64 bits magic number.
        self.MH_MAGIC_64 = 0xfeedfacf # Native endian
        self.MH_CIGAM_64 = 0xcffaedfe # Reverse endian

    def readVersion(self):
        os.environ['DYLD_LIBRARY_PATH'] = os.path.join(self.rootDir, 'libAvKys/Lib')
        programPath = os.path.join(self.appPath,
                                   'Contents/MacOS',
                                   self.programName)
        process = subprocess.Popen([programPath, '--version'],
                                   stdout=subprocess.PIPE)
        stdout, stderr = process.communicate()

        try:
            return stdout.split()[1].strip().decode('utf-8')
        except:
            return 'unknown'

    def detectQmake(self):
        if 'QMAKE_PATH' in os.environ:
            return os.environ['QMAKE_PATH']

        with open(os.path.join(self.rootDir, 'StandAlone/Makefile')) as f:
            for line in f:
                if line.startswith('QMAKE'):
                    return line.split('=')[1].strip()

        return ''

    def detectQtIFW(self):
        if 'BINARYCREATOR' in os.environ:
            return os.environ['BINARYCREATOR']

        # Try official Qt binarycreator because it is statically linked.
        homeQt = os.path.expanduser('~/Qt')

        if os.path.exists(homeQt):
            for f in os.listdir(homeQt):
                path = os.path.join(homeQt, f)

                if fnmatch.fnmatch(path, os.path.join(homeQt, 'QtIFW*')):
                    bcPath = os.path.join(path, 'bin/binarycreator')

                    if os.path.exists(bcPath):
                        return bcPath

        # binarycreator offered by the system is most probably dynamically
        # linked, so it's useful for test purposes only, but not recommended
        # for distribution.
        return self.whereBin('binarycreator')

    def qmakeQuery(self, var):
        process = subprocess.Popen([self.qmake, '-query', var],
                                   stdout=subprocess.PIPE)
        stdout, stderr = process.communicate()

        return stdout.strip().decode(sys.getdefaultencoding())

    def copy(self, src, dst):
        if not os.path.exists(src):
            return

        basename = os.path.basename(src)
        dstpath = os.path.join(dst, basename) if os.path.isdir(dst) else dst
        dstdir = dst if os.path.isdir(dst) else os.path.dirname(dst)

        if os.path.islink(src):
            rsrc = os.path.realpath(src)
            rbasename = os.path.basename(rsrc)
            rdstpath = os.path.join(dstdir, rbasename)

            if not os.path.exists(rdstpath):
                shutil.copy(rsrc, rdstpath)

            if not os.path.exists(dstpath):
                os.symlink(os.path.join('.', rbasename), dstpath)
        elif not os.path.exists(dstpath):
            shutil.copy(src, dst)

    def prepare(self):
        self.sysQmlPath = self.qmakeQuery('QT_INSTALL_QML')
        self.sysPluginsPath = self.qmakeQuery('QT_INSTALL_PLUGINS')
        libDir = os.path.join(self.rootDir, 'libAvKys/Lib')
        self.installDir = os.path.join(self.rootDir, 'ports/deploy/temp_priv')
        self.pkgsDir = os.path.join(self.rootDir, 'ports/deploy/packages_auto/mac')

        if not os.path.exists(self.installDir):
            os.makedirs(self.installDir)

        self.appInstallPath = os.path.join(self.installDir, os.path.basename(self.appPath))

        if not os.path.exists(self.appInstallPath):
            shutil.copytree(self.appPath, self.appInstallPath, True)

        contents = os.path.join(self.appInstallPath, 'Contents')
        rootInstallDir = os.path.join(self.installDir, 'root')

        previousDir = os.getcwd()
        os.chdir(os.path.join(self.rootDir, 'libAvkys'))
        process = subprocess.Popen(['make',
                                    'INSTALL_ROOT={}'.format(rootInstallDir),
                                    'install'],
                                   stdout=subprocess.PIPE)
        process.communicate()
        os.chdir(previousDir)

        if process.returncode != 0:
            return

        frameworksPath = os.path.join(contents, 'Frameworks')

        if not os.path.exists(frameworksPath):
            os.makedirs(frameworksPath)

        installLibDir = os.path.join(rootInstallDir, 'usr/lib')

        for f in os.listdir(installLibDir):
            if f.endswith('.dylib'):
                self.copy(os.path.join(installLibDir, f),
                          frameworksPath)

        qmlPath = os.path.join(contents, 'Resources/qml')

        if not os.path.exists(qmlPath):
            os.makedirs(qmlPath)

        try:
            shutil.copytree(os.path.join(installLibDir, 'qt/qml/AkQml'),
                            os.path.join(qmlPath, 'AkQml'),
                            True)
        except:
            pass

        pluginsPath = os.path.join(contents, 'Plugins')

        if not os.path.exists(pluginsPath):
            os.makedirs(pluginsPath)

        try:
            shutil.copytree(os.path.join(installLibDir, 'avkys'),
                            os.path.join(pluginsPath, 'avkys'),
                            True)
        except:
            pass

    def modulePath(self, importLine):
        imp = importLine.strip().split()
        path = imp[1].replace('.', '/')
        majorVersion = imp[2].split('.')[0]

        if int(majorVersion) > 1:
            path += '.{}'.format(majorVersion)

        return path

    def scanImports(self, path):
        if not os.path.isfile(path):
            return []

        fileName = os.path.basename(path)
        imports = set()

        if fileName.endswith('.qml'):
            with open(path, 'rb') as f:
                for line in f:
                    if re.match(b'^import \\w+' , line):
                        imports.add(self.modulePath(line.strip().decode(sys.getdefaultencoding())))
        elif fileName == 'qmldir':
            with open(path, 'rb') as f:
                for line in f:
                    if re.match(b'^depends ' , line):
                        imports.add(self.modulePath(line.strip().decode(sys.getdefaultencoding())))

        return list(imports)

    def listQmlFiles(self, path):
        qmlFiles = set()

        if os.path.isfile(path):
            baseName = os.path.basename(path)

            if baseName == 'qmldir' or path.endswith('.qml'):
                qmlFiles.add(path)
        else:
            for root, dirs, files in os.walk(os.path.join(self.rootDir, path)):
                for f in files:
                    if f == 'qmldir' or f.endswith('.qml'):
                        qmlFiles.add(os.path.join(root, f))

        return list(qmlFiles)

    def solvedepsQml(self):
        print('Copying Qml modules\n')
        qmlFiles = set()

        for path in self.scanPaths:
            path = os.path.join(self.rootDir, path)

            for f in self.listQmlFiles(path):
                qmlFiles.add(f)

        qmlPath = os.path.join(self.appInstallPath, 'Contents/Resources/qml')
        solved = set()
        solvedImports = set()

        while len(qmlFiles) > 0:
            qmlFile = qmlFiles.pop()

            for imp in self.scanImports(qmlFile):
                if imp in solvedImports:
                    continue

                sysModulePath = os.path.join(self.sysQmlPath, imp)
                installModulePath = os.path.join(qmlPath, imp)

                if os.path.exists(sysModulePath):
                    print('    {} -> {}'.format(sysModulePath, installModulePath))
                    path = installModulePath[: installModulePath.rfind(os.sep)]

                    if not os.path.exists(path):
                        os.makedirs(path)

                    try:
                        shutil.copytree(sysModulePath, installModulePath, True)
                    except:
                        pass

                    solvedImports.add(imp)

                    for f in self.listQmlFiles(sysModulePath):
                        if not f in solved:
                            qmlFiles.add(f)

            solved.add(qmlFile)

    def isMach(self, path):
        try:
            with open(path, 'rb') as f:
                # Read magic number.
                magic = struct.unpack('I', f.read(4))[0]

                if magic == self.MH_MAGIC \
                    or magic == self.MH_CIGAM \
                    or magic == self.MH_MAGIC_64 \
                    or magic == self.MH_CIGAM_64:
                    return True
        except:
            pass

        return False

    def findMachs(self, path):
        machs = []

        for root, dirs, files in os.walk(path):
            for f in files:
                machPath = os.path.join(root, f)

                if not os.path.islink(machPath) and self.isMach(machPath):
                    machs.append(machPath)

        return machs

    # https://github.com/aidansteele/osx-abi-macho-file-format-reference
    def machDump(self, mach):
        # Commands definitions
        LC_REQ_DYLD = 0x80000000
        LC_LOAD_DYLIB = 0xc
        LC_RPATH = 0x1c | LC_REQ_DYLD
        LC_ID_DYLIB = 0xd

        dylibImports = []
        rpaths = []
        dylibId = ''

        with open(mach, 'rb') as f:
            # Read magic number.
            magic = struct.unpack('I', f.read(4))[0]

            if magic == self.MH_MAGIC or magic == self.MH_CIGAM:
                is32bits = True
            elif magic == self.MH_MAGIC_64 or magic == self.MH_CIGAM_64:
                is32bits = False
            else:
                return {}

            # Read number of commands.
            f.seek(12, os.SEEK_CUR)
            ncmds = struct.unpack('I', f.read(4))[0]

            # Move to load commands
            f.seek(8 if is32bits else 12, os.SEEK_CUR)

            for i in range(ncmds):
                # Read a load command and store it's position in the file.
                loadCommandStart = f.tell()
                loadCommand = struct.unpack('II', f.read(8))

                # If the command list a library
                if loadCommand[0] in [LC_LOAD_DYLIB, LC_RPATH, LC_ID_DYLIB]:
                    # Save the position of the next command.
                    nextCommand = f.tell() + loadCommand[1] - 8

                    # Move to the string
                    f.seek(loadCommandStart + struct.unpack('I', f.read(4))[0], os.SEEK_SET)
                    dylib = b''

                    # Read string until null character.
                    while True:
                        c = f.read(1)

                        if c == b'\x00':
                            break

                        dylib += c
                    s = dylib.decode(sys.getdefaultencoding())

                    if loadCommand[0] == LC_LOAD_DYLIB:
                        dylibImports.append(s)
                    elif loadCommand[0] == LC_RPATH:
                        rpaths.append(s)
                    elif loadCommand[0] == LC_ID_DYLIB:
                        dylibId = s

                    f.seek(nextCommand, os.SEEK_SET)
                else:
                    f.seek(loadCommand[1] - 8, os.SEEK_CUR)

        return {'imports': dylibImports, 'rpaths': rpaths, 'id': dylibId}

    def solveRefpath(self, path):
        if not path.startswith('@'):
            return path

        searchPaths = []

        if 'DYLD_LIBRARY_PATH' in os.environ:
            searchPaths += os.environ['DYLD_LIBRARY_PATH'].split(':')

        if 'DYLD_FRAMEWORK_PATH' in os.environ:
            searchPaths += os.environ['DYLD_FRAMEWORK_PATH'].split(':')

        basename = os.path.basename(path)

        for fpath in searchPaths:
            realPath = os.path.join(fpath, basename)

            if os.path.exists(realPath):
                return realPath

        return ''

    def listDependencies(self, path, solve=True):
        machInfo = self.machDump(path)

        if not machInfo:
            return []

        libs = []

        for mach in machInfo['imports']:
            if solve:
                mach = self.solveRefpath(mach)

            if len(mach) < 1:
                continue

            if not mach.startswith('@') and not os.path.exists(mach):
                continue

            libs.append(mach)

        return libs

    def libName(self, lib):
        dep = os.path.basename(lib)
        i = dep.find('.')

        if i >= 0:
            dep = dep[: dep.find('.')]

        if 'Qt' in dep and not 'Qt5' in dep:
            dep = dep.replace('Qt', 'Qt5')

        return dep

    def solvedepsPlugins(self):
        print('\nCopying required plugins\n')

        pluginsMap = {
            'Qt53DRenderer': ['sceneparsers'],
            'Qt5Declarative': ['qml1tooling'],
            'Qt5EglFSDeviceIntegration': ['egldeviceintegrations'],
            'Qt5Gui': ['accessible', 'generic', 'iconengines', 'imageformats', 'platforms', 'platforminputcontexts'],
            'Qt5Location': ['geoservices'],
            'Qt5Multimedia': ['audio', 'mediaservice', 'playlistformats'],
            'Qt5Network': ['bearer'],
            'Qt5Positioning': ['position'],
            'Qt5PrintSupport': ['printsupport'],
            'Qt5QmlTooling': ['qmltooling'],
            'Qt5Quick': ['scenegraph', 'qmltooling'],
            'Qt5Sensors': ['sensors', 'sensorgestures'],
            'Qt5SerialBus': ['canbus'],
            'Qt5Sql': ['sqldrivers'],
            'Qt5TextToSpeech': ['texttospeech'],
            'Qt5WebEngine': ['qtwebengine'],
            'Qt5WebEngineCore': ['qtwebengine'],
            'Qt5WebEngineWidgets': ['qtwebengine'],
            'Qt5XcbQpa': ['xcbglintegrations']
        }

        pluginsMap.update({lib + 'd': pluginsMap[lib] for lib in pluginsMap})
        qtDeps = set()

        for machPath in self.findMachs(self.appInstallPath):
            for dep in self.listDependencies(machPath):
                if self.libName(dep) in pluginsMap:
                    qtDeps.add(dep)

        solved = set()
        plugins = []
        pluginsPath = os.path.join(self.appInstallPath, 'Contents/PlugIns')

        while len(qtDeps) > 0:
            dep = qtDeps.pop()

            for qtDep in self.listDependencies(dep):
                if self.libName(qtDep) in pluginsMap and not qtDep in solved:
                    qtDeps.add(qtDep)

            for plugin in pluginsMap[self.libName(dep)]:
                if not plugin in plugins:
                    sysPluginPath = os.path.join(self.sysPluginsPath, plugin)
                    pluginPath = os.path.join(pluginsPath, plugin)
                    print('    {} -> {}'.format(sysPluginPath, pluginPath))

                    if not os.path.exists(pluginsPath):
                        os.makedirs(pluginsPath)

                    try:
                        shutil.copytree(sysPluginPath, pluginPath, True)
                    except:
                        pass

                    for machPath in self.findMachs(sysPluginPath):
                        for machDep in self.listDependencies(machPath):
                            if self.libName(machDep) in pluginsMap \
                               and machDep != dep \
                               and not machDep in solved:
                                qtDeps.add(machDep)

                    plugins.append(plugin)

            solved.add(dep)

    def isExcluded(self, path):
        if path.startswith('/usr/lib') \
           or path.startswith('/System/Library/Frameworks'):
            return True

        return False

    def solvedepsLibs(self):
        print('\nCopying required libs\n')
        deps = set()

        for machPath in self.findMachs(self.appInstallPath):
            for dep in self.listDependencies(machPath):
                if len(dep) > 0:
                    deps.add(dep)

        _deps = set()

        for dep in deps:
            if not self.isExcluded(dep):
                _deps.add(dep)

        solved = set()
        frameworksInstallPath = os.path.join(self.appInstallPath,
                                             'Contents/Frameworks')

        while len(_deps) > 0:
            dep = _deps.pop()
            basename = os.path.basename(dep)
            framework = ''
            frameworkPath = ''

            if not basename.endswith('.dylib'):
                frameworkPath = dep[: dep.rfind('.framework')] + '.framework'
                framework = os.path.basename(frameworkPath)

            if len(frameworkPath) < 1:
                libPath = os.path.join(frameworksInstallPath, basename)
                print('    {} -> {}'.format(dep, libPath))

                self.copy(dep, libPath)
            else:
                libPath = os.path.join(frameworksInstallPath, framework)
                print('    {} -> {}'.format(frameworkPath, libPath))

                if not os.path.exists(frameworksInstallPath):
                    os.makedirs(frameworksInstallPath)

                try:
                    shutil.copytree(frameworkPath, libPath, True)
                except:
                    pass

            for machDep in self.listDependencies(dep):
                if len(machDep) > 0 \
                   and machDep != dep \
                   and not machDep in solved \
                   and not self.isExcluded(machDep):
                    _deps.add(machDep)

            solved.add(dep)

    def whereBin(self, binary):
        if not 'PATH' in os.environ or len(os.environ['PATH']) < 1:
            return ''

        for path in os.environ['PATH'].split(':'):
            path = os.path.join(path.strip(), binary)

            if os.path.exists(path):
                return path

        return ''

    def solvedeps(self):
        self.solvedepsQml()
        self.solvedepsPlugins()
        self.solvedepsLibs()

    def writeQtConf(self):
        print('Writting qt.conf file')

        paths = {'Plugins': '../Plugins',
                 'Imports': '../Resources/qml',
                 'Qml2Imports': '../Resources/qml'}

        with open(os.path.join(self.appInstallPath, 'Contents/Resources/qt.conf'), 'w') as qtconf:
            qtconf.write('[Paths]\n')

            for path in paths:
                qtconf.write('{} = {}\n'.format(path, paths[path]))

    def removeUnneededFiles(self, path):
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

    def strip(self, binary):
        process = subprocess.Popen(['strip', binary],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        process.communicate()

    def stripSymbols(self):
        print('Stripping symbols')
        path = os.path.join(self.appInstallPath, 'Contents')
        threads = []

        for mach in self.findMachs(path):
            thread = threading.Thread(target=self.strip, args=(mach,))
            threads.append(thread)

            while threading.active_count() >= self.njobs:
                time.sleep(0.25)

            thread.start()

        for thread in threads:
            thread.join()

    def resetFilePermissions(self):
        print('Resetting file permissions')
        rootPath = os.path.join(self.appInstallPath, 'Contents')
        binariesPath = os.path.join(rootPath, 'MacOS')

        for root, dirs, files in os.walk(rootPath):
            for d in dirs:
                os.chmod(os.path.join(root, d), 0o755, follow_symlinks=False)

            for f in files:
                permissions = 0o644

                if root == binariesPath:
                    permissions = 0o744

                os.chmod(os.path.join(root, f), permissions, follow_symlinks=False)

    def fixLibRpath(self, mutex, mach):
        path = os.path.join(self.appInstallPath, 'Contents')
        binariesPath = os.path.join(path, 'MacOS')
        frameworksPath = os.path.join(path, 'Frameworks')
        rpath = os.path.join('@executable_path', os.path.relpath(frameworksPath, binariesPath))
        log = '\tFixed {}\n\n'.format(mach)
        machInfo = self.machDump(mach)

        # Change rpath
        if mach.startswith(binariesPath):
            log += '\t\tChanging rpath to {}\n'.format(rpath)

            for oldRpath in machInfo['rpaths']:
                process = subprocess.Popen(['install_name_tool',
                                            '-delete_rpath', oldRpath, mach],
                                           stdout=subprocess.PIPE)
                process.communicate()

            process = subprocess.Popen(['install_name_tool',
                                        '-add_rpath', rpath, mach],
                                       stdout=subprocess.PIPE)
            process.communicate()

        # Change ID
        if mach.startswith(binariesPath):
            newMachId = machInfo['id']
        elif mach.startswith(frameworksPath):
            newMachId = mach.replace(frameworksPath, rpath)
        else:
            newMachId = os.path.basename(mach)

        if newMachId != machInfo['id']:
            log += '\t\tChanging ID to {}\n'.format(newMachId)

            process = subprocess.Popen(['install_name_tool',
                                        '-id', newMachId, mach],
                                       stdout=subprocess.PIPE)
            process.communicate()

        # Change library links
        for dep in machInfo['imports']:
            if dep.startswith(rpath):
                continue

            if self.isExcluded(dep):
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

                process = subprocess.Popen(['install_name_tool',
                                            '-change', dep, newDepPath, mach],
                                           stdout=subprocess.PIPE)
                process.communicate()

        mutex.acquire()
        print(log)
        mutex.release()

    def fixRpaths(self):
        print('Fixing rpaths\n')
        path = os.path.join(self.appInstallPath, 'Contents')
        mutex = threading.Lock()
        threads = []

        for mach in self.findMachs(path):
            thread = threading.Thread(target=self.fixLibRpath, args=(mutex, mach,))
            threads.append(thread)

            while threading.active_count() >= self.njobs:
                time.sleep(0.25)

            thread.start()

        for thread in threads:
            thread.join()

    def finish(self):
        print('\nCompleting final package structure\n')
        self.writeQtConf()
        print('Removing unnecessary files')
        self.removeUnneededFiles(os.path.join(self.appInstallPath, 'Contents/Frameworks'))
        self.stripSymbols()
        self.resetFilePermissions()
        self.fixRpaths()

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

    def dirSize(self, path):
        size = 0

        for root, dirs, files in os.walk(path):
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

        try:
            shutil.copytree(self.appInstallPath,
                            os.path.join(staggingDir, self.programName + '.app'),
                            True)
        except:
            pass

        imageSize = self.dirSize(staggingDir)
        tmpDmg = os.path.join(self.installDir, self.programName + '_tmp.dmg')
        volumeName = "{}-portable-{}".format(self.programName,
                                    self.programVersion)

        process = subprocess.Popen(['hdiutil', 'create',
                                    '-srcfolder', staggingDir,
                                    '-volname', volumeName,
                                    '-fs', 'HFS+',
                                    '-fsargs', '-c c=64,a=16,e=16',
                                    '-format', 'UDRW',
                                    '-size', str(math.ceil(imageSize * 1.1)),
                                    tmpDmg],
                                   stdout=subprocess.PIPE)
        process.communicate()

        process = subprocess.Popen(['hdiutil',
                                    'attach',
                                    '-readwrite',
                                    '-noverify',
                                    tmpDmg],
                                   stdout=subprocess.PIPE)
        stdout, stderr = process.communicate()
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
        self.copy(os.path.join(self.rootDir, 'StandAlone/share/icons/webcamoid.icns'),
                  volumeIcon)

        process = subprocess.Popen(['SetFile',
                                    '-c', 'icnC',
                                    volumeIcon],
                                   stdout=subprocess.PIPE)
        process.communicate()

        process = subprocess.Popen(['SetFile',
                                    '-a', 'C',
                                    volumePath],
                                   stdout=subprocess.PIPE)
        process.communicate()

        appsShortcut = os.path.join(volumePath, 'Applications')

        if not os.path.exists(appsShortcut):
            os.symlink('/Applications', appsShortcut)

        os.sync()

        process = subprocess.Popen(['hdiutil',
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

        process = subprocess.Popen(['hdiutil',
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

                            print(changeLogText)
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

        appIconSrc = os.path.join(self.rootDir,
                                  'StandAlone/share/icons/{}.icns'.format(self.programName))
        self.copy(appIconSrc, configDir)
        self.copy(os.path.join(self.rootDir, 'COPYING'),
                  os.path.join(metaDir, 'COPYING.txt'))

        try:
            shutil.copytree(self.appInstallPath,
                            os.path.join(dataDir, self.programName + '.app'),
                            True)
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
            config.write('    <RunProgram>@TargetDir@/{0}.app/Contents/MacOS/{0}</RunProgram>\n'.format(self.programName))
            config.write('    <RunProgramDescription>Launch Webcamoid now!</RunProgramDescription>\n')
            config.write('    <StartMenuDir>Webcamoid</StartMenuDir>\n')
            config.write('    <MaintenanceToolName>WebcamoidMaintenanceTool</MaintenanceToolName>\n')
            config.write('    <AllowNonAsciiCharacters>true</AllowNonAsciiCharacters>\n')
            config.write('    <TargetDir>@ApplicationsDir@/{}</TargetDir>\n'.format(self.programName))
            config.write('</Installer>\n')

        self.copy(os.path.join(self.rootDir, 'ports/deploy/installscript.mac.qs'),
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
                                   '{}-{}.dmg'.format(self.programName,
                                                      self.programVersion))

        if not os.path.exists(self.pkgsDir):
            os.makedirs(self.pkgsDir)

        if os.path.exists(packagePath):
            os.remove(packagePath)

        process = subprocess.Popen([self.qtIFW,
                                    '-c', configXml,
                                    '-p', os.path.join(self.installDir,
                                                       'installer/packages'),
                                    packagePath],
                                   cwd=os.path.join(self.installDir, 'installer'),
                                   stdout=subprocess.PIPE)
        process.communicate()
        shutil.rmtree(packagePath.replace('.dmg', '.app'), True)

        if not os.path.exists(packagePath):
            return

        mutex.acquire()
        print('Created installable package:')
        self.printPackageInfo(packagePath)
        mutex.release()

    def package(self):
        print('\nCreating packages\n')
        mutex = threading.Lock()

        threads = [threading.Thread(target=self.createPortable, args=(mutex,)),
                   threading.Thread(target=self.createInstaller, args=(mutex,))]

        for thread in threads:
            thread.start()

        for thread in threads:
            thread.join()

    def cleanup(self):
        shutil.rmtree(os.path.join(self.rootDir, 'ports/deploy/temp_priv'),
                      True)
