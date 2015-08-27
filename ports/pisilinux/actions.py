#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Licensed under the GNU General Public License. version 3.
# See the file http://www.gnu.org/copyleft/gpl.txt

# Use this as variables:
# Package Name : webcamoid
# Version : 6.2.0
# Summary : Webcamoid, the full webcam and multimedia suite.

import os
from pisi.actionsapi import qt5
from pisi.actionsapi import pisitools
from pisi.actionsapi import get

def setup():
    qt5.configure('Webcamoid.pro')

def build():
    os.system('make VERBOSE=1')

def install():
    os.system('make install INSTALL_ROOT="%s"' % (get.installDIR()))
