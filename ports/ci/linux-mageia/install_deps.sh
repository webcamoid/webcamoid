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

#qtIinstallerVerbose=--verbose

if [ ! -z "${USE_WGET}" ]; then
    export DOWNLOAD_CMD="wget -nv -c"
else
    export DOWNLOAD_CMD="curl --retry 10 -sS -kLOC -"
fi

dnf -y install dnf-plugins-core
dnf config-manager --set-enabled \
    mageia-x86_64-nonfree \
    mageia-x86_64-tainted \
    updates-x86_64-nonfree \
    updates-x86_64-tainted
dnf repolist
dnf -y update
dnf -y install \
    lib64fontconfig1 \
    lib64gl1 \
    lib64x11-xcb1 \
    lib64xcb-glx0 \
    lib64xcb-icccm4 \
    lib64xcb-shape0 \
    lib64xcb-util-image0 \
    lib64xcb-util-keysyms1 \
    lib64xcb-util-renderutil0 \
    lib64xcb-xinerama0 \
    lib64xext6\
    lib64xkbcommon0 \
    lib64xrender1 \
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

dnf -y install \
    ccache \
    clang \
    cmake \
    gcc-c++ \
    git \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    lib64alsa2-devel \
    lib64ffmpeg-devel \
    lib64gstreamer-plugins-base1.0-devel \
    lib64jack-devel \
    lib64kmod-devel \
    lib64pipewire-devel \
    lib64portaudio-devel \
    lib64pulseaudio-devel \
    lib64qt5concurrent-devel \
    lib64qt5multimedia-devel \
    lib64qt5opengl-devel \
    lib64qt5quick-devel \
    lib64qt5quickcontrols2-devel \
    lib64qt5svg-devel \
    lib64v4l-devel \
    lib64vlc-devel \
    make \
    patchelf \
    qtbase5-common-devel \
    qtquickcontrols25 \
    qttools5 \
    qtwayland5 \
    vlc-plugin-common \
    which \
    x11-server-xvfb \
    xauth
