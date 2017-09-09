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
import struct
import platform
import subprocess
import shutil
import re

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
        with open(os.path.join(self.rootDir, 'StandAlone/Makefile')) as f:
            for line in f:
                if line.startswith('QMAKE'):
                    return line.split('=')[1].strip()

        return ''

    def qmakeQuery(self, var):
        process = subprocess.Popen([self.qmake, '-query', var],
                                   stdout=subprocess.PIPE)
        stdout, stderr = process.communicate()

        return stdout.strip().decode(sys.getdefaultencoding())

    def copy(self, src, dst):
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
        tempDir = os.path.join(self.rootDir, 'ports/deploy/temp_priv')

        if not os.path.exists(tempDir):
            os.makedirs(tempDir)

        self.appInstallPath = os.path.join(tempDir, os.path.basename(self.appPath))

        if not os.path.exists(self.appInstallPath):
            shutil.copytree(self.appPath, self.appInstallPath, True)

        contents = os.path.join(self.appInstallPath, 'Contents')
        installDir = os.path.join(self.rootDir, 'ports/deploy/temp_priv/root')

        previousDir = os.getcwd()
        os.chdir(os.path.join(self.rootDir, 'libAvkys'))
        process = subprocess.Popen(['make',
                                    'INSTALL_ROOT={}'.format(installDir),
                                    'install'],
                                   stdout=subprocess.PIPE)
        process.communicate()
        os.chdir(previousDir)

        if process.returncode != 0:
            return

        frameworksPath = os.path.join(contents, 'Frameworks')

        if not os.path.exists(frameworksPath):
            os.makedirs(frameworksPath)

        installLibDir = os.path.join(installDir, 'usr/lib')

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

        if not 'FRAMEWORKS_PATH' in os.environ:
            return ''

        basename = os.path.basename(path)

        for fpath in os.environ['FRAMEWORKS_PATH'].split(':'):
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

    def stripSymbols(self):
        print('Stripping symbols')
        path = os.path.join(self.appInstallPath, 'Contents')

        for mach in self.findMachs(path):
            process = subprocess.Popen(['strip', mach],
                                       stdout=subprocess.PIPE)
            process.communicate()

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

    def fixRpaths(self):
        print('Fixing rpaths')
        path = os.path.join(self.appInstallPath, 'Contents')
        binariesPath = os.path.join(path, 'MacOS')
        frameworksPath = os.path.join(path, 'Frameworks')
        rpath = os.path.join('@executable_path', os.path.relpath(frameworksPath, binariesPath))

        for mach in self.findMachs(path):
            print(4 * ' ' + 'Fixing ' + mach)
            print()

            machInfo = self.machDump(mach)

            # Change rpath
            if mach.startswith(binariesPath):
                print(8 * ' ' + 'Changing rpath to {}'.format(rpath))

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
                print(8 * ' ' + 'Changing ID to {}'.format(newMachId))

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
                    print(8 * ' ' + '{} -> {}'.format(dep, newDepPath))

                    process = subprocess.Popen(['install_name_tool',
                                                '-change', dep, newDepPath, mach],
                                               stdout=subprocess.PIPE)
                    process.communicate()

            print()

    def finish(self):
        print('\nCompleting final package structure\n')
        self.writeQtConf()
        print('Removing unnecessary files')
        self.removeUnneededFiles(os.path.join(self.appInstallPath, 'Contents/Frameworks'))
        self.stripSymbols()
        self.resetFilePermissions()
        self.fixRpaths()

    def package(self):
        pass

    def cleanup(self):
        pass
