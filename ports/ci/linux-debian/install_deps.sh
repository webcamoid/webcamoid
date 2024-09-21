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

apt-get -qq -y update
apt-get install -qq -y keyboard-configuration
cp -vf keyboard_config /etc/default/keyboard
dpkg-reconfigure --frontend noninteractive keyboard-configuration

# Install missing dependencies

apt-get -qq -y update
apt-get -qq -y upgrade
apt-get -qq -y install \
    curl \
    file \
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

if [ -z "${ARCHITECTURE}" ]; then
    architecture=amd64
else
    case "${ARCHITECTURE}" in
        aarch64)
            architecture=arm64v8
            ;;
        armv7)
            architecture=arm32v7
            ;;
        *)
            architecture=${ARCHITECTURE}
            ;;
    esac
fi

if [[ ( "${architecture}" = amd64 || "${architecture}" = arm64v8 ) && ! -z "${QTIFWVER}" ]]; then
    # Install Qt Installer Framework

    case "${architecture}" in
        arm64v8)
            qtArch=arm64
            ;;
        *)
            qtArch=x64
            ;;
    esac

    qtIFW=QtInstallerFramework-linux-${qtArch}-${QTIFWVER}.run
    ${DOWNLOAD_CMD} "http://download.qt.io/official_releases/qt-installer-framework/${QTIFWVER}/${qtIFW}" || true

    if [ -e "${qtIFW}" ]; then
        if [ "${architecture}" = arm64v8 ]; then
            ln -svf libtiff.so.6 /usr/lib/aarch64-linux-gnu/libtiff.so.5
            ln -svf libwebp.so.7 /usr/lib/aarch64-linux-gnu/libwebp.so.6
        fi

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
fi

# Install AppImageTool

case "${architecture}" in
    arm64v8)
        appimage=appimagetool-aarch64.AppImage
        ;;
    arm32v7)
        appimage=appimagetool-armhf.AppImage
        ;;
    *)
        appimage=appimagetool-x86_64.AppImage
        ;;
esac

wget -c -O ".local/${appimage}" "https://github.com/AppImage/AppImageKit/releases/download/${APPIMAGEVER}/${appimage}" || true

if [ -e ".local/${appimage}" ]; then
    chmod +x ".local/${appimage}"

    cd .local
    ./${appimage} --appimage-extract
    cp -rvf squashfs-root/usr/* .
    cd ..
fi

# Install build dependecies

apt-get -y install \
    ccache \
    clang \
    cmake \
    dpkg \
    fakeroot \
    file \
    g++ \
    git \
    libasound2-dev \
    libavcodec-dev \
    libavdevice-dev \
    libavformat-dev \
    libavutil-dev \
    libgl1-mesa-dev \
    libjack-dev \
    libkmod-dev \
    libpipewire-0.3-dev \
    libpipewire-0.3-modules \
    libpulse-dev \
    libqt6opengl6-dev \
    libqt6svg6-dev \
    libsdl2-dev \
    libswresample-dev \
    libswscale-dev \
    libusb-dev \
    libuvc-dev \
    libv4l-dev \
    libvlc-dev \
    libvlccore-dev \
    libvulkan-dev \
    libwebpdemux2 \
    libxext-dev \
    libxfixes-dev \
    lintian \
    linux-libc-dev \
    make \
    patchelf \
    pkg-config \
    portaudio19-dev \
    qmake6 \
    qml6-module-qt-labs-folderlistmodel \
    qml6-module-qt-labs-platform \
    qml6-module-qt-labs-settings \
    qml6-module-qtcore \
    qml6-module-qtmultimedia \
    qml6-module-qtqml \
    qml6-module-qtqml-models \
    qml6-module-qtqml-workerscript \
    qml6-module-qtquick-controls \
    qml6-module-qtquick-dialogs \
    qml6-module-qtquick-layouts \
    qml6-module-qtquick-nativestyle \
    qml6-module-qtquick-shapes \
    qml6-module-qtquick-templates \
    qml6-module-qtquick-window \
    qml6-module-qtquick3d-spatialaudio \
    qml6-module-qttest \
    qml6-module-qtwayland-compositor \
    qt6-declarative-dev \
    qt6-l10n-tools \
    qt6-multimedia-dev \
    qt6-wayland \
    vlc-plugin-base \
    xvfb

if [ "${UPLOAD}" != 1 ]; then
    apt-get -y install \
        gstreamer1.0-plugins-base \
        gstreamer1.0-plugins-good \
        libgstreamer-plugins-base1.0-dev \
        libgstreamer1.0-0
fi
