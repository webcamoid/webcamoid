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

# Fix keyboard layout bug when running apt

cat << EOF > keyboard_config
XKBMODEL="pc105"
XKBLAYOUT="us"
XKBVARIANT=""
XKBOPTIONS=""
BACKSPACE="guess"
EOF

export LC_ALL=C
export DEBIAN_FRONTEND=noninteractive

apt-get install -qq -y keyboard-configuration
cp -vf keyboard_config /etc/default/keyboard
dpkg-reconfigure --frontend noninteractive keyboard-configuration

# Install missing dependencies

apt-get -qq -y update
apt-get -qq -y upgrade
apt-get -qq -y install \
    curl \
    libdbus-1-3 \
    libfontconfig1 \
    libgl1 \
    libx11-xcb1 \
    libxcb-glx0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-shape0 \
    libxcb-xinerama0 \
    libxext6 \
    libxkbcommon-x11-0 \
    libxrender1 \
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

apt-get -y install \
    ccache \
    clang \
    cmake \
    file \
    g++ \
    git \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    libasound2-dev \
    libavcodec-dev \
    libavdevice-dev \
    libavformat-dev \
    libavutil-dev \
    libgl1-mesa-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer1.0-0 \
    libjack-dev \
    libkmod-dev \
    libpipewire-0.3-dev \
    libpulse-dev \
    libqt5opengl5-dev \
    libqt5svg5-dev \
    libsdl2-dev \
    libswresample-dev \
    libswscale-dev \
    libusb-dev \
    libuvc-dev \
    libv4l-dev \
    libvlc-dev \
    libvlccore-dev \
    linux-libc-dev \
    make \
    patchelf \
    pkg-config \
    portaudio19-dev \
    qml-module-qt-labs-folderlistmodel \
    qml-module-qt-labs-platform \
    qml-module-qt-labs-settings \
    qml-module-qtqml-models2 \
    qml-module-qtquick-controls2 \
    qml-module-qtquick-dialogs \
    qml-module-qtquick-extras \
    qml-module-qtquick-privatewidgets \
    qml-module-qtquick-templates2 \
    qt5-qmake \
    qtdeclarative5-dev \
    qtmultimedia5-dev \
    qtquickcontrols2-5-dev \
    qttools5-dev-tools \
    qtwayland5 \
    vlc-plugin-base \
    xvfb
