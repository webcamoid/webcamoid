#!/bin/sh

if [ "${DOCKERSYS}" = debian ]; then
    docker exec ${DOCKERSYS} apt-get -y update
    docker exec ${DOCKERSYS} apt-get -y upgrade

    docker exec ${DOCKERSYS} apt-get -y install \
        ccache \
        clang \
        pkg-config \
        linux-libc-dev \
        qt5-qmake \
        qtdeclarative5-dev \
        qtmultimedia5-dev \
        libqt5opengl5-dev \
        libqt5svg5-dev \
        libavcodec-dev \
        libavdevice-dev \
        libavformat-dev \
        libavutil-dev \
        libswresample-dev \
        libswscale-dev \
        libgstreamer-plugins-base1.0-dev \
        libpulse-dev \
        libjack-dev \
        libasound2-dev \
        libv4l-dev
elif [ "${DOCKERSYS}" = fedora ]; then
    docker exec ${DOCKERSYS} yum -y update

    docker exec ${DOCKERSYS} yum -y install \
        ccache \
        clang \
        gcc-c++ \
        qt5-qttools-devel \
        qt5-qtdeclarative-devel \
        qt5-qtsvg-devel \
        ffmpeg-devel \
        libv4l-devel \
        pulseaudio-libs-devel
elif [ "${DOCKERSYS}" = opensuse ]; then
    docker exec ${DOCKERSYS} zypper -n update

    docker exec ${DOCKERSYS} zypper -n in \
        ccache \
        clang \
        libqt5-linguist \
        libqt5-qtbase-devel \
        libqt5-qtdeclarative-devel \
        libqt5-qtsvg-devel \
        ffmpeg-devel \
        libv4l-devel \
        libpulse-devel
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
