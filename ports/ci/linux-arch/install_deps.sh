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

# Install missing dependenies
sudo apt-get -qq -y update
sudo apt-get -qq -y upgrade
sudo apt-get -qq -y install \
    libxkbcommon-x11-0

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

EXEC='sudo ./root.x86_64/bin/arch-chroot root.x86_64'

# Download chroot image
archImage=archlinux-bootstrap-${ARCH_ROOT_DATE}-x86_64.tar.gz
${DOWNLOAD_CMD} ${ARCH_ROOT_URL}/iso/${ARCH_ROOT_DATE}/$archImage
sudo tar xzf $archImage

# Configure mirrors
cp -vf root.x86_64/etc/pacman.conf .
cat << EOF >> pacman.conf

[multilib]
Include = /etc/pacman.d/mirrorlist

[ownstuff]
Server = https://ftp.f3l.de/~martchus/\$repo/os/\$arch
Server = http://martchus.no-ip.biz/repo/arch/\$repo/os/\$arch
EOF
sed -i 's/Required DatabaseOptional/Never/g' pacman.conf
sed -i 's/#TotalDownload/TotalDownload/g' pacman.conf
sudo cp -vf pacman.conf root.x86_64/etc/pacman.conf

cp -vf root.x86_64/etc/pacman.d/mirrorlist .
cat << EOF >> mirrorlist

Server = ${ARCH_ROOT_URL}/\$repo/os/\$arch
EOF
sudo cp -vf mirrorlist root.x86_64/etc/pacman.d/mirrorlist

# Install packages
sudo mkdir -pv root.x86_64/$HOME
sudo mount --bind root.x86_64 root.x86_64
sudo mount --bind $HOME root.x86_64/$HOME

${EXEC} pacman-key --init
${EXEC} pacman-key --populate archlinux
${EXEC} pacman -Syu \
    --noconfirm \
    --ignore linux,linux-api-headers,linux-docs,linux-firmware,linux-headers,pacman
${EXEC} pacman --noconfirm --needed -S \
    ccache \
    clang \
    cmake \
    file \
    git \
    make \
    pkgconf \
    python \
    sed \
    xorg-server-xvfb \
    qt5-quickcontrols2 \
    qt5-svg \
    v4l-utils \
    qt5-tools \
    ffmpeg \
    gst-plugins-base-libs \
    libpulse \
    alsa-lib \
    jack

# Finish
sudo umount root.x86_64/$HOME
sudo umount root.x86_64
