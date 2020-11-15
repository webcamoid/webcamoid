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

import configparser
import json
import os
import platform
import re
import shutil
import subprocess # nosec
import sys
import time
import xml.etree.ElementTree as ET

import tools.utils


class DeployToolsQt(tools.utils.DeployToolsUtils):
    def __init__(self):
        super().__init__()
        self.qmake = ''
        self.qtIFW = ''
        self.qtIFWVersion = ''
        self.qtInstallBins = ''
        self.qtInstallQml = ''
        self.qtInstallPlugins = ''
        self.qmlRootDirs = []
        self.qmlInstallDir = ''
        self.dependencies = []
        self.binarySolver = None
        self.installerConfig = ''
        self.appIcon = ''
        self.installerRunProgram = ''
        self.adminRights = False

    def detectQt(self, path=''):
        self.detectQmake(path)
        self.qtInstallBins = self.qmakeQuery(var='QT_INSTALL_BINS')
        self.qtInstallQml = self.qmakeQuery(var='QT_INSTALL_QML')
        self.qtInstallPlugins = self.qmakeQuery(var='QT_INSTALL_PLUGINS')
        self.detectQtIFW()
        self.detectQtIFWVersion()

    def detectQmake(self, path=''):
        for makeFile in self.detectMakeFiles(path):
            with open(makeFile) as f:
                for line in f:
                    if line.startswith('QMAKE') and '=' in line:
                        self.qmake = line.split('=')[1].strip()

                        return

        if 'QMAKE_PATH' in os.environ:
            self.qmake = os.environ['QMAKE_PATH']

    def detectTargetBinaryFromQt5Make(self, path=''):
        for makeFile in self.detectMakeFiles(path):
            with open(makeFile) as f:
                for line in f:
                    if line.startswith('TARGET') and '=' in line:
                        return os.path.join(path, line.split('=')[1].strip())

        return ''

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

            process = subprocess.Popen(args, # nosec
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE)
            stdout, _ = process.communicate()

            return stdout.strip().decode(sys.getdefaultencoding())
        except:
            pass

        return ''

    @staticmethod
    def detectVersion(proFile):
        if 'DAILY_BUILD' in os.environ:
            return 'daily'

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
            if 'WINEPREFIX' in os.environ:
                homeQt = os.path.expanduser(os.path.join(os.environ['WINEPREFIX'],
                                                         'drive_c/Qt'))
            else:
                homeQt = os.path.expanduser('~/.wine/drive_c/Qt')
        else:
            homeQt = os.path.expanduser('~/Qt')

        binCreator = 'binarycreator'

        if self.targetSystem == 'windows' or self.targetSystem == 'posix_windows':
            binCreator += '.exe'

        for root, _, files in os.walk(homeQt):
            for f in files:
                if f == binCreator:
                    self.qtIFW = os.path.join(root, f)

                    return

        # binarycreator offered by the system is most probably dynamically
        # linked, so it's useful for test purposes only, but not recommended
        # for distribution.
        self.qtIFW = self.whereBin(binCreator)

    def detectQtIFWVersion(self):
        self.qtIFWVersion = ''

        if self.qtIFW == '':
            return

        installerBase = os.path.join(os.path.dirname(self.qtIFW),
                                     'installerbase')

        if self.targetSystem == 'windows' or self.targetSystem == 'posix_windows':
            installerBase += '.exe'

        self.qtIFWVersion = '2.0.0'

        if not os.path.exists(installerBase):
            return

        if self.targetSystem == 'posix_windows':
            installerBase = 'Z:' + installerBase.replace('/', '\\')
            process = subprocess.Popen(['wine', # nosec
                                        installerBase,
                                        '--version'],
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
            stdout, _ = process.communicate(input=b'\n')
        else:
            process = subprocess.Popen([installerBase, # nosec
                                        '--version'],
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
            stdout, _ = process.communicate()

        for line in stdout.split(b'\n'):
            if b'IFW Version:' in line:
                self.qtIFWVersion = line.split(b' ')[2].replace(b'"', b'').replace(b',', b'').decode(sys.getdefaultencoding())

                return

    @staticmethod
    def listQmlFiles(path):
        qmlFiles = set()

        if os.path.isfile(path):
            baseName = os.path.basename(path)

            if baseName == 'qmldir' or path.endswith('.qml'):
                qmlFiles.add(path)
        else:
            for root, _, files in os.walk(path):
                for f in files:
                    if f == 'qmldir' or f.endswith('.qml'):
                        qmlFiles.add(os.path.join(root, f))

        return list(qmlFiles)

    @staticmethod
    def modulePath(importLine):
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
                    self.copy(sysModulePath, installModulePath)
                    solvedImports.add(imp)
                    self.dependencies.append(os.path.join(sysModulePath, 'qmldir'))

                    for f in self.listQmlFiles(sysModulePath):
                        if not f in solved:
                            qmlFiles.add(f)

            solved.add(qmlFile)

    def solvedepsPlugins(self):
        pluginsMap = {
            'Qt53DRenderer': ['sceneparsers', 'geometryloaders'],
            'Qt53DQuickRenderer': ['renderplugins'],
            'Qt5Declarative': ['qml1tooling'],
            'Qt5EglFSDeviceIntegration': ['egldeviceintegrations'],
            'Qt5Gui': ['accessible',
                       'generic',
                       'iconengines',
                       'imageformats',
                       'platforms',
                       'platforminputcontexts',
                       'styles'],
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
            'Qt5WebView': ['webview'],
            'Qt5XcbQpa': ['xcbglintegrations']
        }

        pluginsMap.update({lib + 'd': pluginsMap[lib] for lib in pluginsMap})

        if self.targetSystem == 'android':
            pluginsMap.update({lib + '_' + self.targetArch: pluginsMap[lib] for lib in pluginsMap})

        plugins = []

        for dep in self.binarySolver.scanDependencies(self.installDir):
            libName = self.binarySolver.name(dep)

            if not libName in pluginsMap:
                continue

            for plugin in pluginsMap[libName]:
                if not plugin in plugins:
                    sysPluginPath = os.path.join(self.qtInstallPlugins, plugin)
                    pluginPath = os.path.join(self.pluginsInstallDir, plugin)

                    if not os.path.exists(sysPluginPath):
                        continue

                    print('    {} -> {}'.format(sysPluginPath, pluginPath))
                    self.copy(sysPluginPath, pluginPath)
                    plugins.append(plugin)
                    self.dependencies.append(sysPluginPath)

    def solvedepsAndroid(self):
        installPrefix = self.qmakeQuery(var='QT_INSTALL_PREFIX')
        qtLibsPath = self.qmakeQuery(var='QT_INSTALL_LIBS')
        jars = []
        permissions = set()
        features = set()
        initClasses = set()
        libs = set()

        for f in os.listdir(self.libInstallDir):
            basename = os.path.basename(f)[3:]
            basename = os.path.splitext(basename)[0]
            depFile = os.path.join(qtLibsPath,
                                   basename + '-android-dependencies.xml')

            if os.path.exists(depFile):
                tree = ET.parse(depFile)
                root = tree.getroot()

                for jar in root.iter('jar'):
                    jars.append(jar.attrib['file'])

                    if 'initClass' in jar.attrib:
                        initClasses.append(jar.attrib['initClass'])

                for permission in root.iter('permission'):
                    permissions.add(permission.attrib['name'])

                for feature in root.iter('feature'):
                    features.add(feature.attrib['name'])

                for lib in root.iter('lib'):
                    if 'file' in lib.attrib:
                        libs.add(lib.attrib['file'])

        self.localLibs = [os.path.basename(lib) for lib in libs]

        print('Copying jar files\n')

        for jar in sorted(jars):
            srcPath = os.path.join(installPrefix, jar)
            dstPath = os.path.join(self.rootInstallDir,
                                   'libs',
                                   os.path.basename(jar))
            print('    {} -> {}'.format(srcPath, dstPath))
            self.copy(srcPath, dstPath)

        manifest = os.path.join(self.rootInstallDir, 'AndroidManifest.xml')
        manifestTemp = os.path.join(self.rootInstallDir, 'AndroidManifestTemp.xml')
        tree = ET.parse(manifest)
        root = tree.getroot()
        oldFeatures = set()
        oldPermissions = set()

        for element in root:
            if element.tag == 'uses-feature':
                for key in element.attrib:
                    if key.endswith('name'):
                        oldFeatures.add(element.attrib[key])
            elif element.tag == 'uses-permission':
                for key in element.attrib:
                    if key.endswith('name'):
                        oldPermissions.add(element.attrib[key])

        features -= oldFeatures
        permissions -= oldPermissions
        featuresWritten = len(features) < 1
        permissionsWritten = len(permissions) < 1
        replace = {'-- %%INSERT_INIT_CLASSES%% --' : ':'.join(sorted(initClasses)),
                   '-- %%BUNDLE_LOCAL_QT_LIBS%% --': '1',
                   '-- %%USE_LOCAL_QT_LIBS%% --'   : '1',
                   '-- %%INSERT_LOCAL_LIBS%% --'   : ':'.join(sorted(libs)),
                   '-- %%INSERT_LOCAL_JARS%% --'   : ':'.join(sorted(jars))}

        with open(manifest) as inFile:
            with open(manifestTemp, 'w') as outFile:
                for line in inFile:
                    for key in replace:
                        line = line.replace(key, replace[key])

                    outFile.write(line)
                    spaces = len(line)
                    line = line.lstrip()
                    spaces -= len(line)

                    if line.startswith('<uses-feature') and not featuresWritten:
                        print('\nUpdating features\n')

                        for feature in features:
                            print('    ' + feature)
                            outFile.write(spaces * ' ' + '<uses-feature android:name="{}"/>\n'.format(feature))

                        featuresWritten = True

                    if line.startswith('<uses-permission') and not permissionsWritten:
                        print('\nUpdating permissions\n')

                        for permission in permissions:
                            print('    ' + permission)
                            outFile.write(spaces * ' ' + '<uses-permission android:name="{}"/>\n'.format(permission))

                        permissionsWritten = True

        os.remove(manifest)
        shutil.move(manifestTemp, manifest)

    def writeQtConf(self):
        prefix = self.binaryInstallDir

        if self.targetSystem == 'mac':
            prefix = os.path.abspath(os.path.join(self.binaryInstallDir, '..'))

        paths = {'Plugins': os.path.relpath(self.pluginsInstallDir, prefix).replace('\\', '/'),
                 'Imports': os.path.relpath(self.qmlInstallDir, prefix).replace('\\', '/'),
                 'Qml2Imports': os.path.relpath(self.qmlInstallDir, prefix).replace('\\', '/')}
        confPath = os.path.dirname(self.qtConf)

        if not os.path.exists(confPath):
            os.makedirs(confPath)

        with open(self.qtConf, 'w') as qtconf:
            qtconf.write('[Paths]\n')

            for path in paths:
                qtconf.write('{} = {}\n'.format(path, paths[path]))

    @staticmethod
    def readChangeLog(changeLog, appName, version):
        if os.path.exists(changeLog):
            with open(changeLog) as f:
                for line in f:
                    if not line.startswith('{0} {1}:'.format(appName, version)):
                        continue

                    # Skip first line.
                    f.readline()
                    changeLogText = ''

                    for line_ in f:
                        if re.match('{} \d+\.\d+\.\d+:'.format(appName), line):
                            # Remove last line.
                            i = changeLogText.rfind('\n')

                            if i >= 0:
                                changeLogText = changeLogText[: i]

                            return changeLogText

                        changeLogText += line_

        return ''

    def createInstaller(self):
        if not os.path.exists(self.qtIFW):
            return False

        # Read package config
        packageConf = configparser.ConfigParser()
        packageConf.optionxform=str
        packageConf.read(self.packageConfig, 'utf-8')

        # Create layout
        componentName = 'com.{0}prj.{0}'.format(self.programName)
        packageDir = os.path.join(self.installerPackages, componentName)

        if not os.path.exists(self.installerConfig):
            os.makedirs(self.installerConfig)

        dataDir = os.path.join(packageDir, 'data')
        metaDir = os.path.join(packageDir, 'meta')

        if not os.path.exists(dataDir):
            os.makedirs(dataDir)

        if not os.path.exists(metaDir):
            os.makedirs(metaDir)

        iconName = ''

        if self.appIcon != '' and os.path.exists(self.appIcon):
            self.copy(self.appIcon, self.installerConfig)
            iconName = os.path.splitext(os.path.basename(self.appIcon))[0]

        licenseOutFile = os.path.basename(self.licenseFile)

        if not '.' in licenseOutFile and \
            (self.targetSystem == 'windows' or \
                self.targetSystem == 'posix_windows'):
            licenseOutFile += '.txt'

        self.copy(self.licenseFile, os.path.join(metaDir, licenseOutFile))
        self.copy(self.rootInstallDir, dataDir)

        configXml = os.path.join(self.installerConfig, 'config.xml')
        appName = packageConf['Package']['appName'].strip()

        with open(configXml, 'w') as config:
            config.write('<?xml version="1.0" encoding="UTF-8"?>\n')
            config.write('<Installer>\n')
            config.write('    <Name>{}</Name>\n'.format(appName))

            if 'DAILY_BUILD' in os.environ:
                config.write('    <Version>0.0.0</Version>\n')
            else:
                config.write('    <Version>{}</Version>\n'.format(self.programVersion))

            config.write('    <Title>{}</Title>\n'.format(packageConf['Package']['description'].strip()))
            config.write('    <Publisher>{}</Publisher>\n'.format(appName))
            config.write('    <ProductUrl>{}</ProductUrl>\n'.format(packageConf['Package']['url'].strip()))

            if iconName != '':
                config.write('    <InstallerWindowIcon>{}</InstallerWindowIcon>\n'.format(iconName))
                config.write('    <InstallerApplicationIcon>{}</InstallerApplicationIcon>\n'.format(iconName))
                config.write('    <Logo>{}</Logo>\n'.format(iconName))

            if self.installerRunProgram != '':
                config.write('    <RunProgram>{}</RunProgram>\n'.format(self.installerRunProgram))
                config.write('    <RunProgramDescription>{}</RunProgramDescription>\n'.format(packageConf['Package']['runMessage'].strip()))
                config.write('    <StartMenuDir>{}</StartMenuDir>\n'.format(appName))

            config.write('    <MaintenanceToolName>{}Uninstall</MaintenanceToolName>\n'.format(appName))
            config.write('    <AllowNonAsciiCharacters>true</AllowNonAsciiCharacters>\n')
            config.write('    <TargetDir>{}</TargetDir>\n'.format(self.installerTargetDir))
            config.write('</Installer>\n')

        self.copy(self.installerScript,
                  os.path.join(metaDir, 'installscript.qs'))

        with open(os.path.join(metaDir, 'package.xml'), 'w') as f:
            f.write('<?xml version="1.0"?>\n')
            f.write('<Package>\n')
            f.write('    <DisplayName>{}</DisplayName>\n'.format(appName))
            f.write('    <Description>{}</Description>\n'.format(packageConf['Package']['description'].strip()))

            if 'DAILY_BUILD' in os.environ:
                f.write('    <Version>0.0.0</Version>\n')
            else:
                f.write('    <Version>{}</Version>\n'.format(self.programVersion))

            f.write('    <ReleaseDate>{}</ReleaseDate>\n'.format(time.strftime('%Y-%m-%d')))
            f.write('    <Name>{}</Name>\n'.format(componentName))
            f.write('    <Licenses>\n')
            f.write('        <License name="{0}" file="{1}" />\n'.format(packageConf['Package']['licenseDescription'].strip(),
                                                                         licenseOutFile))
            f.write('    </Licenses>\n')
            f.write('    <Script>installscript.qs</Script>\n')
            f.write('    <UpdateText>\n')

            if not 'DAILY_BUILD' in os.environ:
                f.write(self.readChangeLog(self.changeLog,
                                        appName,
                                        self.programVersion))

            f.write('    </UpdateText>\n')
            f.write('    <Default>true</Default>\n')
            f.write('    <ForcedInstallation>true</ForcedInstallation>\n')
            f.write('    <Essential>false</Essential>\n')

            if self.adminRights:
                f.write('    <RequiresAdminRights>true</RequiresAdminRights>\n')

            f.write('</Package>\n')

        # Remove old file
        if not os.path.exists(self.pkgsDir):
            os.makedirs(self.pkgsDir)

        if os.path.exists(self.outPackage):
            os.remove(self.outPackage)

        params = []

        if self.targetSystem == 'posix_windows':
            params = ['wine']

        params += [self.qtIFW,
                   '-c', configXml,
                   '-p', self.installerPackages,
                   self.outPackage]
        process = subprocess.Popen(params, # nosec
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        process.communicate()

        return True

    def copyAndroidTemplates(self):
        installPrefix = self.qmakeQuery(var='QT_INSTALL_PREFIX')
        sourcesPath = os.path.join(installPrefix, 'src')
        templates = [os.path.join(sourcesPath, '3rdparty/gradle'),
                     os.path.join(sourcesPath, 'android/templates')]

        for template in templates:
            self.copy(template, self.rootInstallDir, overwrite=False)

        deploymentSettingsPath = ''

        for f in os.listdir(self.standAloneDir):
            if re.match('^android-.+-deployment-settings.json$' , f):
                deploymentSettingsPath = os.path.join(self.standAloneDir, f)

                break

        if len(deploymentSettingsPath) < 1:
            return

        with open(deploymentSettingsPath) as f:
            deploymentSettings = json.load(f)

        properties = os.path.join(self.rootInstallDir, 'gradle.properties')
        platform = self.androidPlatform.replace('android-', '')
        javaDir = os.path.join(sourcesPath, 'android','java')

        with open(properties, 'w') as f:
            if 'sdkBuildToolsRevision' in deploymentSettings:
                f.write('androidBuildToolsVersion={}\n'.format(deploymentSettings['sdkBuildToolsRevision']))

            f.write('androidCompileSdkVersion={}\n'.format(platform))
            f.write('buildDir=build\n')
            f.write('qt5AndroidDir={}\n'.format(javaDir))

    def createRccBundle(self):
        rcc = os.path.join(os.path.dirname(self.qmake), 'rcc')
        assetsDir = os.path.abspath(os.path.join(self.assetsIntallDir, '..'))
        assetsFolder = os.path.relpath(self.assetsIntallDir, assetsDir)
        qrcFile = os.path.join(self.assetsIntallDir, assetsFolder + '.qrc')

        params = [rcc,
                  '--project',
                  '-o', qrcFile]
        process = subprocess.Popen(params, # nosec
                                   cwd=self.assetsIntallDir,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        process.communicate()

        params = [rcc,
                  '--root=/{}'.format(assetsFolder),
                  '--binary',
                  '-o', self.assetsIntallDir + '.rcc',
                  qrcFile]
        process = subprocess.Popen(params, # nosec
                                   cwd=assetsDir,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        process.communicate()

        shutil.rmtree(self.assetsIntallDir, True)
