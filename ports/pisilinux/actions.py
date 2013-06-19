#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Licensed under the GNU General Public License. version 3.
# See the file http://www.gnu.org/copyleft/gpl.txt

# Use this as variables:
# Package Name : webcamoid
# Version : 5.0.0b1
# Summary : Webcamoid, the full webcam and multimedia suite.

from pisi.actionsapi import qt4
from pisi.actionsapi import pisitools
from pisi.actionsapi import get

def setup():
    qt4.configure('Webcamoid.pro')

def build():
    qt4.make()

def install():
    qt4.install()
