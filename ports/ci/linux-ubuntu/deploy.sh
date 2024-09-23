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

set -e

if [ ! -z "${GITHUB_SHA}" ]; then
    export GIT_COMMIT_HASH="${GITHUB_SHA}"
elif [ ! -z "${CIRRUS_CHANGE_IN_REPO}" ]; then
    export GIT_COMMIT_HASH="${CIRRUS_CHANGE_IN_REPO}"
fi

export GIT_BRANCH_NAME=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)

if [ -z "${GIT_BRANCH_NAME}" ]; then
    if [ ! -z "${GITHUB_REF_NAME}" ]; then
        export GIT_BRANCH_NAME="${GITHUB_REF_NAME}"
    elif [ ! -z "${CIRRUS_BRANCH}" ]; then
        export GIT_BRANCH_NAME="${CIRRUS_BRANCH}"
    else
        export GIT_BRANCH_NAME=master
    fi
fi

git clone https://github.com/webcamoid/DeployTools.git

if [ -z "${DISTRO}" ]; then
    distro=${DOCKERIMG#*:}
else
    distro=${DISTRO}
fi

export PATH="${PWD}/.local/bin:${PATH}"
export INSTALL_PREFIX=${PWD}/webcamoid-data-${distro}-${COMPILER}
export PACKAGES_DIR=${PWD}/webcamoid-packages/linux-${distro}-${COMPILER}
export BUILD_PATH=${PWD}/build-${distro}-${COMPILER}
export PYTHONPATH="${PWD}/DeployTools"

cat << EOF > force_plugins_copy.conf
[Qt]
extraPlugins = egldeviceintegrations, multimedia, xcbglintegrations, wayland-decoration-client, wayland-graphics-integration-client, wayland-graphics-integration-server, wayland-shell-integration

[PipeWire]
havePipeWire = true

[Vlc]
haveVLC = true
EOF

if [ "${UPLOAD}" != 1 ]; then
    cat << EOF >> force_plugins_copy.conf

[GStreamer]
haveGStreamer = true
EOF
fi

cat << EOF > description.txt
Webcamoid is a multi-platform camera suite with many features like:

* Cross-platform (GNU/Linux, Mac, Windows, Android, FreeBSD)
* Take pictures and record videos with the webcam.
* Manages multiple webcams.
* Written in C++ and Qt.
* Custom controls for each webcam.
* Add funny effects to the webcam.
* 60+ effects available.
* Translated to many languages.
* Use custom network and local files as capture devices.
* Capture from desktop.
* Many recording formats.
* Virtual webcam support for feeding other programs (GNU/Linux, Mac, Windows)
EOF

cat << EOF > package_description.conf
[DebPackage]
descriptionFile = ${PWD}/description.txt

[RpmPackage]
descriptionFile = ${PWD}/description.txt
EOF

xvfb-run --auto-servernum python3 DeployTools/deploy.py \
    -d "${INSTALL_PREFIX}" \
    -c "${BUILD_PATH}/package_info.conf" \
    -c "${PWD}/force_plugins_copy.conf" \
    -c "${PWD}/package_description.conf" \
    -o "${PACKAGES_DIR}"
