#!/bin/bash

# Webcamoid, webcam capture application.
# Copyright (C) 2023  Gonzalo Exequiel Pedone
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

if [ "${GITHUB_SHA}" != "" ]; then
    branch=${GITHUB_REF##*/}
    commitSha=${GITHUB_SHA}
else
    branch=${CIRRUS_BASE_BRANCH}
    commitSha=${CIRRUS_BASE_SHA}
fi

if [ "${branch}" = "" ]; then
    branch=$(git rev-parse --abbrev-ref HEAD)
fi

if [ "${commitSha}" = "" ]; then
    commitSha=$(git rev-parse "origin/${branch}")
fi

if [ "${DAILY_BUILD}" != 1 ]; then
    export DAILY_BUILD=0
fi

if [ "${DAILY_BUILD}" = 1 ]; then
    version=daily-${branch}
else
    verMaj=$(grep VER_MAJ libAvKys/cmake/ProjectCommons.cmake | awk '{print $2}' | tr -d ')' | head -n 1)
    verMin=$(grep VER_MIN libAvKys/cmake/ProjectCommons.cmake | awk '{print $2}' | tr -d ')' | head -n 1)
    verPat=$(grep VER_PAT libAvKys/cmake/ProjectCommons.cmake | awk '{print $2}' | tr -d ')' | head -n 1)
    version=${verMaj}.${verMin}.${verPat}
fi

mkdir -p snap
cat << EOF > snap/snapcraft.yaml
name: webcamoid
version: "${version}"
summary: Full featured and multiplatform webcam suite
description: |
  - Cross-platform (GNU/Linux, Mac, Windows, Android)
  - Take pictures and record videos with the webcam.
  - Manages multiple webcams.
  - Written in C++ and Qt.
  - Custom controls for each webcam.
  - Add funny effects to the webcam.
  - 60+ effects available.
  - Translated to many languages.
  - Use custom network and local files as capture devices.
  - Capture from desktop.
  - Many recording formats.
  - Virtual webcam support for feeding other programs.
base: core22
grade: stable
confinement: strict
compression: lzo
architectures:
  - build-on: amd64

parts:
  webcamoid:
    plugin: cmake
    cmake-parameters:
      - -DCMAKE_INSTALL_PREFIX=/usr
      - -DCMAKE_BUILD_TYPE=Release
      - -DDAILY_BUILD=${DAILY_BUILD}
    source: https://github.com/webcamoid/webcamoid.git
    source-branch: ${branch}
    source-commit: ${commitSha}
    build-packages:
      - g++
      - git
      - gstreamer1.0-plugins-base
      - gstreamer1.0-plugins-good
      - libasound2-dev
      - libavcodec-dev
      - libavdevice-dev
      - libavformat-dev
      - libavutil-dev
      - libgl1-mesa-dev
      - libgstreamer-plugins-base1.0-dev
      - libgstreamer1.0-0
      - libjack-dev
      - libkmod-dev
      - libpipewire-0.3-dev
      - libpulse-dev
      - libqt6opengl6-dev
      - libqt6svg6-dev
      - libsdl2-dev
      - libswresample-dev
      - libswscale-dev
      - libusb-dev
      - libuvc-dev
      - libv4l-dev
      - libvlc-dev
      - libvlccore-dev
      - libxext-dev
      - libxfixes-dev
      - linux-libc-dev
      - make
      - patchelf
      - pkg-config
      - portaudio19-dev
      - qmake6
      - qml6-module-qt-labs-folderlistmodel
      - qml6-module-qt-labs-platform
      - qml6-module-qt-labs-settings
      - qml6-module-qtqml-models
      - qml6-module-qtquick-controls
      - qml6-module-qtquick-dialogs
      - qml6-module-qtquick3d-spatialaudio
      - qt6-declarative-dev
      - qt6-l10n-tools
      - qt6-multimedia-dev
      - qt6-wayland
      - vlc-plugin-base
      - xvfb
    stage-packages:
      - freeglut3
      - libavdevice58
      - libavfilter7
      - libbz2-1.0
      - libglu1-mesa
      - libgstreamer-plugins-base1.0-0
      - libgstreamer1.0-0
      - libncursesw6
      - libpipewire-0.3-modules
      - libportaudio2
      - libqt6gui6
      - libqt6multimedia6
      - libqt6network6
      - libqt6svg6
      - libresid-builder0c2a
      - libsdl2-2.0-0
      - libsidplay2
      - libspa-0.2-modules
      - libtinfo6
      - libv4l-0
      - libv4lconvert0
      - libvlc5
      - qml6-module-qt-labs-folderlistmodel
      - qml6-module-qt-labs-platform
      - qml6-module-qt-labs-settings
      - qml6-module-qtqml-models
      - qml6-module-qtquick-controls
      - qml6-module-qtquick-dialogs
      - qt6-qmltooling-plugins
      - qt6-wayland
      - vlc-plugin-base

apps:
  webcamoid:
    command: usr/bin/webcamoid
    common-id: io.github.webcamoid.Webcamoid
    desktop: usr/share/applications/webcamoid.desktop
    environment:
      SNAP_DESKTOP_RUNTIME: \$SNAP
      QT_PLUGIN_PATH: \$SNAP/usr/lib/\$SNAPCRAFT_ARCH_TRIPLET/qt6/plugins:\$QT_PLUGIN_PATH
      QML_IMPORT_PATH: \$SNAP/usr/lib/\$SNAPCRAFT_ARCH_TRIPLET/qt6/qml:\$QML_IMPORT_PATH
      QML2_IMPORT_PATH: \$SNAP/usr/lib/\$SNAPCRAFT_ARCH_TRIPLET/qt6/qml:\$QML2_IMPORT_PATH
      PYTHONPATH: \$PYTHONPATH:\$SNAP/usr/lib/python3/dist-packages
      ALSA_CONFIG_PATH: \$SNAP/usr/share/alsa/alsa.conf
      PIPEWIRE_CONFIG_NAME: \$SNAP/usr/share/pipewire/pipewire.conf
      PIPEWIRE_MODULE_DIR: \$SNAP/usr/lib/\$SNAPCRAFT_ARCH_TRIPLET/pipewire-0.3
      SPA_PLUGIN_DIR: \$SNAP/usr/lib/\$SNAPCRAFT_ARCH_TRIPLET/spa-0.2
      GST_PLUGIN_PATH: \$SNAP/usr/lib/\$SNAPCRAFT_ARCH_TRIPLET/gstreamer-1.0
      GST_PLUGIN_SCANNER: \$SNAP/usr/lib/\$SNAPCRAFT_ARCH_TRIPLET/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner
      VLC_PLUGIN_PATH: \$SNAP/usr/lib/\$SNAPCRAFT_ARCH_TRIPLET/vlc/plugins
    plugs:
       - home
       - desktop
       - desktop-legacy
       - opengl
       - wayland
       - x11
       - unity7
       - camera
       - audio-record
       - audio-playback
       - network
       - network-status
       - network-bind
       - bluez
       - screen-inhibit-control
       - removable-media
       - optical-drive

slots:
    webcamoid:
       interface: dbus
       bus: session
       name: io.github.webcamoid.Webcamoid
EOF
