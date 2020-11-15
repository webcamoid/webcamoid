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
import hashlib
import multiprocessing
import os
import platform
import shutil
import subprocess # nosec
import sys

class DeployToolsUtils:
    def __init__(self):
        self.make = ''

        if os.name == 'posix' and sys.platform.startswith('darwin'):
            self.system = 'mac'
        elif os.name == 'nt' and sys.platform.startswith('win32'):
            self.system = 'windows'
        elif os.name == 'posix':
            self.system = 'posix'
        else:
            self.system = ''

        self.arch = platform.architecture()[0]
        self.targetArch = self.arch
        self.targetSystem = self.system
        pathSep = ';' if self.system == 'windows' else ':'
        self.sysBinsPath = []

        if 'PATH' in os.environ:
            self.sysBinsPath += \
                [path.strip() for path in os.environ['PATH'].split(pathSep)]

        self.njobs = multiprocessing.cpu_count()

        if self.njobs < 4:
            self.njobs = 4

    def detectTargetArch(self, binary=''):
        if binary == '':
            binary = self.mainBinary

        self.targetArch = platform.architecture(binary)[0]

    def whereBin(self, binary):
        for path in self.sysBinsPath:
            path = os.path.join(path, binary)

            if os.path.exists(path):
                return path

        return ''

    def copy(self, src, dst='.', copyReals=False, overwrite=True):
        if not os.path.exists(src):
            return False

        if os.path.isdir(src):
            if os.path.isfile(dst):
                return False

            for root, dirs, files in os.walk(src):
                for f in files:
                    fromF = os.path.join(root, f)
                    toF = os.path.relpath(fromF, src)
                    toF = os.path.join(dst, toF)
                    toF = os.path.normpath(toF)
                    self.copy(fromF, toF, copyReals, overwrite)

                for d in dirs:
                    fromD = os.path.join(root, d)
                    toD = os.path.relpath(fromD, src)
                    toD = os.path.join(dst, toD)

                    try:
                        os.makedirs(os.path.normpath(toD))
                    except:
                        pass
        elif os.path.isfile(src):
            if os.path.isdir(dst):
                dst = os.path.realpath(dst)
                dst = os.path.join(dst, os.path.basename(src))

            dirname = os.path.dirname(dst)

            if not os.path.exists(dirname):
                try:
                    os.makedirs(dirname)
                except:
                    return False

            if os.path.exists(dst):
                if not overwrite:
                    return True

                try:
                    os.remove(dst)
                except:
                    return False

            if copyReals and os.path.islink(src):
                realpath = os.path.realpath(src)
                basename = os.path.basename(realpath)
                os.symlink(os.path.join('.', basename), dst)
                self.copy(realpath,
                          os.path.join(dirname, basename),
                          copyReals,
                          overwrite)
            else:
                try:
                    if self.system == 'windows':
                        shutil.copy(src, dst)
                    else:
                        shutil.copy(src, dst, follow_symlinks=False)
                except:
                    return False

        return True


    def move(self, src, dst='.', moveReals=False):
        if not os.path.exists(src):
            return False

        if os.path.isdir(src):
            if os.path.isfile(dst):
                return False

            for root, dirs, files in os.walk(src):
                for f in files:
                    fromF = os.path.join(root, f)
                    toF = os.path.relpath(fromF, src)
                    toF = os.path.join(dst, toF)
                    toF = os.path.normpath(toF)
                    self.move(fromF, toF, moveReals)

                for d in dirs:
                    fromD = os.path.join(root, d)
                    toD = os.path.relpath(fromD, src)
                    toD = os.path.join(dst, toD)

                    try:
                        os.makedirs(os.path.normpath(toD))
                    except:
                        pass
        elif os.path.isfile(src):
            if os.path.isdir(dst):
                dst = os.path.realpath(dst)
                dst = os.path.join(dst, os.path.basename(src))

            dirname = os.path.dirname(dst)

            if not os.path.exists(dirname):
                try:
                    os.makedirs(dirname)
                except:
                    return False

            if os.path.exists(dst):
                try:
                    os.remove(dst)
                except:
                    return False

            if moveReals and os.path.islink(src):
                realpath = os.path.realpath(src)
                basename = os.path.basename(realpath)
                os.symlink(os.path.join('.', basename), dst)
                self.move(realpath, os.path.join(dirname, basename), moveReals)
            else:
                try:
                    shutil.move(src, dst)
                except:
                    return False

        return True

    def detectMake(self):
        if 'MAKE_PATH' in os.environ:
            self.make = os.environ['MAKE_PATH']

            return

        makes = ['mingw32-make', 'make'] if self.system == 'windows' else ['make']
        ext = '.exe' if self.system == 'windows' else ''

        for make in makes:
            makePath = self.whereBin(make + ext)

            if makePath != '':
                self.make = makePath

                break

    def makeInstall(self, buildDir, params={}):
        previousDir = os.getcwd()
        os.chdir(buildDir)
        params_ = [key + '=' + params[key] for key in params]
        process = subprocess.Popen([self.make, 'install'] + params_, # nosec
                                    stdout=subprocess.PIPE)

        process.communicate()
        os.chdir(previousDir)

        return process.returncode

    @staticmethod
    def sha256sum(fileName):
        sha = hashlib.sha256()

        with open(fileName, 'rb') as f:
            while True:
                data = f.read(1024)

                if not data:
                    break

                sha.update(data)

        return sha.hexdigest()

    @staticmethod
    def detectMakeFiles(makePath):
        makeFiles = []

        try:
            for f in os.listdir(makePath):
                path = os.path.join(makePath, f)

                if os.path.isfile(path) and fnmatch.fnmatch(f.lower(), 'makefile*'):
                    makeFiles += [path]
        except:
            pass

        return makeFiles
