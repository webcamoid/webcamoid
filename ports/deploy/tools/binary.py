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

import os
import re
import subprocess # nosec
import threading
import time

import tools


class DeployToolsBinary(tools.utils.DeployToolsUtils):
    def __init__(self):
        super().__init__()
        self.detectStrip()
        self.excludes = []

    def isValid(self, path):
        return False

    def find(self, path):
        binaries = []

        for root, _, files in os.walk(path):
            for f in files:
                binaryPath = os.path.join(root, f)

                if not os.path.islink(binaryPath) and self.isValid(binaryPath):
                    binaries.append(binaryPath)

        return binaries

    def dump(self, binary):
        return {}

    def dependencies(self, binary):
        return []

    def allDependencies(self, binary):
        deps = self.dependencies(binary)
        solved = set()

        while len(deps) > 0:
            dep = deps.pop()

            for binDep in self.dependencies(dep):
                if binDep != dep and not binDep in solved:
                    deps.append(binDep)

            if self.system == 'mac':
                i = dep.rfind('.framework/')

                if i >= 0:
                    dep = dep[: i] + '.framework'

            solved.add(dep)

        return solved

    def scanDependencies(self, path):
        deps = set()

        for binPath in self.find(path):
            for dep in self.allDependencies(binPath):
                deps.add(dep)

        return sorted(deps)

    def name(self, binary):
        return ''

    def detectStrip(self):
        self.stripBin = self.whereBin('strip.exe' if self.system == 'windows' else 'strip')

    def strip(self, binary):
        if self.stripBin == '':
            return

        process = subprocess.Popen([self.stripBin, binary], # nosec
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        process.communicate()

    def stripSymbols(self, path):
        threads = []

        for binary in self.find(path):
            thread = threading.Thread(target=self.strip, args=(binary,))
            threads.append(thread)

            while threading.active_count() >= self.njobs:
                time.sleep(0.25)

            thread.start()

        for thread in threads:
            thread.join()

    def readExcludeList(self, excludeList):
        self.excludes = []

        if os.path.exists(excludeList):
            with open(excludeList) as f:
                for line in f:
                    line = line.strip()

                    if len(line) > 0 and line[0] != '#':
                        i = line.find('#')

                        if i >= 0:
                            line = line[: i]

                        line = line.strip()

                        if len(line) > 0:
                            self.excludes.append(line)

    def isExcluded(self, path):
        for exclude in self.excludes:
            if self.targetSystem == 'windows' or self.targetSystem == 'posix_windows':
                path = path.lower().replace('\\', '/')
                exclude = exclude.lower()

            if re.fullmatch(exclude, path):
                return True

        return False

    def resetFilePermissions(self, rootPath, binariesPath):
        for root, dirs, files in os.walk(rootPath):
            for d in dirs:
                permissions = 0o755
                path = os.path.join(root, d)

                if self.system == 'mac':
                    os.chmod(path, permissions, follow_symlinks=False)
                else:
                    os.chmod(path, permissions)

            for f in files:
                permissions = 0o644
                path = os.path.join(root, f)

                if root == binariesPath and self.isValid(path):
                    permissions = 0o744

                if self.system == 'mac':
                    os.chmod(path, permissions, follow_symlinks=False)
                else:
                    os.chmod(path, permissions)
