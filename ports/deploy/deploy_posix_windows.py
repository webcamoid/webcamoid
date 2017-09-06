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
import subprocess
import shutil
import re

class Deploy:
    def __init__(self, rootDir, system, targetSystem, arch):
        self.scanPaths = ['StandAlone/webcamoid.exe',
                          'StandAlone/share/qml',
                          'libAvKys/Plugins']
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

    def qmakeQuery(self, var):
        process = subprocess.Popen([self.qmake, '-query', var],
                                   stdout=subprocess.PIPE)
        stdout, stderr = process.communicate()

        return stdout.strip().decode(sys.getdefaultencoding())

    def removeUnneededFiles(self, path):
        afiles = set()

        for root, dirs, files in os.walk(path):
            for f in files:
                if f.endswith('.a') or f.endswith('.static.prl'):
                    afiles.add(os.path.join(root, f))

        for afile in afiles:
            os.remove(afile)

    def prepare(self):
        self.sysBinsPath = self.qmakeQuery('QT_INSTALL_BINS')
        self.sysQmlPath = self.qmakeQuery('QT_INSTALL_QML')
        self.sysPluginsPath = self.qmakeQuery('QT_INSTALL_PLUGINS')
        self.installDir = os.path.join(self.rootDir, 'ports/deploy/temp_priv/root')

        previousDir = os.getcwd()
        os.chdir(self.rootDir)
        process = subprocess.Popen(['make', 'INSTALL_ROOT={}'.format(self.installDir), 'install'],
                                   stdout=subprocess.PIPE)
        process.communicate()
        os.chdir(previousDir)

        if process.returncode != 0:
            return

        self.removeUnneededFiles(os.path.join(self.installDir, 'webcamoid/bin'))

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

        qmlPath = os.path.join(self.installDir, 'webcamoid/lib/qt/qml')
        print(qmlPath)
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
                        shutil.copytree(sysModulePath, installModulePath)
                    except:
                        pass

                    solvedImports.add(imp)

                    for f in self.listQmlFiles(sysModulePath):
                        if not f in solved:
                            qmlFiles.add(f)

            solved.add(qmlFile)

        self.removeUnneededFiles(qmlPath)

    def isExe(self, path):
        process = subprocess.Popen(['file', '-bi', path],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()

        if process.returncode != 0:
            return False

        if b'application/x-dosexec' in stdout:
            return True

        return False

    def findExes(self, path):
        elfs = []

        for root, dirs, files in os.walk(path):
            for f in files:
                elfPath = os.path.join(root, f)

                if self.isExe(elfPath):
                    elfs.append(elfPath)

        return elfs

    def listDependencies(self, path):
        process = subprocess.Popen([os.path.join(self.sysBinsPath, 'objdump'), '-x', path],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()

        if process.returncode != 0:
            return []

        libs = []

        for line in stdout.decode(sys.getdefaultencoding()).split('\n'):
            if re.search('DLL Name:', line):
                lib = line.split('DLL Name:')[1]
                lib = os.path.join(self.sysBinsPath, lib.strip())

                if os.path.exists(lib):
                    libs.append(lib)

        return libs

    def libName(self, lib):
        dep = os.path.basename(lib)

        return dep[: dep.find('.')]

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

        for elfPath in self.findExes(self.installDir):
            for dep in self.listDependencies(elfPath):
                if self.libName(dep) in pluginsMap:
                    qtDeps.add(dep)

        solved = set()
        plugins = []
        pluginsPath = os.path.join(self.installDir, 'webcamoid/bin')

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
                        shutil.copytree(sysPluginPath, pluginPath)
                    except:
                        pass

                    for elfPath in self.findExes(sysPluginPath):
                        for exeDep in self.listDependencies(elfPath):
                            if self.libName(exeDep) in pluginsMap \
                               and exeDep != dep \
                               and not exeDep in solved:
                                qtDeps.add(exeDep)

                    plugins.append(plugin)

            solved.add(dep)

        self.removeUnneededFiles(pluginsPath)

    def solvedepsLibs(self):
        print('\nCopying required libs\n')
        deps = set()
        extraDeps = ['libeay32.dll',
                     'ssleay32.dll',
                     'libEGL.dll',
                     'libGLESv2.dll',
                     'D3DCompiler_43.dll',
                     'D3DCompiler_46.dll',
                     'D3DCompiler_47.dll']

        for dep in extraDeps:
            path = os.path.join(self.sysBinsPath, dep)

            if os.path.exists(path):
                deps.add(path)

        for elfPath in self.findExes(self.installDir):
            for dep in self.listDependencies(elfPath):
                deps.add(dep)

        solved = set()

        while len(deps) > 0:
            dep = deps.pop()

            libPath = os.path.join(self.installDir, 'webcamoid/bin', os.path.basename(dep))
            print('    {} -> {}'.format(dep, libPath))

            if not os.path.exists(libPath):
                shutil.copy(dep, libPath)

            for exeDep in self.listDependencies(dep):
                if exeDep != dep and not exeDep in solved:
                    deps.add(exeDep)

            solved.add(dep)

    def solvedeps(self):
        self.solvedepsQml()
        self.solvedepsPlugins()
        self.solvedepsLibs()

    def finish(self):
        pass

    def package(self):
        pass

    def cleanup(self):
        pass
