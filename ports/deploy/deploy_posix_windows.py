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

import math
import mimetypes
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
import zipfile


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
        self.wineCmd = 'wine64' if self.targetArch == '64bit' and shutil.which('wine64') else 'wine'
        self.qmake = self.detectQmake()
        self.qtIFW = self.detectQtIFW()
        self.qtIFWVersion = self.detectQtIFWVersion()
        self.programVersion = self.readVersion()
        self.installerIconSize = 256
        self.njobs = multiprocessing.cpu_count()

        if self.njobs < 4:
            self.njobs = 4

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

        subprocess.run(['wineconsole', '--backend=curses', versionBat],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        with open(os.path.join(tempDir, 'version.txt')) as f:
            version = f.readline();

        shutil.rmtree(tempDir)

        try:
            return version.split()[1].strip()
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

        for root, dirs, files in os.walk(os.path.expanduser('~/.wine/drive_c/Qt')):
            for f in files:
                if f == 'binarycreator.exe':
                    return os.path.join(root, f)

        return self.whereExe('binarycreator.exe')

    def detectQtIFWVersion(self):
        installerBase = os.path.join(os.path.dirname(self.qtIFW),
                                     'installerbase.exe')

        if not os.path.exists(installerBase):
            return '2.0.0'

        process = subprocess.Popen([self.wineCmd, installerBase,
                                    '--version'],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()

        for line in stdout.split(b'\n'):
            if b'IFW Version:' in line:
                return line.split(b' ')[2].replace(b'"', b'').decode(sys.getdefaultencoding())

        return version

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

    def prepare(self):
        self.sysBinsPath = self.qmakeQuery('QT_INSTALL_BINS')
        self.sysQmlPath = self.qmakeQuery('QT_INSTALL_QML')
        self.sysPluginsPath = self.qmakeQuery('QT_INSTALL_PLUGINS')
        self.installDir = os.path.join(self.rootDir, 'ports/deploy/temp_priv/root')
        self.pkgsDir = os.path.join(self.rootDir, 'ports/deploy/packages_auto/windows')

        previousDir = os.getcwd()
        os.chdir(self.rootDir)
        process = subprocess.Popen(['make', 'INSTALL_ROOT={}'.format(self.installDir), 'install'],
                                   stdout=subprocess.PIPE)
        process.communicate()
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
        path = os.path.join(self.sysBinsPath, exe)

        return path if os.path.exists(path) else ''

    # https://msdn.microsoft.com/en-us/library/windows/desktop/ms680547(v=vs.85).aspx
    # https://upload.wikimedia.org/wikipedia/commons/1/1b/Portable_Executable_32_bit_Structure_in_SVG_fixed.svg
    def exeDump(self, exe):
        dllImports = set()

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

                if sectionName == b'idata' or sectionName == b'rdata':
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

                dllImports.add(dllName.decode(sys.getdefaultencoding()))

        return dllImports

    def listDependencies(self, path):
        libs = []

        for exe in self.exeDump(path):
            lib = self.whereExe(exe)

            if len(lib) > 0:
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
        pluginsPath = os.path.join(self.installDir, 'webcamoid/lib/qt/plugins')

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

            self.copy(dep, libPath)

            for exeDep in self.listDependencies(dep):
                if exeDep != dep and not exeDep in solved:
                    deps.add(exeDep)

            solved.add(dep)

    def removeDebugs(self):
        dbgFiles = set()

        for root, dirs, files in os.walk(os.path.join(self.installDir,
                                                      'webcamoid/lib/qt')):
            for f in files:
                if f.endswith('.dll'):
                    fname, ext = os.path.splitext(f)
                    dbgFile = os.path.join(root, '{}d{}'.format(fname, ext))

                    if os.path.exists(dbgFile):
                        dbgFiles.add(dbgFile)

        for f in dbgFiles:
            os.remove(f)

    def solvedeps(self):
        self.solvedepsQml()
        self.solvedepsPlugins()
        self.removeDebugs()
        self.solvedepsLibs()

    def writeQtConf(self):
        print('Writting qt.conf file')

        paths = {'Plugins': '../lib/qt/plugins',
                 'Imports': '../lib/qt/qml',
                 'Qml2Imports': '../lib/qt/qml'}

        with open(os.path.join(self.installDir, 'webcamoid/bin/qt.conf'), 'w') as qtconf:
            qtconf.write('[Paths]\n')

            for path in paths:
                qtconf.write('{} = {}\n'.format(path, paths[path]))

    def createLauncher(self):
        print('Writting launcher file')

        with open(os.path.join(self.installDir, 'webcamoid/webcamoid.bat'), 'w') as launcher:
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

    def strip(self, binary):
        process = subprocess.Popen([os.path.join(self.sysBinsPath, 'strip'),
                                    binary],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        process.communicate()

    def stripSymbols(self):
        print('Stripping symbols')
        path = os.path.join(self.installDir, 'webcamoid')
        threads = []

        for exe in self.findExes(path):
            thread = threading.Thread(target=self.strip, args=(exe,))
            threads.append(thread)

            while threading.active_count() >= self.njobs:
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
        self.removeUnneededFiles(os.path.join(self.installDir, 'webcamoid'))

    def hrSize(self, size):
        i = int(math.log(size) // math.log(1024))

        if i < 1:
            return '{} B'.format(size)

        units = ['KiB', 'MiB', 'GiB', 'TiB']
        sizeKiB = size / (1024 ** i)

        return '{:.2f} {}'.format(sizeKiB, units[i - 1])

    def printPackageInfo(self, path):
        if os.path.exists(path):
            print('   ', os.path.basename(path),
                  self.hrSize(os.path.getsize(path)))

    def createPortable(self, mutex):
        path = os.path.join(self.installDir, 'webcamoid')
        arch = 'win32' if self.targetArch == '32bit' else 'win64'
        packagePath = \
            os.path.join(self.pkgsDir,
                         'webcamoid-portable-{}-{}.zip'.format(self.programVersion,
                                                               arch))

        if not os.path.exists(self.pkgsDir):
            os.makedirs(self.pkgsDir)

        with zipfile.ZipFile(packagePath, 'w', zipfile.ZIP_DEFLATED, False) as zipFile:
            for root, dirs, files in os.walk(path):
                for f in dirs + files:
                    filePath = os.path.join(root, f)
                    dstPath = os.path.join('webcamoid',
                                           filePath.replace(path + os.sep, ''))
                    zipFile.write(filePath, dstPath)

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

        appIconSrc = os.path.join(self.rootDir, 'StandAlone/share/icons/hicolor/{0}x{0}/webcamoid.ico'.format(self.installerIconSize))
        self.copy(appIconSrc, configDir)
        self.copy(os.path.join(self.rootDir, 'COPYING'),
                  os.path.join(metaDir, 'COPYING.txt'))

        try:
            shutil.copytree(os.path.join(self.installDir, 'webcamoid'), dataDir, True)
        except:
            pass

        configXml = os.path.join(configDir, 'config.xml')

        if int(self.qtIFWVersion.split('.')[0]) < 3:
            appsDir = '@ApplicationsDir@'
        else:
            if self.targetArch == '32bit':
                appsDir = '@ApplicationsDirX86@'
            else:
                appsDir = '@ApplicationsDirX64@'

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
            config.write('    <RunProgram>@TargetDir@/bin/webcamoid.exe</RunProgram>\n')
            config.write('    <RunProgramDescription>Launch Webcamoid now!</RunProgramDescription>\n')
            config.write('    <StartMenuDir>Webcamoid</StartMenuDir>\n')
            config.write('    <MaintenanceToolName>WebcamoidMaintenanceTool</MaintenanceToolName>\n')
            config.write('    <AllowNonAsciiCharacters>true</AllowNonAsciiCharacters>\n')
            config.write('    <TargetDir>{}/webcamoid</TargetDir>\n'.format(appsDir))
            config.write('</Installer>\n')

        self.copy(os.path.join(self.rootDir, 'ports/deploy/installscript.windows.qs'),
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
        arch = 'win32' if self.targetArch == '32bit' else 'win64'
        packagePath = os.path.join(self.pkgsDir,
                                   'webcamoid-{}-{}.exe'.format(self.programVersion,
                                                                arch))

        if not os.path.exists(self.pkgsDir):
            os.makedirs(self.pkgsDir)

        if os.path.exists(packagePath):
            os.remove(packagePath)

        process = subprocess.Popen([self.wineCmd, self.qtIFW,
                                    '-c', configXml,
                                    '-p', os.path.join(self.installDir,
                                                       'installer/packages'),
                                    packagePath],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()

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
