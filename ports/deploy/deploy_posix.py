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
    def __init__(self, rootDir, system, arch):
        self.scanPaths = ['StandAlone/webcamoid',
                          'StandAlone/share/qml',
                          'libAvKys/Plugins']
        self.rootDir = rootDir
        self.system = system
        self.arch = arch
        self.targetArch = self.arch
        self.targetSystem = self.detectSystem()
        self.programVersion = self.readVersion()
        self.qmake = self.detectQmake()
        self.excludes = self.readExcludeList()

    def detectSystem(self):
        exeFile = os.path.join(self.rootDir, self.scanPaths[0] + '.exe')

        return 'windows' if os.path.exists(exeFile) else self.system

    def readVersion(self):
        if self.targetSystem != 'posix':
            return ''

        os.environ['LD_LIBRARY_PATH'] = os.path.join(self.rootDir, 'libAvKys/Lib')
        programPath = os.path.join(self.rootDir, self.scanPaths[0])
        process = subprocess.Popen([programPath, '--version'],
                                   stdout=subprocess.PIPE)
        stdout, stderr = process.communicate()

        try:
            return stdout.split()[1].strip().decode(sys.getdefaultencoding())
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

    def prepare(self):
        self.sysQmlPath = self.qmakeQuery('QT_INSTALL_QML')
        self.sysPluginsPath = self.qmakeQuery('QT_INSTALL_PLUGINS')
        self.installDir = os.path.join(self.rootDir, 'ports/deploy/temp_priv/root')
        insQmlDir = os.path.join(self.installDir, self.sysQmlPath)
        dstQmlDir = os.path.join(self.installDir, 'usr/lib/qt/qml')

        previousDir = os.getcwd()
        os.chdir(self.rootDir)
        process = subprocess.Popen(['make', 'INSTALL_ROOT={}'.format(self.installDir), 'install'],
                                   stdout=subprocess.PIPE)
        process.communicate()
        os.chdir(previousDir)

        if process.returncode != 0:
            return

        if insQmlDir != dstQmlDir:
            if not os.path.exists(dstQmlDir):
                os.makedirs(dstQmlDir)

            try:
                shutil.copy(insQmlDir, dstQmlDir)
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

        qmlPath = os.path.join(self.installDir, 'usr/lib/qt/qml')
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

    def isElf(self, path):
        process = subprocess.Popen(['file', '-bi', path],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()

        if process.returncode != 0:
            return False

        if b'application/x-sharedlib' in stdout:
            return True

        return False

    def findElfs(self, path):
        elfs = []

        for root, dirs, files in os.walk(path):
            for f in files:
                elfPath = os.path.join(root, f)

                if self.isElf(elfPath):
                    elfs.append(elfPath)

        return elfs

    def listDependencies(self, path):
        process = subprocess.Popen(['ldd', path],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()

        if process.returncode != 0:
            return []

        libs = []

        for line in stdout.decode(sys.getdefaultencoding()).split('\n'):
            if '=>' in line:
                lib = line.split('=>')[1]
                i = lib.rfind('(')

                if i >= 0:
                    lib = lib[: i]

                lib = lib.strip()

                if os.path.exists(lib):
                    libs.append(lib)

        return libs

    def libName(self, lib):
        dep = os.path.basename(lib)[3:]

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

        qtDeps = set()

        for elfPath in self.findElfs(self.installDir):
            for dep in self.listDependencies(elfPath):
                if self.libName(dep) in pluginsMap:
                    qtDeps.add(dep)

        solved = set()
        plugins = []

        while len(qtDeps) > 0:
            dep = qtDeps.pop()

            for qtDep in self.listDependencies(dep):
                if self.libName(qtDep) in pluginsMap and not qtDep in solved:
                    qtDeps.add(qtDep)

            for plugin in pluginsMap[self.libName(dep)]:
                if not plugin in plugins:
                    sysPluginPath = os.path.join(self.sysPluginsPath, plugin)
                    pluginPath = os.path.join(self.installDir, 'usr/lib/qt/plugins')
                    print('    {} -> {}'.format(sysPluginPath, pluginPath))

                    if not os.path.exists(pluginPath):
                        os.makedirs(pluginPath)

                    try:
                        shutil.copytree(sysPluginPath, os.path.join(pluginPath, plugin))
                    except:
                        pass

                    for elfPath in self.findElfs(sysPluginPath):
                        for elfDep in self.listDependencies(elfPath):
                            if self.libName(elfDep) in pluginsMap \
                               and elfDep != dep \
                               and not elfDep in solved:
                                qtDeps.add(elfDep)

                    plugins.append(plugin)

            solved.add(dep)

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

            if not os.path.exists(libPath):
                shutil.copy(dep, libPath)

            for elfDep in self.listDependencies(dep):
                if elfDep != dep and not elfDep in solved:
                    _deps.add(elfDep)

            solved.add(_dep)

    def solvedeps(self):
        self.solvedepsQml()
        self.solvedepsPlugins()
        self.solvedepsLibs()

    def package(self):
        pass

    def finish(self):
        pass
