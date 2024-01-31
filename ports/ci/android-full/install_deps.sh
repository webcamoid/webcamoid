#!/bin/bash

# Webcamoid, webcam capture application.
# Copyright (C) 2024  Gonzalo Exequiel Pedone
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

# Configure mirrors

cat << EOF >> /etc/pacman.d/mirrorlist

Server = ${ARCH_ROOT_URL}/\$repo/os/\$arch
EOF

# Install missing dependencies

pacman-key --init
pacman-key --populate archlinux
pacman -Syu \
    --noconfirm \
    --ignore linux,linux-api-headers,linux-docs,linux-firmware,linux-headers,pacman
pacman --noconfirm --needed -S \
    base-devel \
    curl \
    fontconfig \
    git \
    gradle \
    jdk-openjdk \
    libglvnd \
    libx11 \
    libxcb \
    libxext \
    libxkbcommon \
    libxkbcommon-x11 \
    libxrender \
    ninja \
    python-pip \
    wget \
    xcb-util-image \
    xcb-util-keysyms \
    xcb-util-renderutil \
    xcb-util-wm

mkdir -p .local/bin

# Create a normal user without a password

mkdir -p /tmp/aurbuild
useradd -m -G wheel aurbuild
passwd -d aurbuild
echo 'aurbuild ALL=(ALL) NOPASSWD: ALL' >> /etc/sudoers
chown -R aurbuild:aurbuild /tmp/aurbuild

# Install Yay

su - aurbuild -c "git clone https://aur.archlinux.org/yay.git /tmp/aurbuild/yay"
su - aurbuild -c "cd /tmp/aurbuild/yay && makepkg -si --noconfirm"

# Install aqt installer

su - aurbuild -c "yay --noconfirm --needed -S python-aqtinstall"

# Install Qt for Android

aqt install-qt linux desktop "${QTVER_ANDROID}" -O "$PWD/Qt"

for arch_ in $(echo "${TARGET_ARCH}" | tr ":" "\n"); do
    case "${arch_}" in
        arm64-v8a)
            arch_=arm64_v8a
            ;;
        armeabi-v7a)
            arch_=armv7
            ;;
        *)
            ;;
    esac

    aqt install-qt linux android "${QTVER_ANDROID}" "android_${arch_}" -m qtmultimedia -O "$PWD/Qt"
    chmod +x "${PWD}/Qt/${QTVER_ANDROID}/android_${arch_}/bin/qt-cmake"
done

# Install packages

pacman --noconfirm --needed -S \
    ccache \
    cmake \
    patchelf \
    python \
    xorg-server-xvfb

# Install packages from AUR

for arch_ in $(echo "${TARGET_ARCH}" | tr ":" "\n"); do
    envArch=${arch_}

    case "${arch_}" in
        arm64-v8a)
            envArch=aarch64
            ;;
        armeabi-v7a)
            envArch=armv7a-eabi
            ;;
        x86)
            envArch=arm64_v8a
            ;;
        x86_64)
            envArch=x86-64
            ;;
        *)
            ;;
    esac

    # Install bootstrap packages before anything else.

    su - aurbuild -c "yay --noconfirm --needed -S android-${envArch}-x264-bootstrap"

    # Install dependencies.

    su - aurbuild -c "yay --noconfirm --needed -S android-${envArch}-ffmpeg"
done
