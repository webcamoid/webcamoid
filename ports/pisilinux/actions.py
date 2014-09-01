#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Licensed under the GNU General Public License. version 3.
# See the file http://www.gnu.org/copyleft/gpl.txt

# Use this as variables:
# Package Name : webcamoid
# Version : 5.1.0
# Summary : Webcamoid, the full webcam and multimedia suite.

import os
from pisi.actionsapi import qt4
from pisi.actionsapi import pisitools
from pisi.actionsapi import get

def setup():
    qt4.configure('Webcamoid.pro', 'QMAKE_LRELEASE=/usr/bin/lrelease')

def build():
    os.system('make VERBOSE=1')

def install():
    os.system('make install INSTALL_ROOT="%s"' % (get.installDIR()))
