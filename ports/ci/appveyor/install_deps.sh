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

[ -f environment.sh ] && source environment.sh

pacman -Syy
pacman --noconfirm --needed -S \
    ccache \
    clang \
    cmake \
    git \
    make \
    pkgconf \
    python3

if [ "${PLATFORM}" = x86 ]; then
    packagesArch=i686
else
    packagesArch=x86_64
fi

pacman --noconfirm --needed -S \
    mingw-w64-${packagesArch}-ccache \
    mingw-w64-${packagesArch}-clang \
    mingw-w64-${packagesArch}-cmake \
    mingw-w64-${packagesArch}-pkgconf \
    mingw-w64-${packagesArch}-qt5 \
    mingw-w64-${packagesArch}-ffmpeg

if [ -z "${DAILY_BUILD}" ] && [ -z "${RELEASE_BUILD}" ]; then
    pacman --noconfirm --needed -S \
        mingw-w64-${packagesArch}-gst-plugins-base \
        mingw-w64-${packagesArch}-gst-plugins-good \
        mingw-w64-${packagesArch}-gst-plugins-bad \
        mingw-w64-${packagesArch}-gst-plugins-ugly
fi
