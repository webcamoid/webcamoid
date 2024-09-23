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

export WINEPREFIX=/opt/.wine
export NSIS_VERSION=3.10
export PATH="${PWD}/nsis-${NSIS_VERSION}:${PWD}/.local/bin:${PATH}"
export MINGW_PREFIX=/usr/${TARGET_ARCH}-w64-mingw32
export INSTALL_PREFIX=${PWD}/webcamoid-data-${COMPILER}-${TARGET_ARCH}
export PACKAGES_DIR=${PWD}/webcamoid-packages/windows-${COMPILER}-${TARGET_ARCH}
export BUILD_PATH=${PWD}/build-${COMPILER}-${TARGET_ARCH}
export PYTHONPATH="${PWD}/DeployTools"
export TEMP="${PWD}/.tmp"

mkdir -p "${TEMP}"

qtInstallBins=$("${MINGW_PREFIX}/lib/qt6/bin/qmake" -query QT_INSTALL_BINS)
cat << EOF > overwrite_syslibdir.conf
[System]
libDir = ${qtInstallBins}, ${MINGW_PREFIX}/bin
EOF

cat << EOF > package_info_strip.conf
[System]
stripCmd = ${TARGET_ARCH}-w64-mingw32-strip
EOF

python DeployTools/deploy.py \
    -d "${INSTALL_PREFIX}" \
    -c "${BUILD_PATH}/package_info.conf" \
    -c "${BUILD_PATH}/package_info_windows.conf" \
    -c "${PWD}/overwrite_syslibdir.conf" \
    -c "${PWD}/package_info_strip.conf" \
    -o "${PACKAGES_DIR}"
