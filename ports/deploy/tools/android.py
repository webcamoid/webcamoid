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

import os
import shutil
import xml.etree.ElementTree as ET

import tools.utils


class AndroidTools(tools.utils.DeployToolsUtils):
    def __init__(self):
        super().__init__()
        self.androidPlatform = ''
        self.archMap = [('arm64-v8a'  , 'aarch64', 'aarch64-linux-android'),
                        ('armeabi-v7a', 'arm'    , 'arm-linux-androideabi'),
                        ('x86'        , 'i686'   , 'i686-linux-android'   ),
                        ('x86_64'     , 'x86_64' , 'x86_64-linux-android' )]
        self.androidSDK = ''
        self.androidNDK = ''
        self.bundledInLib = []
        self.qtLibs = []
        self.localLibs = []

        if 'ANDROID_HOME' in os.environ:
            self.androidSDK = os.environ['ANDROID_HOME']

        if 'ANDROID_NDK_ROOT' in os.environ:
            self.androidNDK = os.environ['ANDROID_NDK_ROOT']
        elif 'ANDROID_NDK' in os.environ:
            self.androidNDK = os.environ['ANDROID_NDK']

    def detectAndroidPlatform(self, path=''):
        for makeFile in self.detectMakeFiles(path):
            with open(makeFile) as f:
                for line in f:
                    if line.startswith('DESTDIR') and '=' in line:
                        buildPath = os.path.join(path, line.split('=')[1].strip())
                        chunks = [part for part in buildPath.split(os.path.sep) if len(part) > 0]

                        if len(chunks) >= 2:
                            self.androidPlatform = chunks[len(chunks) - 2]

                            return

    def detectLibPaths(self):
        if len(self.androidNDK) < 1:
            return []

        for arch in self.archMap:
            if self.targetArch == arch[0]:
                return [os.path.join(self.androidNDK,
                                     'toolchains',
                                     'llvm',
                                     'prebuilt',
                                     'linux-x86_64',
                                     'sysroot',
                                     'usr',
                                     'lib',
                                     arch[1] + '-linux-android')]

        return []

    def detectBinPaths(self):
        if len(self.androidNDK) < 1:
            return []

        for arch in self.archMap:
            if self.targetArch == arch[0]:
                binPath = os.path.join(self.androidNDK,
                                       'toolchains',
                                       'llvm',
                                       'prebuilt',
                                       'linux-x86_64',
                                       arch[2],
                                       'bin')

                return [binPath]

        return []

    def fixQtLibs(self):
        for root, dirs, files in os.walk(self.assetsIntallDir):
            for f in files:
                if f.endswith('.so'):
                    srcPath = os.path.join(root, f)
                    relPath = root.replace(self.assetsIntallDir, '')[1:]
                    prefix = 'lib' + relPath.replace(os.path.sep, '_') + '_'
                    lib = ''

                    if f.startswith(prefix):
                        lib = f
                    else:
                        lib = prefix + f

                    dstPath = os.path.join(self.libInstallDir, lib)
                    print('    {} -> {}'.format(srcPath, dstPath))
                    self.move(srcPath, dstPath)
                    self.bundledInLib += [(lib, os.path.join(relPath, f))]

    def libBaseName(self, lib):
        basename = os.path.basename(lib)

        return basename[3: len(basename) - 3]

    def fixLibsXml(self):
        bundledInAssets = []
        assetsDir = os.path.join(self.rootInstallDir, 'assets')

        for root, dirs, files in os.walk(assetsDir):
            for f in files:
                srcPath = os.path.join(root.replace(assetsDir, '')[1:], f)
                dstPath = os.path.sep.join(srcPath.split(os.path.sep)[1:])

                if (len(dstPath) > 0):
                    bundledInAssets += [(srcPath, dstPath)]

        libsXml = os.path.join(self.rootInstallDir, 'res', 'values', 'libs.xml')
        libsXmlTemp = os.path.join(self.rootInstallDir, 'res', 'values', 'libsTemp.xml')

        tree = ET.parse(libsXml)
        root = tree.getroot()
        oldFeatures = set()
        oldPermissions = set()
        resources = {}

        for array in root:
            if not array.attrib['name'] in resources:
                resources[array.attrib['name']] = set()

            for item in array:
                if item.text:
                    lib = item.text.strip()

                    if len(lib) > 0:
                        lib = '<item>{}</item>'.format(lib)
                        resources[array.attrib['name']].add(lib)

        qtLibs = set(['<item>{};{}</item>'.format(self.targetArch, self.libBaseName(lib)) for lib in self.qtLibs])

        if 'qt_libs' in resources:
            qtLibs -= resources['qt_libs']

        qtLibs = '\n'.join(sorted(list(qtLibs)))
        bundledInLib = set(['<item>{}:{}</item>'.format(lib[0], lib[1]) for lib in self.bundledInLib])

        if 'bundled_in_lib' in resources:
            bundledInLib -= resources['bundled_in_lib']

        bundledInLib = '\n'.join(sorted(list(bundledInLib)))
        bundledInAssets = set(['<item>{}:{}</item>'.format(lib[0], lib[1]) for lib in bundledInAssets])

        if 'bundled_in_assets' in resources:
            bundledInAssets -= resources['bundled_in_assets']

        bundledInAssets = '\n'.join(sorted(list(bundledInAssets)))

        localLibs = sorted(list(set(self.localLibs)))
        localLibs = set(['<item>{};{}</item>'.format(self.targetArch, ':'.join(localLibs)) for lib in localLibs])

        if 'load_local_libs' in resources:
            localLibs -= resources['load_local_libs']

        localLibs = '\n'.join(sorted(list(localLibs)))

        replace = {'<!-- %%INSERT_EXTRA_LIBS%% -->'       : '',
                   '<!-- %%INSERT_QT_LIBS%% -->'          : qtLibs,
                   '<!-- %%INSERT_BUNDLED_IN_LIB%% -->'   : bundledInLib,
                   '<!-- %%INSERT_BUNDLED_IN_ASSETS%% -->': bundledInAssets,
                   '<!-- %%INSERT_LOCAL_LIBS%% -->'       : localLibs}

        with open(libsXml) as inFile:
            with open(libsXmlTemp, 'w') as outFile:
                for line in inFile:
                    for key in replace:
                        line = line.replace(key, replace[key])

                    outFile.write(line)

        os.remove(libsXml)
        shutil.move(libsXmlTemp, libsXml)
