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

import fnmatch
import os
import re
import shutil
import subprocess
import sys

import tools.utils

class DeployToolsQt(tools.utils.DeployToolsUtils):
    def __init__(self):
        super().__init__()
        self.qmake = ''
        self.qtIFW = ''
        self.qtInstallBins = ''
        self.qtInstallQml = ''
        self.qtInstallPlugins = ''
        self.qmlRootDirs = []
        self.qmlInstallDir = ''
        self.qtDependencies = []
        self.binarySolver = None

    def detectMakeFiles(self, makePath):
        makeFiles = []

        try:
            for f in os.listdir(makePath):
                path = os.path.join(makePath, f)

                if os.path.isfile(path) and fnmatch.fnmatch(f.lower(), 'makefile*'):
                    makeFiles += [path]
        except:
            pass

        return makeFiles

    def detectQt(self, path=''):
        self.detectQmake(path)
        self.qtInstallBins = self.qmakeQuery(var='QT_INSTALL_BINS')
        self.qtInstallQml = self.qmakeQuery(var='QT_INSTALL_QML')
        self.qtInstallPlugins = self.qmakeQuery(var='QT_INSTALL_PLUGINS')
        self.detectQtIFW()

    def detectQmake(self, path=''):
        for makeFile in self.detectMakeFiles(path):
            with open(makeFile) as f:
                for line in f:
                    if line.startswith('QMAKE') and '=' in line:
                        self.qmake = line.split('=')[1].strip()

                        return

        if 'QMAKE_PATH' in os.environ:
            self.qmake = os.environ['QMAKE_PATH']

    def qmakeQuery(self, qmake='', var=''):
        if qmake == '':
            if 'QMAKE_PATH' in os.environ:
                qmake = os.environ['QMAKE_PATH']
            else:
                qmake = self.qmake

        try:
            args = [qmake, '-query']

            if var != '':
                args += [var]

            process = subprocess.Popen(args,
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE)
            stdout, stderr = process.communicate()

            return stdout.strip().decode(sys.getdefaultencoding())
        except:
            pass

        return ''

    def detectVersion(self, proFile):
        verMaj = '0'
        verMin = '0'
        verPat = '0'

        try:
            with open(proFile) as f:
                for line in f:
                    if line.startswith('VER_MAJ') and '=' in line:
                        verMaj = line.split('=')[1].strip()
                    elif line.startswith('VER_MIN') and '=' in line:
                        verMin = line.split('=')[1].strip()
                    elif line.startswith('VER_PAT') and '=' in line:
                        verPat = line.split('=')[1].strip()
        except:
            pass

        return verMaj + '.' + verMin + '.' + verPat

    def detectQtIFW(self):
        if 'BINARYCREATOR' in os.environ:
            self.qtIFW = os.environ['BINARYCREATOR']

            return

        # Try official Qt binarycreator first because it is statically linked.

        if self.targetSystem == 'windows':
            homeQt = 'C:\\Qt'
        elif self.targetSystem == 'posix_windows':
            homeQt = os.path.expanduser('~/.wine/drive_c/Qt')
        else:
            homeQt = os.path.expanduser('~/Qt')

        binCreator = 'binarycreator'

        if self.targetSystem == 'windows' or self.targetSystem == 'posix_windows':
            binCreator += '.exe'

        for root, dirs, files in os.walk(os.path.expanduser(homeQt)):
            for f in files:
                if f == binCreator:
                    self.qtIFW = os.path.join(root, f)

                    return

        # binarycreator offered by the system is most probably dynamically
        # linked, so it's useful for test purposes only, but not recommended
        # for distribution.
        self.qtIFW = self.whereBin(binCreator)

    def listQmlFiles(self, path):
        qmlFiles = set()

        if os.path.isfile(path):
            baseName = os.path.basename(path)

            if baseName == 'qmldir' or path.endswith('.qml'):
                qmlFiles.add(path)
        else:
            for root, dirs, files in os.walk(path):
                for f in files:
                    if f == 'qmldir' or f.endswith('.qml'):
                        qmlFiles.add(os.path.join(root, f))

        return list(qmlFiles)

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

    def solvedepsQml(self):
        print('Copying Qml modules\n')
        qmlFiles = set()

        for path in self.qmlRootDirs:
            path = os.path.join(self.rootDir, path)

            for f in self.listQmlFiles(path):
                qmlFiles.add(f)

        solved = set()
        solvedImports = set()

        while len(qmlFiles) > 0:
            qmlFile = qmlFiles.pop()

            for imp in self.scanImports(qmlFile):
                if imp in solvedImports:
                    continue

                sysModulePath = os.path.join(self.qtInstallQml, imp)
                installModulePath = os.path.join(self.qmlInstallDir, imp)

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
                    self.qtDependencies.append(os.path.join(sysModulePath, 'qmldir'))

                    for f in self.listQmlFiles(sysModulePath):
                        if not f in solved:
                            qmlFiles.add(f)

            solved.add(qmlFile)

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

        for binPath in self.binarySolver.find(self.installDir):
            for dep in self.binarySolver.dependencies(binPath):
                if self.binarySolver.name(dep) in pluginsMap:
                    qtDeps.add(dep)

        solved = set()
        plugins = []

        while len(qtDeps) > 0:
            dep = qtDeps.pop()

            for qtDep in self.binarySolver.dependencies(dep):
                if self.binarySolver.name(qtDep) in pluginsMap and not qtDep in solved:
                    qtDeps.add(qtDep)

            for plugin in pluginsMap[self.binarySolver.name(dep)]:
                if not plugin in plugins:
                    sysPluginPath = os.path.join(self.qtInstallPlugins, plugin)
                    pluginPath = os.path.join(self.pluginsInstallDir, plugin)
                    print('    {} -> {}'.format(sysPluginPath, pluginPath))

                    if not os.path.exists(self.pluginsInstallDir):
                        os.makedirs(self.pluginsInstallDir)

                    try:
                        shutil.copytree(sysPluginPath, pluginPath, True)
                    except:
                        pass

                    for binPath in self.binarySolver.find(sysPluginPath):
                        for binDep in self.binarySolver.dependencies(binPath):
                            if self.binarySolver.name(binDep) in pluginsMap \
                               and binDep != dep \
                               and not binDep in solved:
                                qtDeps.add(binDep)

                    plugins.append(plugin)
                    self.qtDependencies.append(sysPluginPath)

            solved.add(dep)
