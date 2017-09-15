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

import mimetypes
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
        self.scanPaths = ['StandAlone\\webcamoid.exe',
                          'StandAlone\\share\\qml',
                          'libAvKys\\Plugins']
        self.rootDir = rootDir
        self.buildDir = os.environ['BUILD_DIR'] if 'BUILD_DIR' in os.environ else rootDir
        self.system = system
        self.arch = arch
        self.targetSystem = system
        self.targetArch = self.detectArch()
        self.qmake = self.detectQmake()
        self.programVersion = self.readVersion()
        self.make = self.detectMake()

    def detectArch(self):
        exeFile = os.path.join(self.buildDir, self.scanPaths[0])

        return platform.architecture(exeFile)[0]

    def readVersion(self):
        self.sysBinsPath = self.qmakeQuery('QT_INSTALL_BINS')
        os.environ['PATH'] = ';'.join([os.path.join(self.buildDir, 'libAvKys\\Lib'),
                                       self.sysBinsPath,
                                       os.environ['PATH']])
        programPath = os.path.join(self.buildDir, self.scanPaths[0])
        process = subprocess.Popen([programPath, '--version'],
                                   stdout=subprocess.PIPE)
        stdout, stderr = process.communicate()

        try:
            return stdout.split()[1].strip().decode('utf-8')
        except:
            return 'unknown'

    def detectQmake(self):
        with open(os.path.join(self.buildDir, 'StandAlone\\Makefile')) as f:
            for line in f:
                if line.startswith('QMAKE'):
                    return line.split('=')[1].strip()

        return ''

    def detectMake(self):
        if 'MAKE_PATH' in os.environ:
            return os.environ['MAKE_PATH']

        defaultPath = os.path.dirname(self.qmake)
        makePaths = [os.path.join(defaultPath, 'mingw32-make.exe'),
                     os.path.join(defaultPath, 'make.exe'),
                     'C:\\MinGW\\bin\\mingw32-make.exe']

        for path in makePaths:
            if os.path.exists(path):
                return path

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
        self.installDir = os.environ['INSTALL_PREFIX'] if 'INSTALL_PREFIX' in os.environ else os.path.join(self.buildDir, 'ports\\deploy\\temp_priv\\root')
        previousDir = os.getcwd()
        os.chdir(self.buildDir)

        process = subprocess.Popen([self.make, 'install'],
                                   stdout=subprocess.PIPE)

        stdout, stderr = process.communicate()
        os.chdir(previousDir)

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

        qmlPath = os.path.join(self.installDir, 'lib\\qt\\qml')
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

    def isExe(self, path):
        mimetype, encoding = mimetypes.guess_type(path)

        if mimetype == 'application/x-msdownload':
            return True

        return False

    def findExes(self, path):
        exes = []

        for root, dirs, files in os.walk(path):
            for f in files:
                exePath = os.path.join(root, f)

                if not os.path.islink(exePath) and self.isExe(exePath):
                    exes.append(exePath)

        return exes

    def whereExe(self, exe):
        if not 'PATH' in os.environ or len(os.environ['PATH']) < 1:
            return ''

        for path in os.environ['PATH'].split(';'):
            path = os.path.join(path.strip(), exe)

            if os.path.exists(path):
                return path

        return ''

    # https://msdn.microsoft.com/en-us/library/windows/desktop/ms680547(v=vs.85).aspx
    # https://upload.wikimedia.org/wikipedia/commons/1/1b/Portable_Executable_32_bit_Structure_in_SVG_fixed.svg
    def exeDump(self, exe):
        dllImports = []

        with open(exe, 'rb') as f:
            # Check DOS header signature.
            if f.read(2) != b'MZ':
                return []

            # Move to COFF header.
            f.seek(0x3c, os.SEEK_SET)
            peHeaderOffset = struct.unpack('I', f.read(4))
            f.seek(peHeaderOffset[0], os.SEEK_SET)
            peSignatue = f.read(4)

            # Check COFF header signature.
            if peSignatue != b'PE\x00\x00':
                return []

            # Read COFF header.
            coffHeader = struct.unpack('HHIIIHH', f.read(20))
            nSections = coffHeader[1]
            sectionTablePos = coffHeader[5] + f.tell()

            # Read magic signature in standard COFF fields.
            peType = 'PE32' if f.read(2) == b'\x0b\x01' else 'PE32+'

            # Move to data directories.
            f.seek(102 if peType == 'PE32' else 118, os.SEEK_CUR)

            # Read the import table.
            importTablePos, importTableSize = struct.unpack('II', f.read(8))

            # Move to Sections table.
            f.seek(sectionTablePos, os.SEEK_SET)
            sections = []
            idataTableVirtual = -1
            idataTablePhysical = -1

            # Search for 'idata' section.
            for i in range(nSections):
                # Read section.
                section = struct.unpack('8pIIIIIIHHI', f.read(40))
                sectionName = section[0].replace(b'\x00', b'')

                # Save a reference to the sections.
                sections += [section]

                if sectionName == b'idata':
                    idataTableVirtual = section[2]
                    idataTablePhysical = section[4]

                    # If import table was defined calculate it's position in
                    # the file in relation to the address given by 'idata'.
                    if importTableSize > 0:
                        idataTablePhysical += importTablePos - section[2]

            if idataTablePhysical < 0:
                return []

            # Move to 'idata' section.
            f.seek(idataTablePhysical, os.SEEK_SET)
            dllList = set()

            # Read 'idata' directory table.
            while True:
                # Read DLL entries.
                dllImport = struct.unpack('IIIII', f.read(20))

                # Null directory entry.
                if dllImport[0] | dllImport[1] | dllImport[2] | dllImport[3] | dllImport[4] == 0:
                    break

                # Locate where is located the DLL name in relation to the
                # sections.
                for section in sections:
                    if dllImport[3] >= section[2] \
                        and dllImport[3] < section[1] + section[2]:
                        dllList.add(dllImport[3] - section[2] + section[4])

                        break

            for dll in dllList:
                # Move to DLL name.
                f.seek(dll, os.SEEK_SET)
                dllName = b''

                # Read string until null character.
                while True:
                    c = f.read(1)

                    if c == b'\x00':
                        break

                    dllName += c

                dllImports.append(dllName.decode(sys.getdefaultencoding()))

        return dllImports

    def listDependencies(self, path):
        libs = []

        for exe in self.exeDump(path):
            lib = self.whereExe(exe)
            dirname = os.path.dirname(lib).replace('/', '\\').lower()

            if len(lib) > 0 \
                and os.path.exists(lib) \
                and dirname != 'c:\\windows\\system32':
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

        for exePath in self.findExes(self.installDir):
            for dep in self.listDependencies(exePath):
                if self.libName(dep) in pluginsMap:
                    qtDeps.add(dep)

        solved = set()
        plugins = []
        pluginsPath = os.path.join(self.installDir, 'lib/qt/plugins')

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

                    for elfPath in self.findExes(sysPluginPath):
                        for exeDep in self.listDependencies(elfPath):
                            if self.libName(exeDep) in pluginsMap \
                               and exeDep != dep \
                               and not exeDep in solved:
                                qtDeps.add(exeDep)

                    plugins.append(plugin)

            solved.add(dep)

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
            path = self.whereExe(dep)

            if len(path) > 0 and os.path.exists(path):
                deps.add(path)

        for elfPath in self.findExes(self.installDir):
            for dep in self.listDependencies(elfPath):
                deps.add(dep)

        solved = set()

        while len(deps) > 0:
            dep = deps.pop()

            libPath = os.path.join(self.installDir, 'bin', os.path.basename(dep))
            print('    {} -> {}'.format(dep, libPath))

            self.copy(dep, libPath)

            for exeDep in self.listDependencies(dep):
                if exeDep != dep and not exeDep in solved:
                    deps.add(exeDep)

            solved.add(dep)

    def solvedeps(self):
        self.solvedepsQml()
        self.solvedepsPlugins()
        self.solvedepsLibs()

    def writeQtConf(self):
        print('Writting qt.conf file')

        paths = {'Plugins': '../lib/qt/plugins',
                 'Imports': '../lib/qt/qml',
                 'Qml2Imports': '../lib/qt/qml'}

        with open(os.path.join(self.installDir, 'bin/qt.conf'), 'w') as qtconf:
            qtconf.write('[Paths]\n')

            for path in paths:
                qtconf.write('{} = {}\n'.format(path, paths[path]))

    def createLauncher(self):
        print('Writting launcher file')

        with open(os.path.join(self.installDir, 'webcamoid.bat'), 'w') as launcher:
            launcher.write('@echo off\n')
            launcher.write('\n')
            launcher.write('rem Default values: desktop | angle | software\n')
            launcher.write('rem set QT_OPENGL=angle\n')
            launcher.write('\n')
            launcher.write('rem Default values: d3d11 | d3d9 | warp\n')
            launcher.write('rem set QT_ANGLE_PLATFORM=d3d11\n')
            launcher.write('\n')
            launcher.write('rem Default values: software | d3d12 | openvg\n')
            launcher.write('rem set QT_QUICK_BACKEND=""\n')
            launcher.write('\n')
            launcher.write('start /b "" "%~dp0bin\\webcamoid" -q "%~dp0lib\\qt\\qml" -p "%~dp0lib\\avkys" -c "%~dp0share\\config"\n')

    def removeUnneededFiles(self, path):
        afiles = set()

        for root, dirs, files in os.walk(path):
            for f in files:
                if f.endswith('.a') \
                    or f.endswith('.static.prl') \
                    or f.endswith('.pdb') \
                    or f.endswith('.lib'):
                    afiles.add(os.path.join(root, f))

        for afile in afiles:
            os.remove(afile)

    def strip(self, strip, binary):
        process = subprocess.Popen([strip, binary],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        process.communicate()

    def stripSymbols(self):
        strip = self.whereExe('strip.exe')

        if len(strip) < 1:
            return

        print('Stripping symbols')
        path = os.path.join(self.installDir)
        threads = []

        for exe in self.findExes(path):
            thread = threading.Thread(target=self.strip, args=(strip, exe,))
            threads.append(thread)

            while threading.active_count() >= 64:
                time.sleep(0.25)

            thread.start()

        for thread in threads:
            thread.join()

    def finish(self):
        print('\nCompleting final package structure\n')
        self.writeQtConf()
        self.createLauncher()
        self.stripSymbols()
        print('Removing unnecessary files')
        self.removeUnneededFiles(self.installDir)

    def package(self):
        pass

    def cleanup(self):
        pass
