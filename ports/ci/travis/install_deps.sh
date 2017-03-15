#!/bin/sh

if [ "${TRAVIS_OS_NAME}" = linux ]; then
    EXEC="docker exec ${DOCKERSYS}"
fi

if [ "${DOCKERSYS}" = debian ]; then
    ${EXEC} apt-get -y update

    if [ "${DOCKERIMG}" = ubuntu:precise ]; then
        ${EXEC} apt-get -y install python-software-properties
        ${EXEC} add-apt-repository ppa:beineri/opt-qt562
    elif [ "${DOCKERIMG}" = ubuntu:trusty ]; then
        ${EXEC} apt-get -y install software-properties-common
        ${EXEC} add-apt-repository ppa:beineri/opt-qt58-trusty
    elif [ "${DOCKERIMG}" = ubuntu:xenial ]; then
        ${EXEC} apt-get -y install software-properties-common
        ${EXEC} add-apt-repository ppa:beineri/opt-qt58-xenial
    fi

    if [ "${DOCKERIMG}" = ubuntu:precise ] || \
       [ "${DOCKERIMG}" = ubuntu:trusty ] || \
       [ "${DOCKERIMG}" = ubuntu:xenial ]; then
        ${EXEC} add-apt-repository ppa:sergey-dryabzhinsky/packages
        ${EXEC} add-apt-repository ppa:sergey-dryabzhinsky/toolchain
        ${EXEC} add-apt-repository ppa:sergey-dryabzhinsky/ffmpeg
    fi

    if [ "${DOCKERIMG}" = ubuntu:precise ]; then
        ${EXEC} add-apt-repository ppa:h-rayflood/gcc-upper
        ${EXEC} add-apt-repository ppa:h-rayflood/llvm
    fi

    ${EXEC} apt-get -y update
    ${EXEC} apt-get -y upgrade

    # Install dev tools
    ${EXEC} apt-get -y install \
        ccache \
        make \
        pkg-config \
        linux-libc-dev \
        libgl1-mesa-dev \
        libpulse-dev \
        libjack-dev \
        libasound2-dev \
        libv4l-dev

    if [ "${DOCKERIMG}" != ubuntu:precise ]; then
        ${EXEC} apt-get -y install \
            g++ \
            clang \
            libgstreamer-plugins-base1.0-dev
    else
        ${EXEC} apt-get -y install \
            g++-4.9 \
            clang-3.6
    fi

    # Install Qt dev
    if [ "${DOCKERIMG}" = ubuntu:precise ]; then
        ${EXEC} apt-get -y install \
            qt56tools \
            qt56declarative \
            qt56multimedia \
            qt56svg
    elif [ "${DOCKERIMG}" = ubuntu:trusty ] || \
         [ "${DOCKERIMG}" = ubuntu:xenial ]; then
        ${EXEC} apt-get -y install \
            qt58tools \
            qt58declarative \
            qt58multimedia \
            qt58svg
    else
        ${EXEC} apt-get -y install \
            qt5-qmake \
            qtdeclarative5-dev \
            qtmultimedia5-dev \
            libqt5opengl5-dev \
            libqt5svg5-dev
    fi

    # Install FFmpeg dev
    ${EXEC} apt-get -y install \
        libavcodec-dev \
        libavdevice-dev \
        libavformat-dev \
        libavutil-dev \
        libavresample-dev \
        libswresample-dev \
        libswscale-dev
elif [ "${DOCKERSYS}" = fedora ]; then
    ${EXEC} dnf install -y https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-${FEDORAVER}.noarch.rpm
    ${EXEC} dnf install -y https://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-${FEDORAVER}.noarch.rpm
    ${EXEC} dnf -y update

    ${EXEC} dnf -y install \
        ccache \
        clang \
        make \
        gcc-c++ \
        qt5-qttools-devel \
        qt5-qtdeclarative-devel \
        qt5-qtmultimedia-devel \
        qt5-qtsvg-devel \
        ffmpeg-devel \
        gstreamer1-plugins-base-devel \
        libv4l-devel \
        alsa-lib-devel \
        pulseaudio-libs-devel \
        jack-audio-connection-kit-devel
elif [ "${DOCKERSYS}" = opensuse ]; then
    ${EXEC} zypper -n update

    ${EXEC} zypper -n in \
        ccache \
        clang \
        libqt5-linguist \
        libqt5-qtbase-devel \
        libqt5-qtdeclarative-devel \
        libqt5-qtmultimedia-devel \
        libqt5-qtsvg-devel \
        ffmpeg-devel \
        gstreamer-plugins-base-devel \
        libv4l-devel \
        alsa-devel \
        libpulse-devel \
        libjack-devel
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    brew install \
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
