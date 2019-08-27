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
import struct
import sys

import tools.binary


class DeployToolsBinary(tools.binary.DeployToolsBinary):
    def __init__(self):
        super().__init__()
        self.ldLibraryPath = os.environ['LD_LIBRARY_PATH'].split(':') if 'LD_LIBRARY_PATH' in os.environ else []
        self.libsSeachPaths = self.readLdconf() \
                            + ['/usr/lib',
                               '/usr/lib64',
                               '/lib',
                               '/lib64',
                               '/usr/local/lib',
                               '/usr/local/lib64']
        self.emCodes = {3  : '386',
                        40 : 'ARM',
                        62 : 'X86_64',
                        183: 'AARCH64'}

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

    def isValid(self, path):
        with open(path, 'rb') as f:
            return f.read(4) == b'\x7fELF'

    @staticmethod
    def readString(f):
        s = b''

        while True:
            c = f.read(1)

            if c == b'\x00':
                break

            s += c

        return s

    @staticmethod
    def readNumber(f, arch):
        if arch == '32bits':
            return struct.unpack('I', f.read(4))[0]

        return struct.unpack('Q', f.read(8))[0]

    @staticmethod
    def readDynamicEntry(f, arch):
        if arch == '32bits':
            return struct.unpack('iI', f.read(8))

        return struct.unpack('qQ', f.read(16))

    # https://refspecs.linuxfoundation.org/lsb.shtml (See Core, Generic)
    # https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
    def dump(self, binary):
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

        with open(binary, 'rb') as f:
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

    @staticmethod
    def readRpaths(elfInfo, binDir):
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
                depElfInfo = self.dump(path)

                if depElfInfo:
                    if 'machine' in depElfInfo and (machine == 0 or depElfInfo['machine'] == machine):
                        return path
                    elif 'links' in depElfInfo and len(depElfInfo['links']) > 0:
                        return path

        return ''

    def dependencies(self, binary):
        elfInfo = self.dump(binary)

        if not elfInfo:
            return []

        rpaths, runpaths = self.readRpaths(elfInfo, os.path.dirname(binary))
        libs = []
        deps = []

        if 'imports' in elfInfo:
            deps = elfInfo['imports']
        elif 'links' in elfInfo:
            deps = elfInfo['links']

        machine = 0

        if 'machine' in elfInfo:
            machine = elfInfo['machine']

        for lib in deps:
            libpath = self.libPath(lib, machine, rpaths, runpaths)

            if len(libpath) > 0 and not self.isExcluded(libpath):
                libs.append(libpath)

        return libs

    def name(self, binary):
        dep = os.path.basename(binary)[3:]

        return dep[: dep.find('.')]

    def machineEMCode(self, binary):
        info = self.dump(binary)

        if 'machine' in info:
            if info['machine'] in self.emCodes:
                return self.emCodes[info['machine']]

        return 'UNKNOWN'
