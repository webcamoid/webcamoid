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

zypper -n dup

if [[ "${DOCKERIMG}" == *leap* ]]; then
    zypper -n in fontconfig
else
    zypper -n in libfontconfig1
fi

zypper -n in \
    curl \
    libX11-xcb1 \
    libXext6 \
    libXrender1 \
    libdbus-1-3 \
    libglvnd \
    libxcb-glx0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-shape0 \
    libxcb-xinerama0 \
    libxkbcommon-x11-0 \
    wget

mkdir -p .local/bin

# Install Qt Installer Framework

qtIFW=QtInstallerFramework-linux-x64-${QTIFWVER}.run
${DOWNLOAD_CMD} "http://download.qt.io/official_releases/qt-installer-framework/${QTIFWVER}/${qtIFW}" || true

if [ -e "${qtIFW}" ]; then
    chmod +x "${qtIFW}"
    QT_QPA_PLATFORM=minimal \
    ./"${qtIFW}" \
        --verbose \
        --root ~/QtIFW \
        --accept-licenses \
        --accept-messages \
        --confirm-command \
        install
    cd .local
    cp -rvf ~/QtIFW/* .
    cd ..
fi

# Install AppImageTool

appimage=appimagetool-x86_64.AppImage
wget -c -O ".local/${appimage}" "https://github.com/AppImage/AppImageKit/releases/download/${APPIMAGEVER}/${appimage}" || true

if [ -e ".local/${appimage}" ]; then
    chmod +x ".local/${appimage}"

    cd .local
    ./${appimage} --appimage-extract
    cp -rvf squashfs-root/usr/* .
    cd ..
fi

zypper -n in \
    alsa-devel \
    ccache \
    clang \
    cmake \
    ffmpeg-devel \
    git \
    gstreamer-plugins-base \
    gstreamer-plugins-base-devel \
    gstreamer-plugins-good \
    libjack-devel \
    libkmod-devel \
    libpulse-devel \
    libusb-1_0-devel \
    libv4l-devel \
    libXext-devel \
    libXfixes-devel \
    patchelf \
    pipewire-devel \
    portaudio-devel \
    python3 \
    qt6-concurrent-devel \
    qt6-declarative-devel \
    qt6-multimedia-devel \
    qt6-quickcontrols2-devel \
    qt6-svg-devel \
    qt6-tools-linguist \
    qt6-wayland \
    qt6-widgets-devel \
    qt6-xml-devel \
    vlc-devel \
    vlc-noX \
    which \
    xauth \
    xvfb-run \
    SDL2-devel
