#!/bin/bash

# Webcamoid, camera capture application.
# Copyright (C) 2026  Gonzalo Exequiel Pedone
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

if [ -n "${USE_WGET}" ]; then
    export DOWNLOAD_CMD="wget -nv -c"
else
    export DOWNLOAD_CMD="curl --retry 10 -sS -kLOC -"
fi

if [ -z "${ARCHITECTURE}" ]; then
    architecture="${DOCKERIMG%%/*}"
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

if [ -z "${GCC_VERSION}" ]; then
    GCC_VERSION=12
fi

export LC_ALL=C
export DEBIAN_FRONTEND=noninteractive

# Install missing dependencies

sudo apt-get -qq -y update
sudo apt-get -qq -y upgrade
sudo apt-get -qq -y install \
    curl \
    file \
    libdbus-1-3 \
    libfontconfig1 \
    libgl1 \
    libx11-xcb1 \
    libxcb-cursor0 \
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
    python3-pip \
    wget

if apt-cache show libgpgme11 2>/dev/null | grep -q "^Package:"; then
    sudo apt-get -qq -y install libgpgme11
elif apt-cache show libgpgme11t64 2>/dev/null | grep -q "^Package:"; then
    sudo apt-get -qq -y install libgpgme11t64
else
    sudo apt-get -qq -y install libgpgme45
fi

mkdir -p .local/bin

if [[ ( "${architecture}" = amd64 || "${architecture}" = arm64v8 ) && -n "${QTIFWVER}" ]]; then
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
            ln -svf libtiff.so.6 /usr/lib/aarch64-linux-gnu/libtiff.so.5 || true
            ln -svf libwebp.so.7 /usr/lib/aarch64-linux-gnu/libwebp.so.6 || true
        fi

        chmod +x "${qtIFW}"
        QT_QPA_PLATFORM=minimal \
        ./"${qtIFW}" \
            --verbose \
            --root ~/QtIFW \
            --accept-licenses \
            --accept-messages \
            --confirm-command \
            install || true

        if [ -d ~/QtIFW ]; then
            cd .local
            cp -rvf ~/QtIFW/* .
            cd ..
        fi
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

wget -c -O ".local/${appimage}" "https://github.com/AppImage/appimagetool/releases/download/${APPIMAGEVER}/${appimage}" || true

if [ -e ".local/${appimage}" ]; then
    chmod +x ".local/${appimage}"

    cd .local
    ./${appimage} --appimage-extract || true

    if [ -e squashfs-root/usr ]; then
        cp -rvf squashfs-root/usr/* .
    fi

    cd ..
fi

# Install build dependecies

if [ -n "${BREW_GCC_VERSION}" ]; then
    BREW_GCC_VERSION=@${BREW_GCC_VERSION}
fi

if [ "${USE_SYSTEM_GCC}" != 1 ]; then
    USE_GCC=gcc${BREW_GCC_VERSION}
fi

brew update || true
brew upgrade || true

brew install \
    alsa-lib \
    ccache \
    cmake \
    ffmpeg \
    ${USE_GCC} \
    git \
    libuvc \
    libxext \
    libxfixes \
    ninja \
    patchelf \
    pkg-config \
    pipewire \
    portaudio \
    pulseaudio \
    python \
    qt \
    vulkan-headers
