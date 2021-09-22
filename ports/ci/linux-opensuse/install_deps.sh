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

if [ ! -z "${USE_WGET}" ]; then
    export DOWNLOAD_CMD="wget -nv -c"
else
    export DOWNLOAD_CMD="curl --retry 10 -sS -kLOC -"
fi

zypper -n dup
zypper -n in \
    curl \
    libX11-xcb1 \
    libXext6 \
    libXrender1 \
    libdbus-1-3 \
    libfontconfig1 \
    libglvnd \
    libxcb-glx0 \
    libxkbcommon-x11-0 \
    wget

mkdir -p .local/bin

# Install Qt Installer Framework

qtIFW=QtInstallerFramework-linux-x64-${QTIFWVER}.run
${DOWNLOAD_CMD} http://download.qt.io/official_releases/qt-installer-framework/${QTIFWVER}/${qtIFW} || true

if [ -e ${qtIFW} ]; then
    chmod +x ${qtIFW}
    QT_QPA_PLATFORM=minimal \
    ./${qtIFW} \
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
wget -c -O .local/${appimage} https://github.com/AppImage/AppImageKit/releases/download/${APPIMAGEVER}/${appimage} || true

if [ -e .local/${appimage} ]; then
    chmod +x .local/${appimage}

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
    gstreamer-plugins-base-devel \
    gzip \
    libjack-devel \
    libQt5QuickControls2-devel \
    libpulse-devel \
    libqt5-linguist \
    libqt5-qtbase-devel \
    libqt5-qtdeclarative-devel \
    libqt5-qtquickcontrols2 \
    libqt5-qtsvg-devel \
    libv4l-devel \
    vlc-devel \
    python3 \
    which \
    xauth \
    xvfb-run
