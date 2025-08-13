#!/bin/bash

# Webcamoid, camera capture application.
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

set -e

if [ ! -z "${USE_WGET}" ]; then
    export DOWNLOAD_CMD="wget -nv -c"
else
    export DOWNLOAD_CMD="curl --retry 10 -sS -kLOC -"
fi

# Configure mirrors

cat << EOF >> /etc/pacman.d/mirrorlist

Server = ${ARCH_ROOT_URL}/\$repo/os/\$arch
EOF

# Update pacman

pacman-key --init
pacman-key --populate archlinux
pacman -Syu \
    --noconfirm \
    --ignore linux,linux-api-headers,linux-docs,linux-firmware,linux-headers,pacman

# Optimize pacman

cp -vf /etc/pacman.conf.pacnew /etc/pacman.conf || true
sed -i 's/#DisableSandbox/DisableSandbox/g' /etc/pacman.conf
sed -i 's/#ParallelDownloads/ParallelDownloads/g' /etc/pacman.conf

# Install missing dependencies

pacman --noconfirm --needed -S \
    base-devel \
    clang \
    curl \
    fontconfig \
    git \
    gradle \
    jdk17-openjdk \
    jre17-openjdk-headless \
    libglvnd \
    libx11 \
    libxcb \
    libxext \
    libxkbcommon \
    libxkbcommon-x11 \
    libxrender \
    maven \
    ninja \
    p7zip \
    python-pip \
    wget \
    xcb-util-image \
    xcb-util-keysyms \
    xcb-util-renderutil \
    xcb-util-wm

# Create a folder for installing the local binaries.

mkdir -p .local/bin

# Create a normal user without a password

mkdir -p /tmp/aurbuild
useradd -m -G wheel aurbuild
passwd -d aurbuild
echo 'aurbuild ALL=(ALL) NOPASSWD: ALL' >> /etc/sudoers
chown -R aurbuild:aurbuild /tmp/aurbuild

# Install gdown

python -m venv ./pvenv
./pvenv/bin/pip install gdown

# Download local Android binary repository

gdriveId='1OvewPH0SmPWAPPga2H06gevTKvqiZ9uo'
./pvenv/bin/gdown -c -O arch-repo-local-packages.7z "https://drive.google.com/uc?id=${gdriveId}"
7z x -p"${FILE_PASSWORD}" -oarch-repo/ arch-repo-local-packages.7z

# Check the db file

ls "${PWD}/arch-repo/local-packages/os/any/local-packages.db"

# Asign the local repository to the alpm group

chown :alpm -Rf "${PWD}/arch-repo"

# Map the directory to a local server

nohup python -m http.server --directory "${PWD}/arch-repo" &
sleep 10s
cat ~/nohup.out || true

# Configure local Android binary repository

cat << EOF >> /etc/pacman.conf

[local-packages]
SigLevel = Never
Server = http:///localhost:8000/local-packages/os/any
EOF
sed -i 's/Required DatabaseOptional/Never/g' /etc/pacman.conf

pacman -Syy

# Configure Java

archlinux-java status
sudo archlinux-java set java-17-openjdk
java -version
archlinux-java status

# Install packages

pacman --noconfirm --needed -S \
    ccache \
    cmake \
    patchelf \
    python \
    xorg-server-xvfb

# Install packages from AUR

pacman --noconfirm --needed -S \
    android-configure \
    android-environment \
    android-ndk \
    android-platform-${ANDROID_MINIMUM_PLATFORM} \
    android-platform-${ANDROID_TARGET_PLATFORM} \
    android-sdk \
    android-sdk-build-tools \
    android-sdk-platform-tools \
    qt6-base qt6-declarative \
    qt6-imageformats \
    qt6-multimedia \
    qt6-shadertools \
    qt6-svg \
    qt6-tools

for arch_ in $(echo "${TARGET_ARCH}" | tr ":" "\n"); do
    envArch=${arch_}

    case "${arch_}" in
        arm64-v8a)
            envArch=aarch64
            ;;
        armeabi-v7a)
            envArch=armv7a-eabi
            ;;
        x86_64)
            envArch=x86-64
            ;;
        *)
            ;;
    esac

    # Install dependencies.

    pacman --noconfirm --needed -S \
        android-${envArch}-qt6-base \
        android-${envArch}-qt6-declarative \
        android-${envArch}-qt6-imageformats \
        android-${envArch}-qt6-multimedia \
        android-${envArch}-qt6-shadertools \
        android-${envArch}-qt6-svg \
        android-${envArch}-qt6-tools \
        android-${envArch}-ffmpeg-minimal \
        android-${envArch}-faac \
        android-${envArch}-libmp4v2 \
        android-${envArch}-libwebm
done
