#!/bin/bash

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

[ -f ./environment.sh ] && source ./environment.sh

cat ./environment.sh

pacman -Syy
pacman --noconfirm --needed -S \
    git \
    make \
    pkg-config \
    python3

if [ "${PLATFORM}" = x86 ]; then
    pacman --noconfirm --needed -S \
        mingw-w64-i686-pkg-config \
        mingw-w64-i686-qt5 \
        mingw-w64-i686-ffmpeg \
        mingw-w64-i686-gst-plugins-base \
        mingw-w64-i686-gst-plugins-good \
        mingw-w64-i686-gst-plugins-bad \
        mingw-w64-i686-gst-plugins-ugly
else
    pacman --noconfirm --needed -S \
        mingw-w64-x86_64-pkg-config \
        mingw-w64-x86_64-qt5 \
        mingw-w64-x86_64-ffmpeg \
        mingw-w64-x86_64-gst-plugins-base \
        mingw-w64-x86_64-gst-plugins-good \
        mingw-w64-x86_64-gst-plugins-bad \
        mingw-w64-x86_64-gst-plugins-ugly
fi
