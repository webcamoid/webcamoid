#!/bin/bash

if [ "${TRAVIS_OS_NAME}" = linux ]; then
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
    wget -c https://download.qt.io/archive/qt/${QTVER:0:3}/${QTVER}/qt-opensource-linux-x64-android-${QTVER}.run
    chmod +x qt-opensource-linux-x64-android-${QTVER}.run
    ./qt-opensource-linux-x64-android-${QTVER}.run \
        -platform minimal \
        --script "$PWD/ports/ci/travis/qt_non_interactive_install.qs" \
        --no-force-installations
elif [ "${DOCKERSYS}" = debian ]; then
    # Install Qt Installer Framework
    wget -c http://download.qt.io/official_releases/qt-installer-framework/${QTIFWVER}/QtInstallerFramework-linux-x64.run

    ./QtInstallerFramework-linux-x64.run \
        -platform minimal \
        --script "$PWD/ports/ci/travis/qtifw_non_interactive_install.qs" \
        --no-force-installations

    # Install AppImageTool
    mkdir -p ~/.local/bin
    wget -c -O ~/.local/bin/appimagetool https://github.com/AppImage/AppImageKit/releases/download/9/appimagetool-x86_64.AppImage

    ${EXEC} apt-get -y update

    if [ "${DOCKERIMG}" = ubuntu:trusty ]; then
        ${EXEC} apt-get -y install software-properties-common
        ${EXEC} add-apt-repository ppa:beineri/opt-qt58-trusty
    elif [ "${DOCKERIMG}" = ubuntu:xenial ]; then
        ${EXEC} apt-get -y install software-properties-common
        ${EXEC} add-apt-repository ppa:beineri/opt-qt58-xenial
    fi

    ${EXEC} apt-get -y update
    ${EXEC} apt-get -y upgrade

    # Install dev tools
    ${EXEC} apt-get -y install \
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
        libgstreamer-plugins-base1.0-dev

    # Install Qt dev
    if [ "${DOCKERIMG}" = ubuntu:trusty ] || \
         [ "${DOCKERIMG}" = ubuntu:xenial ]; then
        ${EXEC} apt-get -y install \
            qt58tools \
            qt58declarative \
            qt58multimedia \
            qt58svg \
            qt58quickcontrols \
            qt58quickcontrols2 \
            qt58graphicaleffects
    else
        ${EXEC} apt-get -y install \
            qt5-qmake \
            qtdeclarative5-dev \
            qtmultimedia5-dev \
            libqt5opengl5-dev \
            libqt5svg5-dev \
            qml-module-qt-labs-folderlistmodel \
            qml-module-qt-labs-settings \
            qml-module-qtqml-models2 \
            qml-module-qtquick-controls \
            qml-module-qtquick-dialogs \
            qml-module-qtquick-extras \
            qml-module-qtquick-privatewidgets
    fi

    # Install FFmpeg dev
    ${EXEC} apt-get -y install \
        libavcodec-dev \
        libavdevice-dev \
        libavformat-dev \
        libavutil-dev \
        libavresample-dev \
        libswscale-dev

    if [ "${DOCKERIMG}" != ubuntu:trusty ]; then
        ${EXEC} apt-get -y install \
            libswresample-dev
    fi
elif [ "${DOCKERSYS}" = fedora ]; then
    ${EXEC} dnf install -y https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-${FEDORAVER}.noarch.rpm
    ${EXEC} dnf install -y https://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-${FEDORAVER}.noarch.rpm
    ${EXEC} dnf -y update

    ${EXEC} dnf -y install \
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
        qt5-qtgraphicaleffects \
        ffmpeg-devel \
        gstreamer1-plugins-base-devel \
        libv4l-devel \
        alsa-lib-devel \
        pulseaudio-libs-devel \
        jack-audio-connection-kit-devel
elif [ "${DOCKERSYS}" = opensuse ]; then
    ${EXEC} zypper -n update

    ${EXEC} zypper -n in \
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
        libqt5-qtgraphicaleffects \
        ffmpeg-devel \
        gstreamer-plugins-base-devel \
        libv4l-devel \
        alsa-devel \
        libpulse-devel \
        libjack-devel
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    # Install Qt Installer Framework
    wget -c http://download.qt.io/official_releases/qt-installer-framework/${QTIFWVER}/QtInstallerFramework-mac-x64.dmg
    device=$(hdiutil attach -readwrite -noverify QtInstallerFramework-mac-x64.dmg | \
             egrep '^/dev/' | sed 1q | awk '{print $1}')

    /Volumes/QtInstallerFramework-mac-x64/QtInstallerFramework-mac-x64.app/Contents/MacOS/QtInstallerFramework-mac-x64 \
        -platform minimal \
        --script "$PWD/ports/ci/travis/qtifw_non_interactive_install.qs" \
        --no-force-installations

    hdiutil detach "${device}"

    # Install Syphon framework
    wget -c https://github.com/Syphon/Simple/releases/download/version-3/Syphon.Simple.Apps.3.zip
    unzip Syphon.Simple.Apps.3.zip
    mkdir -p Syphon
    cp -Rvf "Syphon Simple Apps/Simple Client.app/Contents/Frameworks/Syphon.framework" ./Syphon

    brew install \
        python3 \
        ccache \
        pkg-config \
        qt5 \
        ffmpeg \
        gstreamer \
        gst-plugins-base \
        pulseaudio \
        jack \
        libuvc
fi
