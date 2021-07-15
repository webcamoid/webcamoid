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

EXEC="docker exec ${DOCKERSYS}"

cat << EOF > keyboard_config
XKBMODEL="pc105"
XKBLAYOUT="us"
XKBVARIANT=""
XKBOPTIONS=""
BACKSPACE="guess"
EOF

cat << EOF > base_packages.sh
#!/bin/sh

export LC_ALL=C
export DEBIAN_FRONTEND=noninteractive

apt-get update -qq -y
apt-get install -qq -y keyboard-configuration
cp -vf keyboard_config /etc/default/keyboard
dpkg-reconfigure --frontend noninteractive keyboard-configuration

if [ "${DOCKERIMG}" = ubuntu:focal ]; then
apt-get -y install software-properties-common
add-apt-repository ppa:beineri/opt-qt-${QTVER}-focal
fi

apt-get -y update
apt-get -y upgrade

# Install dev tools

apt-get -y install \
    ccache \
    clang \
    cmake \
    g++ \
    git \
    libasound2-dev \
    libavcodec-dev \
    libavdevice-dev \
    libavformat-dev \
    libavresample-dev \
    libavutil-dev \
    libgl1-mesa-dev \
    libjack-dev \
    libpulse-dev \
    libswresample-dev \
    libswscale-dev \
    libv4l-dev \
    linux-libc-dev \
    make \
    pkg-config \
    xvfb
EOF
chmod +x base_packages.sh
${EXEC} bash base_packages.sh

if [ "${UPLOAD}" != 1 ]; then
    ${EXEC} apt-get -y install \
        libgstreamer-plugins-base1.0-dev

    if [ "${DOCKERIMG}" != ubuntu:focal ]; then
        ${EXEC} apt-get -y install \
            libusb-dev \
            libuvc-dev
    fi
fi

# Install Qt dev
if [ "${DOCKERIMG}" = ubuntu:focal ]; then
    ${EXEC} apt-get -y install \
        qt${PPAQTVER}tools \
        qt${PPAQTVER}declarative \
        qt${PPAQTVER}svg \
        qt${PPAQTVER}quickcontrols2
else
    ${EXEC} apt-get -y install \
        qt5-qmake \
        qttools5-dev-tools \
        qtdeclarative5-dev \
        libqt5opengl5-dev \
        libqt5svg5-dev \
        qtquickcontrols2-5-dev \
        qml-module-qt-labs-folderlistmodel \
        qml-module-qt-labs-settings \
        qml-module-qtqml-models2 \
        qml-module-qtquick-controls2 \
        qml-module-qtquick-dialogs \
        qml-module-qtquick-extras \
        qml-module-qtquick-privatewidgets \
        qml-module-qtquick-templates2
fi
