# Webcamoid, webcam capture application.
# Copyright (C) 2016  Gonzalo Exequiel Pedone
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

CONFIG += console c++11

DEFINES += __STDC_CONSTANT_MACROS NO_DSHOW_STRSAFE

LIBS += \
    -ladvapi32 \
    -lkernel32 \
    -lole32 \
    -loleaut32 \
    -lshell32 \
    -lstrmiids \
    -luser32 \
    -luuid \
    -lwinmm

SOURCES = \
    test.cpp

TARGET = test_auto
