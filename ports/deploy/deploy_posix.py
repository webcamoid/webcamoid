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
        self.qtIFW = self.detectQtIFW()
        self.appImage = self.detectAppImage()
        self.excludes = self.readExcludeList()
        self.dependencies = []
        self.ldLibraryPath = os.environ['LD_LIBRARY_PATH'].split(':') if 'LD_LIBRARY_PATH' in os.environ else []
        self.libsSeachPaths = self.readLdconf() \
                            + ['/usr/lib', '/usr/lib64', '/lib', '/lib64']
        self.installerIconSize = 128
        self.njobs = multiprocessing.cpu_count()

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

    def readLdconf(self, ldconf='/etc/ld.so.conf'):
        if not os.path.exists(ldconf):
            return []

        confDir = os.path.dirname(ldconf)
        libpaths = []

        with open(ldconf) as f:
            for line in f:
                i = line.find('#')

                if i == 0:
                    continue

                if i >= 0:
                    line = line[: i]

                line = line.strip()

                if len(line) < 1:
                    continue

                if line.startswith('include'):
                    conf = line.split()[1]

                    if not conf.startswith('/'):
                        conf = os.path.join(confDir, conf)

                    dirname = os.path.dirname(conf)

                    if os.path.exists(dirname):
                        for f in os.listdir(dirname):
                            path = os.path.join(dirname, f)

                            if fnmatch.fnmatch(path, conf):
                                libpaths += self.readLdconf(path)
                else:
                    libpaths.append(line)

        return libpaths

    def detectQmake(self):
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

    def detectAppImage(self):
        if 'APPIMAGETOOL' in os.environ:
            return os.environ['APPIMAGETOOL']

        return self.whereBin('appimagetool')

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
        self.installDir = os.path.join(self.rootDir, 'ports/deploy/temp_priv/root')
        self.pkgsDir = os.path.join(self.rootDir, 'ports/deploy/packages_auto', sys.platform)
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
                shutil.copytree(insQmlDir, dstQmlDir, True)
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
                        shutil.copytree(sysModulePath, installModulePath, True)
                    except:
                        pass

                    solvedImports.add(imp)
                    self.dependencies.append(os.path.join(sysModulePath, 'qmldir'))

                    for f in self.listQmlFiles(sysModulePath):
                        if not f in solved:
                            qmlFiles.add(f)

            solved.add(qmlFile)

    def isElf(self, path):
        with open(path, 'rb') as f:
            return f.read(4) == b'\x7fELF'

    def findElfs(self, path):
        elfs = []

        for root, dirs, files in os.walk(path):
            for f in files:
                elfPath = os.path.join(root, f)

                if not os.path.islink(elfPath) and self.isElf(elfPath):
                    elfs.append(elfPath)

        return elfs

    def readString(self, f):
        s = b''

        while True:
            c = f.read(1)

            if c == b'\x00':
                break

            s += c

        return s

    def readNumber(self, f, arch):
        if arch == '32bits':
            return struct.unpack('I', f.read(4))[0]

        return struct.unpack('Q', f.read(8))[0]

    def readDynamicEntry(self, f, arch):
        if arch == '32bits':
            return struct.unpack('iI', f.read(8))

        return struct.unpack('qQ', f.read(16))

    # https://refspecs.linuxfoundation.org/lsb.shtml (See Core, Generic)
    # https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
    def elfDump(self, elf):
        # ELF file magic
        ELFMAGIC = b'\x7fELF'

        # Sections
        SHT_STRTAB = 0x3
        SHT_DYNAMIC = 0x6

        # Dynamic section entries
        DT_NULL = 0
        DT_NEEDED = 1
        DT_RPATH = 15
        DT_RUNPATH = 0x1d

        with open(elf, 'rb') as f:
            # Read magic signature.
            magic = f.read(4)

            if magic != ELFMAGIC:
                return {}

            # Read the data structure of the file.
            eiClass = '32bits' if struct.unpack('B', f.read(1))[0] == 1 else '64bits'

            # Read machine code.
            f.seek(0x12, os.SEEK_SET)
            machine = struct.unpack('H', f.read(2))[0]

            # Get a pointer to the sections table.
            sectionHeaderTable = 0

            if eiClass == '32bits':
                f.seek(0x20, os.SEEK_SET)
                sectionHeaderTable = self.readNumber(f, eiClass)
                f.seek(0x30, os.SEEK_SET)
            else:
                f.seek(0x28, os.SEEK_SET)
                sectionHeaderTable = self.readNumber(f, eiClass)
                f.seek(0x3c, os.SEEK_SET)

            # Read the number of sections.
            nSections = struct.unpack('H', f.read(2))[0]

            # Read the index of the string table that stores sections names.
            shstrtabIndex = struct.unpack('H', f.read(2))[0]

            # Read sections.
            f.seek(sectionHeaderTable, os.SEEK_SET)
            neededPtr = []
            rpathsPtr = []
            runpathsPtr = []
            strtabs = []
            shstrtab = []

            for section in range(nSections):
                sectionStart = f.tell()

                # Read the a pointer to the virtual address in the string table
                # that contains the name of this section.
                sectionName = struct.unpack('I', f.read(4))[0]

                # Read the type of this section.
                sectionType = struct.unpack('I', f.read(4))[0]

                # Read the virtual address of this section.
                f.seek(sectionStart + (0x0c if eiClass == '32bits' else 0x10), os.SEEK_SET)
                shAddr = self.readNumber(f, eiClass)

                # Read the offset in file to this section.
                shOffset = self.readNumber(f, eiClass)
                f.seek(shOffset, os.SEEK_SET)

                if sectionType == SHT_DYNAMIC:
                    # Read dynamic sections.
                    while True:
                        # Read dynamic entries.
                        dTag, dVal = self.readDynamicEntry(f, eiClass)

                        if dTag == DT_NULL:
                            # End of dynamic sections.
                            break
                        elif dTag == DT_NEEDED:
                            # Dynamically imported libraries.
                            neededPtr.append(dVal)
                        elif dTag == DT_RPATH:
                            # RPATHs.
                            rpathsPtr.append(dVal)
                        elif dTag == DT_RUNPATH:
                            # RUNPATHs.
                            runpathsPtr.append(dVal)
                elif sectionType == SHT_STRTAB:
                    # Read string tables.
                    if section == shstrtabIndex:
                        # We found the string table that stores sections names.
                        shstrtab = [shAddr, shOffset]
                    else:
                        # Save string tables for later usage.
                        strtabs += [[sectionName, shAddr, shOffset]]

                # Move to next section.
                f.seek(sectionStart + (0x28 if eiClass == '32bits' else 0x40), os.SEEK_SET)

            # Libraries names and RUNPATHs are located in '.dynstr' table.
            strtab = []

            for tab in strtabs:
                f.seek(tab[0] - shstrtab[0] + shstrtab[1], os.SEEK_SET)

                if self.readString(f) == b'.dynstr':
                    strtab = tab

            # Read dynamically imported libraries.
            needed = set()

            for lib in neededPtr:
                f.seek(lib + strtab[2], os.SEEK_SET)
                needed.add(self.readString(f).decode(sys.getdefaultencoding()))

            # Read RPATHs
            rpaths = set()

            for path in rpathsPtr:
                f.seek(path + strtab[2], os.SEEK_SET)
                rpaths.add(self.readString(f).decode(sys.getdefaultencoding()))

            # Read RUNPATHs
            runpaths = set()

            for path in runpathsPtr:
                f.seek(path + strtab[2], os.SEEK_SET)
                runpaths.add(self.readString(f).decode(sys.getdefaultencoding()))

            return {'machine': machine,
                    'imports': needed,
                    'rpath': rpaths,
                    'runpath': runpaths}

        return {}

    def readRpaths(self, elfInfo, binDir):
        rpaths = []
        runpaths = []

        # http://amir.rachum.com/blog/2016/09/17/shared-libraries/
        for rpath in ['rpath', 'runpath']:
            for path in elfInfo[rpath]:
                if '$ORIGIN' in path:
                    path = path.replace('$ORIGIN', binDir)

                if not path.startswith('/'):
                    path = os.path.join(binDir, path)

                path = os.path.normpath(path)

                if rpath == 'rpath':
                    rpaths.append(path)
                else:
                    runpaths.append(path)

        return rpaths, runpaths

    def libPath(self, lib, machine, rpaths, runpaths):
        # man ld.so
        searchPaths = rpaths \
                    + self.ldLibraryPath \
                    + runpaths \
                    + self.libsSeachPaths

        for libdir in searchPaths:
            path = os.path.join(libdir, lib)

            if os.path.exists(path):
                depElfInfo = self.elfDump(path)

                if depElfInfo and depElfInfo['machine'] == machine:
                    return path

        return ''

    def listDependencies(self, path):
        elfInfo = self.elfDump(path)

        if not elfInfo:
            return []

        rpaths, runpaths = self.readRpaths(elfInfo, os.path.dirname(path))
        libs = []

        for lib in elfInfo['imports']:
            libpath = self.libPath(lib, elfInfo['machine'], rpaths, runpaths)

            if len(libpath) > 0:
                libs.append(libpath)

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

        pluginsMap.update({lib + 'd': pluginsMap[lib] for lib in pluginsMap})
        qtDeps = set()

        for elfPath in self.findElfs(self.installDir):
            for dep in self.listDependencies(elfPath):
                if self.libName(dep) in pluginsMap:
                    qtDeps.add(dep)

        solved = set()
        plugins = []
        pluginsPath = os.path.join(self.installDir, 'usr/lib/qt/plugins')

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

                    for elfPath in self.findElfs(sysPluginPath):
                        for elfDep in self.listDependencies(elfPath):
                            if self.libName(elfDep) in pluginsMap \
                               and elfDep != dep \
                               and not elfDep in solved:
                                qtDeps.add(elfDep)

                    plugins.append(plugin)
                    self.dependencies.append(sysPluginPath)

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

    def whereBin(self, binary):
        if not 'PATH' in os.environ or len(os.environ['PATH']) < 1:
            return ''

        for path in os.environ['PATH'].split(':'):
            path = os.path.join(path.strip(), binary)

            if os.path.exists(path):
                return path

        return ''

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

    def strip(self, binary):
        process = subprocess.Popen(['strip', binary],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        process.communicate()

    def stripSymbols(self):
        print('Stripping symbols')

        path = os.path.join(self.installDir, 'usr')
        threads = []

        for elf in self.findElfs(path):
            thread = threading.Thread(target=self.strip, args=(elf,))
            threads.append(thread)

            while threading.active_count() >= self.njobs:
                time.sleep(0.25)

            thread.start()

        for thread in threads:
            thread.join()

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
