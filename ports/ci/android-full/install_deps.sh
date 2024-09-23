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
    jdk17-openjdk \
    jre17-openjdk-headless \
    libglvnd \
    libx11 \
    libxcb \
    libxext \
    libxkbcommon \
    libxkbcommon-x11 \
    libxrender \
    ninja \
    p7zip \
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

# Install gdown

su - aurbuild -c "yay --noconfirm --needed -S gdown"

# Download local Android binary repository

gdriveId='1OvewPH0SmPWAPPga2H06gevTKvqiZ9uo'
gdown -c -O arch-repo-local-packages.7z "https://drive.google.com/uc?id=${gdriveId}"
7z x -p"${FILE_PASSWORD}" -oarch-repo/ arch-repo-local-packages.7z

# Configure local Android binary repository

cat << EOF >> /etc/pacman.conf

[local-packages]
SigLevel = Never
Server = file:///${PWD}/arch-repo/local-packages/os/any
EOF
sed -i 's/Required DatabaseOptional/Never/g' /etc/pacman.conf

pacman -Syy

# Install aqt installer

python -m venv "$PWD/python"
"${PWD}/python/bin/pip" install -U pip
"${PWD}/python/bin/pip" install -U aqtinstall

# Install Qt for Android

"${PWD}/python/bin/aqt" install-qt linux desktop "${QTVER_ANDROID}" -O "$PWD/Qt"

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

    "${PWD}/python/bin/aqt" install-qt linux android "${QTVER_ANDROID}" "android_${arch_}" -m qtmultimedia -O "$PWD/Qt"
    chmod +x "${PWD}/Qt/${QTVER_ANDROID}/android_${arch_}/bin/qt-cmake"

    # Patch templates

    sed -i '/requestLegacyExternalStorage/d' "${PWD}/Qt/${QTVER_ANDROID}/android_${arch_}/src/android/templates/AndroidManifest.xml"
    sed -i 's/android.useAndroidX=true/android.useAndroidX=false/g' "${PWD}/Qt/${QTVER_ANDROID}/android_${arch_}/src/3rdparty/gradle/gradle.properties"
    sed -i '/androidx/d' "${PWD}/Qt/${QTVER_ANDROID}/android_${arch_}/src/android/templates/build.gradle"
done

# Install packages

pacman --noconfirm --needed -S \
    ccache \
    cmake \
    patchelf \
    python \
    xorg-server-xvfb

# Install packages from AUR

su - aurbuild -c "yay --noconfirm --needed -S android-configure android-environment android-ndk android-platform-${ANDROID_MINIMUM_PLATFORM} android-platform-${ANDROID_TARGET_PLATFORM} android-sdk android-sdk-build-tools android-sdk-platform-tools"

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

    su - aurbuild -c "yay --noconfirm --needed -S android-${envArch}-ffmpeg"
done
