#!/bin/bash

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

set -e
set -o errexit

if [ ! -z "${USE_WGET}" ]; then
    export DOWNLOAD_CMD="wget -nv -c"
else
    export DOWNLOAD_CMD="curl --retry 10 -sS -kLOC -"
fi

# Configure mirrors

sed -n '/^\[core\]/q;p' /etc/pacman.conf > /etc/pacman_temp.conf
mv -vf /etc/pacman_temp.conf /etc/pacman.conf

cat << EOF >> /etc/pacman.conf

[ownstuff]
Server = https://ftp.f3l.de/~martchus/\$repo/os/\$arch
Server = http://martchus.no-ip.biz/repo/arch/\$repo/os/\$arch

[core]
Include = /etc/pacman.d/mirrorlist

[extra]
Include = /etc/pacman.d/mirrorlist

[multilib]
Include = /etc/pacman.d/mirrorlist
EOF
sed -i 's/Required DatabaseOptional/Never/g' /etc/pacman.conf

cat << EOF >> /etc/pacman.d/mirrorlist

Server = ${ARCH_ROOT_URL}/\$repo/os/\$arch
EOF

# Install packages

pacman-key --init
pacman-key --populate archlinux
pacman -Syu \
    --noconfirm \
    --ignore linux,linux-api-headers,linux-docs,linux-firmware,linux-headers,pacman
pacman --noconfirm --needed -S \
    ccache \
    clang \
    cmake \
    file \
    git \
    lib32-mpg123 \
    make \
    mpg123 \
    nsis \
    pkgconf \
    python \
    qt6-declarative \
    qt6-tools \
    sed \
    unzip \
    vulkan-headers \
    wine \
    xorg-server-xvfb \
    mingw-w64-cmake \
    mingw-w64-ffmpeg \
    mingw-w64-gcc \
    mingw-w64-portaudio \
    mingw-w64-pkg-config \
    mingw-w64-qt6-declarative \
    mingw-w64-qt6-imageformats \
    mingw-w64-qt6-multimedia \
    mingw-w64-qt6-svg \
    mingw-w64-qt6-tools \
    mingw-w64-sdl2 \
    mingw-w64-vulkan-headers

# Install NSIS

NSIS_VERSION=3.10
nsis=nsis-${NSIS_VERSION}.zip
${DOWNLOAD_CMD} "https://downloads.sourceforge.net/nsis/NSIS%20${NSIS_VERSION:0:1}/${NSIS_VERSION}/${nsis}"

if [ -e "${nsis}" ]; then
    unzip -q "${PWD}/${nsis}"
fi
