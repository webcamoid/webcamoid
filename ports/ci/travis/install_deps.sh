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

if [ "${ARCH_ROOT_BUILD}" = 1 ]; then
    EXEC='sudo ./root.x86_64/bin/arch-chroot root.x86_64'
elif [ "${TRAVIS_OS_NAME}" = linux ] && [ "${ANDROID_BUILD}" != 1 ]; then
    mkdir -p .local/bin
    qtIFW=QtInstallerFramework-linux-x64.run

    # Install Qt Installer Framework
    wget -c http://download.qt.io/official_releases/qt-installer-framework/${QTIFWVER}/${qtIFW} || true

    if [ -e ${qtIFW} ]; then
        chmod +x ${qtIFW}

        QT_QPA_PLATFORM=minimal \
        ./QtInstallerFramework-linux-x64.run \
            -v \
            --script "$PWD/ports/ci/travis/qtifw_non_interactive_install.qs" \
            --no-force-installations

        cp -vf ~/Qt/QtIFW-${QTIFWVER/-*/}/bin/* .local/bin/
    fi

    appimage=appimagetool-x86_64.AppImage

    # Install AppImageTool
    wget -c -O .local/${appimage} https://github.com/AppImage/AppImageKit/releases/download/${APPIMAGEVER}/${appimage} || true

    if [ -e .local/${appimage} ]; then
        chmod +x .local/${appimage}

        cd .local
        ./${appimage} --appimage-extract
        cp -rvf squashfs-root/usr/* .
        cd ..
    fi

    # Set default Docker command
    EXEC="docker exec ${DOCKERSYS}"
fi

if [ "${ANDROID_BUILD}" = 1 ]; then
    sudo apt-get -y install make

    mkdir -p build
    cd build

    # Install Android NDK
    wget -c https://dl.google.com/android/repository/android-ndk-${NDKVER}-linux-x86_64.zip
    unzip -q android-ndk-${NDKVER}-linux-x86_64.zip

    # Install Qt for Android
    wget -c https://download.qt.io/archive/qt/${QTVER:0:4}/${QTVER}/qt-opensource-linux-x64-${QTVER}.run
    chmod +x qt-opensource-linux-x64-${QTVER}.run

    QT_QPA_PLATFORM=minimal \
    ./qt-opensource-linux-x64-${QTVER}.run \
        -v \
        --script "$PWD/../ports/ci/travis/qt_non_interactive_install.qs" \
        --no-force-installations
elif [ "${ARCH_ROOT_BUILD}" = 1 ]; then
    # Download chroot image
    archImage=archlinux-bootstrap-${ARCH_ROOT_DATE}-x86_64.tar.gz
    wget -c ${ARCH_ROOT_URL}/iso/${ARCH_ROOT_DATE}/$archImage
    sudo tar xzf $archImage

    # Configure mirrors
    cp -vf root.x86_64/etc/pacman.conf .
    cat << EOF >> pacman.conf

[multilib]
Include = /etc/pacman.d/mirrorlist

[ownstuff]
SigLevel = Optional TrustAll
Server = http://martchus.no-ip.biz/repo/arch/ownstuff/os/\$arch
EOF
    sed -i 's/Required DatabaseOptional/Optional TrustAll/g' pacman.conf
    sudo cp -vf pacman.conf root.x86_64/etc/pacman.conf

    cp -vf root.x86_64/etc/pacman.d/mirrorlist .
    cat << EOF >> mirrorlist

Server = ${ARCH_ROOT_URL}/\$repo/os/\$arch
EOF
    sudo cp -vf mirrorlist root.x86_64/etc/pacman.d/mirrorlist

    # Install packages
    sudo mkdir -pv root.x86_64/home/user/webcamoid
    sudo mount --bind root.x86_64 root.x86_64
    sudo mount --bind ${PWD} root.x86_64/home/user/webcamoid

    ${EXEC} pacman-key --init
    ${EXEC} pacman -Syy
    ${EXEC} pacman --noconfirm --needed -S \
        ccache \
        clang \
        make \
        pkgconf \
        python \
        xorg-server-xvfb \
        qt5-quickcontrols \
        qt5-quickcontrols2 \
        qt5-svg \
        v4l-utils \
        qt5-tools \
        qt5-multimedia \
        ffmpeg \
        gst-plugins-base-libs \
        libpulse \
        alsa-lib \
        jack

    # Finish
    sudo umount root.x86_64/home/user/webcamoid
    sudo umount root.x86_64
elif [ "${DOCKERSYS}" = debian ]; then
    ${EXEC} apt-get -y update

    if [ "${DOCKERIMG}" = ubuntu:xenial ]; then
        ${EXEC} apt-get -y install software-properties-common
        ${EXEC} add-apt-repository ppa:beineri/opt-qt-${QTVER}-xenial
    fi

    ${EXEC} apt-get -y update
    ${EXEC} apt-get -y upgrade

    # Install dev tools
    ${EXEC} apt-get -y install \
        git \
        xvfb \
        g++ \
        clang \
        ccache \
        make \
        pkg-config \
        linux-libc-dev \
        libgl1-mesa-dev \
        libpulse-dev \
        libjack-dev \
        libasound2-dev \
        libv4l-dev \
        libavcodec-dev \
        libavdevice-dev \
        libavformat-dev \
        libavutil-dev \
        libavresample-dev \
        libswscale-dev \
        libswresample-dev

    if [ -z "${DAILY_BUILD}" ]; then
        ${EXEC} apt-get -y install \
            libgstreamer-plugins-base1.0-dev
    fi

    # Install Qt dev
    if [ "${DOCKERIMG}" = ubuntu:xenial ]; then
        ${EXEC} apt-get -y install \
            qt${PPAQTVER}tools \
            qt${PPAQTVER}declarative \
            qt${PPAQTVER}multimedia \
            qt${PPAQTVER}svg \
            qt${PPAQTVER}quickcontrols \
            qt${PPAQTVER}quickcontrols2 \
            qt${PPAQTVER}graphicaleffects
    else
        ${EXEC} apt-get -y install \
            qt5-qmake \
            qtdeclarative5-dev \
            qtmultimedia5-dev \
            libqt5opengl5-dev \
            libqt5svg5-dev \
            qtquickcontrols2-5-dev \
            qml-module-qt-labs-folderlistmodel \
            qml-module-qt-labs-settings \
            qml-module-qtqml-models2 \
            qml-module-qtquick-controls \
            qml-module-qtquick-controls2 \
            qml-module-qtquick-dialogs \
            qml-module-qtquick-extras \
            qml-module-qtquick-privatewidgets \
            qml-module-qtquick-templates2
    fi
elif [ "${DOCKERSYS}" = fedora ]; then
    ${EXEC} dnf install -y https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-${FEDORAVER}.noarch.rpm
    ${EXEC} dnf install -y https://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-${FEDORAVER}.noarch.rpm
    ${EXEC} dnf -y update

    ${EXEC} dnf -y install \
        git \
        file \
        which \
        xorg-x11-xauth \
        xorg-x11-server-Xvfb \
        ccache \
        clang \
        make \
        gcc-c++ \
        qt5-qttools-devel \
        qt5-qtdeclarative-devel \
        qt5-qtmultimedia-devel \
        qt5-qtsvg-devel \
        qt5-qtquickcontrols \
        qt5-qtquickcontrols2-devel \
        qt5-qtgraphicaleffects \
        ffmpeg-devel \
        gstreamer1-plugins-base-devel \
        libv4l-devel \
        alsa-lib-devel \
        pulseaudio-libs-devel \
        jack-audio-connection-kit-devel
elif [ "${DOCKERSYS}" = opensuse ]; then
    ${EXEC} zypper -n dup

    ${EXEC} zypper -n in \
        git \
        which \
        xauth \
        xvfb-run \
        python3 \
        ccache \
        clang \
        libqt5-linguist \
        libqt5-qtbase-devel \
        libqt5-qtdeclarative-devel \
        libqt5-qtmultimedia-devel \
        libqt5-qtsvg-devel \
        libqt5-qtquickcontrols \
        libqt5-qtquickcontrols2 \
        libQt5QuickControls2-devel \
        libqt5-qtgraphicaleffects \
        ffmpeg-devel \
        gstreamer-plugins-base-devel \
        libv4l-devel \
        alsa-devel \
        libpulse-devel \
        libjack-devel
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    brew install \
        p7zip \
        python3 \
        ccache \
        pkg-config \
        qt5 \
        ffmpeg

    if [ -z "${DAILY_BUILD}" ]; then
        brew install \
            gstreamer \
            gst-plugins-base \
            pulseaudio \
            jack \
            libuvc
    fi

    qtIFW=QtInstallerFramework-mac-x64.dmg

    # Install Qt Installer Framework
    wget -c http://download.qt.io/official_releases/qt-installer-framework/${QTIFWVER}/${qtIFW} || true

    if [ -e "${qtIFW}" ]; then
        hdiutil convert ${qtIFW} -format UDZO -o qtifw
        7z x -oqtifw qtifw.dmg -bb
        7z x -oqtifw qtifw/5.hfsx -bb
        chmod +x qtifw/QtInstallerFramework-mac-x64/QtInstallerFramework-mac-x64.app/Contents/MacOS/QtInstallerFramework-mac-x64

        qtifw/QtInstallerFramework-mac-x64/QtInstallerFramework-mac-x64.app/Contents/MacOS/QtInstallerFramework-mac-x64 \
            -v \
            --script "$PWD/ports/ci/travis/qtifw_non_interactive_install.qs" \
            --no-force-installations
    fi
fi
