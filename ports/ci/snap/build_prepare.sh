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
else
    branch=${CIRRUS_BASE_BRANCH}
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
      - -DBUILD_RPATH_USE_ORIGIN=1
      - -DDAILY_BUILD=${DAILY_BUILD}
    source: https://github.com/webcamoid/webcamoid.git
    build-packages:
      - file
      - g++
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
      - libqt5opengl5-dev
      - libqt5svg5-dev
      - libsdl2-dev
      - libswresample-dev
      - libswscale-dev
      - libusb-dev
      - libuvc-dev
      - libv4l-dev
      - libvlc-dev
      - libvlccore-dev
      - linux-libc-dev
      - make
      - pkg-config
      - portaudio19-dev
      - qml-module-qt-labs-folderlistmodel
      - qml-module-qt-labs-platform
      - qml-module-qt-labs-settings
      - qml-module-qtqml-models2
      - qml-module-qtquick-controls2
      - qml-module-qtquick-dialogs
      - qml-module-qtquick-extras
      - qml-module-qtquick-privatewidgets
      - qml-module-qtquick-templates2
      - qt5-qmake
      - qtdeclarative5-dev
      - qtmultimedia5-dev
      - qtquickcontrols2-5-dev
      - qttools5-dev-tools
      - qtwayland5
      - vlc-plugin-base
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
      - libqt5gui5
      - libqt5multimedia5
      - libqt5network5
      - libqt5svg5
      - libresid-builder0c2a
      - libsdl2-2.0-0
      - libsidplay2
      - libspa-0.2-modules
      - libtinfo6
      - libv4l-0
      - libv4lconvert0
      - libvlc5
      - qml-module-qt-labs-platform
      - qml-module-qt-labs-settings
      - qml-module-qtgraphicaleffects
      - qml-module-qtquick-controls2
      - qml-module-qtquick-layouts
      - qml-module-qtquick-templates2
      - qml-module-qtquick-window2
      - qml-module-qtquick2
      - qt5-qmltooling-plugins
      - qtwayland5
      - vlc-plugin-base

apps:
  webcamoid:
    command: usr/bin/webcamoid
    common-id: io.github.webcamoid.Webcamoid
    desktop: usr/share/applications/webcamoid.desktop
    environment:
      SNAP_DESKTOP_RUNTIME: \$SNAP
      LD_LIBRARY_PATH: \$SNAP/opt/qt515/lib
      QT_PLUGIN_PATH: \$SNAP/usr/lib/\$SNAPCRAFT_ARCH_TRIPLET/qt5/plugins:\$QT_PLUGIN_PATH
      QML_IMPORT_PATH: \$SNAP/usr/lib/\$SNAPCRAFT_ARCH_TRIPLET/qt5/qml:\$QML_IMPORT_PATH
      QML2_IMPORT_PATH: \$SNAP/usr/lib/\$SNAPCRAFT_ARCH_TRIPLET/qt5/qml:\$QML2_IMPORT_PATH
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
