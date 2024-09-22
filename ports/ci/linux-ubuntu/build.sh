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

if [ ! -z "${GITHUB_SHA}" ]; then
    export GIT_COMMIT_HASH="${GITHUB_SHA}"
elif [ ! -z "${CIRRUS_CHANGE_IN_REPO}" ]; then
    export GIT_COMMIT_HASH="${CIRRUS_CHANGE_IN_REPO}"
fi

if [ "${COMPILER}" = clang ]; then
    COMPILER_C=clang
    COMPILER_CXX=clang++
else
    COMPILER_C=gcc
    COMPILER_CXX=g++
fi

if [ -z "${DISABLE_CCACHE}" ]; then
    EXTRA_PARAMS="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_OBJCXX_COMPILER_LAUNCHER=ccache"
fi

if [ "${UPLOAD}" == 1 ]; then
    EXTRA_PARAMS="${EXTRA_PARAMS} -DNOGSTREAMER=ON -DNOLIBAVDEVICE=ON -DNOLIBUVC=ON"
fi

export PATH=${HOME}/.local/bin:${PATH}

if [ -z "${DISTRO}" ]; then
    distro=${DOCKERIMG#*:}
else
    distro=${DISTRO}
fi

if [ -z "${ARCHITECTURE}" ]; then
    architecture="${DOCKERIMG%%/*}"
else
    case "${ARCHITECTURE}" in
        aarch64)
            architecture=arm64v8
            ;;
        armv7)
            architecture=arm32v7
            ;;
        *)
            architecture=${ARCHITECTURE}
            ;;
    esac
fi

case "$architecture" in
    arm64v8)
        libArchDir=aarch64-linux-gnu
        ;;
    arm32v7)
        libArchDir=arm-linux-gnueabihf
        ;;
    *)
        libArchDir=x86_64-linux-gnu
        ;;
esac

QMAKE_EXECUTABLE=/usr/lib/qt6/bin/qmake
LRELEASE_TOOL=/usr/lib/qt6/bin/lrelease
LUPDATE_TOOL=/usr/lib/qt6/bin/lupdate

INSTALL_PREFIX=${PWD}/webcamoid-data-${distro}-${COMPILER}
buildDir=build-${distro}-${COMPILER}
mkdir "${buildDir}"
cmake \
    -LA \
    -S . \
    -B "${buildDir}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
    -DCMAKE_C_COMPILER="${COMPILER_C}" \
    -DCMAKE_CXX_COMPILER="${COMPILER_CXX}" \
    -DQT_QMAKE_EXECUTABLE="${QMAKE_EXECUTABLE}" \
    -DLRELEASE_TOOL="${LRELEASE_TOOL}" \
    -DLUPDATE_TOOL="${LUPDATE_TOOL}" \
    -DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}" \
    ${EXTRA_PARAMS} \
    -DGST_PLUGINS_SCANNER_PATH="/usr/lib/${libArchDir}/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner" \
    -DDAILY_BUILD="${DAILY_BUILD}"
cmake --build "${buildDir}" --parallel "${NJOBS}"
cmake --install "${buildDir}"
