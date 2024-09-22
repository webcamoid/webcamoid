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

#qtIinstallerVerbose=--verbose

if [ ! -z "${USE_WGET}" ]; then
    export DOWNLOAD_CMD="wget -nv -c"
else
    export DOWNLOAD_CMD="curl --retry 10 -sS -kLOC -"
fi

dnf -y upgrade-minimal --exclude=systemd,systemd-libs
dnf -y install \
    curl \
    dbus-libs \
    fontconfig \
    libX11-xcb\
    libXext \
    libXrender \
    libglvnd-glx \
    libxcb \
    libxkbcommon \
    libxkbcommon-x11 \
    wget \
    xcb-util-wm \
    xcb-util-image \
    xcb-util-keysyms \
    xcb-util-renderutil

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

dnf -y upgrade-minimal --exclude=systemd,systemd-libs
dnf -y install \
    SDL2-devel \
    alsa-lib-devel \
    ccache \
    clang \
    cmake \
    libavdevice-free-devel \
    libavfilter-free-devel \
    libXext-devel \
    libXfixes-devel \
    file \
    gcc-c++ \
    git \
    gstreamer1-plugins-base \
    gstreamer1-plugins-base-devel \
    gstreamer1-plugins-good \
    jack-audio-connection-kit-devel \
    kmod-devel \
    libusb1-devel \
    libuvc-devel \
    libv4l-devel \
    make \
    patchelf \
    pipewire-devel \
    portaudio-devel \
    pulseaudio-libs-devel \
    qt6-linguist \
    qt6-qtbase-devel \
    qt6-qtdeclarative-devel \
    qt6-qtmultimedia-devel \
    qt6-qtquickcontrols2-devel \
    qt6-qtsvg-devel \
    qt6-qttools-devel \
    qt6-qtwayland \
    vlc-core \
    vlc-devel \
    which \
    xorg-x11-server-Xvfb \
    xorg-x11-xauth
