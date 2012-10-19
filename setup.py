#!/usr/bin/python2 -B
# -*- coding: utf-8 -*-
#
# Webcamod, webcam capture plasmoid.
# Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
#
# Webcamod is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Webcamod is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
#
# Email     : hipersayan DOT x AT gmail DOT com
# Web-Site 1: http://github.com/hipersayanX/Webcamoid
# Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796

import os
import sys
import glob
from distutils import core, sysconfig


prefix = sysconfig.PREFIX

for arg in sys.argv:
    if arg.startswith('--prefix='):
        prefix = arg.split('=', 1)[1]

mainWindow = os.path.join(prefix,
                          'share/apps/plasma/plasmoids/Webcamoid/contents/'
                          'code/mainwindow.py')

with open('webcamoid', 'w') as launcher:
    launcher.write('#!/bin/sh\n'
                   '\n'
                   'python2 -B \'{0}\'\n'.format(mainWindow))

os.chmod('webcamoid', 0744)
launcherPath = os.path.join(prefix, 'bin/webcamoid')

with open('Webcamoid.desktop', 'w') as desktopLauncher:
    desktopLauncher.\
        write('[Desktop Entry]\n'
              'Encoding=UTF-8\n'
              'Name=Webcamoid\n'
              'GenericName=Webcam Capture Software\n'
              'GenericName[ca]=Programari de Captura de Càmera web\n'
              'GenericName[de]=Webcam-Capture-Software\n'
              'GenericName[el]=κάμερα συλλαμβάνει το λογισμικό\n'
              'GenericName[es]=Programa para Captura de la Webcam\n'
              'GenericName[fr]=Logiciel de Capture Webcam\n'
              'GenericName[gl]=Programa de Captura de Webcam\n'
              'GenericName[it]=Webcam Capture Software\n'
              'GenericName[ja]=ウェブカメラのキャプチャソフトウェア\n'
              'GenericName[ko]=웹캠 캡처 소프트웨어\n'
              'GenericName[pt]=Software de Captura de Webcam\n'
              'GenericName[ru]=Веб-камера захвата программного обеспечения\n'
              'GenericName[zh_CN]=摄像头捕捉软件\n'
              'GenericName[zh_TW]=攝像頭捕捉軟件\n'
              'Comment=Webcam Capture Software\n'
              'Comment[ca]=Programari de Captura de Càmera web\n'
              'Comment[de]=Webcam-Capture-Software\n'
              'Comment[el]=κάμερα συλλαμβάνει το λογισμικό\n'
              'Comment[es]=Programa para Captura de la Webcam\n'
              'Comment[fr]=Logiciel de Capture Webcam\n'
              'Comment[gl]=Programa de Captura de Webcam\n'
              'Comment[it]=Webcam Capture Software\n'
              'Comment[ja]=ウェブカメラのキャプチャソフトウェア\n'
              'Comment[ko]=웹캠 캡처 소프트웨어\n'
              'Comment[pt]=Software de Captura de Webcam\n'
              'Comment[ru]=Веб-камера захвата программного обеспечения\n'
              'Comment[zh_CN]=摄像头捕捉软件\n'
              'Comment[zh_TW]=攝像頭捕捉軟件\n'
              'Exec=\'{0}\'\n'
              'Icon=camera-web\n'
              'Terminal=false\n'
              'Type=Application\n'
              'Categories=AudioVideo;KDE;Qt\n'
              'StartupNotify=true\n'.format(launcherPath))

core.setup(
    name='Webcamoid',
    version='4.0.0',
    license='GPLv3',
    requires=('ctypes',
              'fcntl',
              'os',
              'platform',
              'signal',
              'subprocess',
              'sys',
              'time',
              'PyQt4',
              'PyKDE4'),
    maintainer='Gonzalo Exequiel Pedone',
    maintainer_email='hipersayan DOT x AT gmail DOT com',
    url='http://github.com/hipersayanX/Webcamoid',
    download_url='http://kde-apps.org/content/show.php/Webcamoid?'
                                                            'content=144796',
    platforms=('linux',),
    description='A webcam plasmoid for the KDE desktop environment.',

    data_files=[('share/apps/plasma/plasmoids/Webcamoid/contents/code',
                 glob.glob('contents/code/*.py')),
                ('share/apps/plasma/plasmoids/Webcamoid/contents/code/v4l2',
                 ['contents/code/v4l2/__init__.py',
                  'contents/code/v4l2/v4l2.py',
                  'contents/code/v4l2/LICENSE']),
                ('share/apps/plasma/plasmoids/Webcamoid/contents/ts',
                 glob.glob('contents/ts/*.qm')),
                ('share/apps/plasma/plasmoids/Webcamoid/contents/ui',
                 glob.glob('contents/ui/*.ui')),
                ('share/apps/plasma/plasmoids/Webcamoid',
                 ['metadata.desktop']),
                ('share/applications/kde4', ['Webcamoid.desktop']),
                ('share/licenses/Webcamoid', ['COPYING'])],
    scripts=['webcamoid'],
    classifiers=('Topic :: Multimedia :: Video :: Capture',
                 'Environment :: X11 Applications :: KDE',
                 'Environment :: X11 Applications :: Qt',
                 'Development Status :: 5 - Production/Stable',
                 'Intended Audience :: End Users/Desktop',
                 'License :: OSI Approved :: '
                 'GNU General Public License v3 or later (GPLv3+)',
                 'Natural Language :: Catalan',
                 'Natural Language :: Chinese (Simplified)',
                 'Natural Language :: Chinese (Traditional)',
                 'Natural Language :: English',
                 'Natural Language :: French',
                 'Natural Language :: Galician',
                 'Natural Language :: German',
                 'Natural Language :: Greek',
                 'Natural Language :: Italian',
                 'Natural Language :: Japanese',
                 'Natural Language :: Korean',
                 'Natural Language :: Portuguese',
                 'Natural Language :: Russian',
                 'Natural Language :: Spanish',
                 'Operating System :: POSIX :: Linux',
                 'Programming Language :: Python :: 2.7'),
)
