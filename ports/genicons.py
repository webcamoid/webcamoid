#!/usr/bin/env python
#
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
import subprocess # nosec

# Utils

def createPng(inputFile, outputFile, size, dpi):
    if not shutil.which('convert'):
        return

    subprocess.Popen(['convert',  # nosec
                      '-density', '{0}'.format(dpi),
                      '-resize', '{0}x{0}'.format(size),
                      '-background', 'none',
                      inputFile,
                      outputFile]).communicate()

    if shutil.which('pngquant'):
        subprocess.Popen(['pngquant',
                          '--verbose',
                          '--force',
                          '--strip',
                          '--output', outputFile,
                          outputFile]).communicate()
    elif shutil.which('optipng'):
        subprocess.Popen(['optipng',  # nosec
                          '-O7',
                          '-zw', '32k',
                          outputFile]).communicate()

def createIco(inputFile, outputFile, size, dpi):
    if not shutil.which('convert'):
        return

    subprocess.Popen(['convert',  # nosec
                      '-density', '{0}'.format(dpi),
                      '-resize', '{0}x{0}'.format(size),
                      '-background', 'none',
                      inputFile,
                      outputFile]).communicate()

def createIcns(outputFile, icons):
    if not shutil.which('png2icns'):
        return

    subprocess.Popen(['png2icns',  # nosec
                    outputFile] + icons).communicate()

# Remove old icons

iconSize = [8, 16, 22, 32, 48, 64, 128, 256]

for root, dirs, _ in os.walk('../StandAlone/share/themes/WebcamoidTheme/icons'):
    for d in dirs:
        if d == 'scalable':
            for size in iconSize:
                outdir = os.path.realpath(os.path.join(root,
                                                    '{0}x{0}'.format(size)))

                try:
                    shutil.rmtree(outdir)
                except:
                    pass

# Optimize SVG files.

for root, _, files in os.walk('../StandAlone/share/themes/WebcamoidTheme/icons'):
    for f in files:
        if f.endswith('.svg'):
            filePath = os.path.realpath(os.path.join(root, f))
            basename, _ = os.path.splitext(f)
            tmpPath = os.path.realpath(os.path.join(root, basename + '.tmp.svg'))

            if shutil.which('scour'):
                subprocess.Popen(['scour',
                                  '--enable-viewboxing',
                                  '--enable-id-stripping',
                                  '--enable-comment-stripping',
                                  '--shorten-ids',
                                  '--indent=none',
                                  '-i', filePath,
                                  '-o', tmpPath]).communicate()

                try:
                    shutil.move(tmpPath, filePath)
                except:
                    pass
            elif shutil.which('inkscape'):
                subprocess.Popen(['inkscape',
                                  '-z',
                                  '--vacuum-defs',
                                  '-f', filePath,
                                  '-l', tmpPath]).communicate()

                try:
                    shutil.move(tmpPath, filePath)
                except:
                    pass

# Generate default theme icons.

for root, _, files in os.walk('../StandAlone/share/themes/WebcamoidTheme/icons'):
    for f in files:
        if f.endswith('.svg'):
            filePath = os.path.join(root, f)
            outdir = os.path.realpath(os.path.join(root, '..'))

            for size in iconSize:
                basename, _ = os.path.splitext(f)
                outfile = os.path.join(outdir,
                                       '{0}x{0}'.format(size),
                                       basename + '.png')

                try:
                    os.makedirs(os.path.dirname(outfile))
                except:
                    pass

                createPng(filePath, outfile, size, 120)

                if basename == 'webcamoid':
                    outfile = os.path.join(outdir,
                                            '{0}x{0}'.format(size),
                                            basename + '.ico')
                createIco(filePath, outfile, size, 120)

# Generate Mac icns file.

macIconSize = [16, 32, 48, 128, 256, 512, 1024]
icons = []

for root, dirs, _ in os.walk('../StandAlone/share/themes/WebcamoidTheme/icons'):
    for d in dirs:
        if d == 'scalable':
            for size in macIconSize:
                iconPath = os.path.realpath(os.path.join(root,
                                                         '{0}x{0}'.format(size),
                                                         'webcamoid.png'))

                if os.path.exists(iconPath):
                    icons.append(iconPath)

createIcns('../StandAlone/share/themes/WebcamoidTheme/icons/webcamoid.icns', icons)

# Update icons resources file.

with open('../StandAlone/icons.qrc', 'w') as resource:
    resource.write('<RCC>\n')
    resource.write('    <qresource prefix="/Webcamoid">\n')
    resourceFiles = []

    for root, _, files in os.walk('../StandAlone/share/themes/WebcamoidTheme/icons'):
        for f in files:
            resourceFiles.append(os.path.join(root.replace('../StandAlone/', ''), f))

    for res in sorted(resourceFiles):
        resource.write(8 * ' ' + '<file>' + res + '</file>\n')

    resource.write('    </qresource>\n')
    resource.write('</RCC>\n')

# Generate Android icons.

assets = [("ldpi"   , 120,  36),
          ("mdpi"   , 160,  48),
          ("hdpi"   , 240,  72),
          ("xhdpi"  , 320,  96),
          ("xxhdpi" , 480, 144),
          ("xxxhdpi", 640, 192)]

for folder, dpi, size in assets:
    filePath = '../StandAlone/share/themes/WebcamoidTheme/icons/hicolor/scalable/webcamoid.svg'
    outfile = '../StandAlone/share/android/res/drawable-{}/icon.png'.format(folder)
    createPng(filePath, outfile, size, dpi)
