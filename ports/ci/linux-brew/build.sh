#!/bin/bash

# Webcamoid, camera capture application.
# Copyright (C) 2026  Gonzalo Exequiel Pedone
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

if [ -n "${GITHUB_SHA}" ]; then
    export GIT_COMMIT_HASH="${GITHUB_SHA}"
fi

BREW_PREFIX=/home/linuxbrew/.linuxbrew

BREW_GCC_VERSION=$(ls ${BREW_PREFIX}/lib/gcc 2>/dev/null | grep '[0-9]' | sort -V | tail -1)

if [ "${COMPILER}" = clang ]; then
    COMPILER_C=${BREW_PREFIX}/bin/clang
    COMPILER_CXX=${BREW_PREFIX}/bin/clang++
else
    if [ -z "${BREW_GCC_VERSION}" ]; then
        COMPILER_C=gcc
        COMPILER_CXX=g++
    else
        COMPILER_C=${BREW_PREFIX}/bin/gcc-${BREW_GCC_VERSION}
        COMPILER_CXX=${BREW_PREFIX}/bin/g++-${BREW_GCC_VERSION}
    fi
fi

if [ -z "${DISABLE_CCACHE}" ]; then
    EXTRA_PARAMS="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_OBJCXX_COMPILER_LAUNCHER=ccache"
fi

if [ "${UPLOAD}" = 1 ]; then
    EXTRA_PARAMS="${EXTRA_PARAMS} -DNOLIBAVDEVICE=ON -DNOLIBUVC=ON"
fi

# Apparently this codec is causing Webcamoid to hang in old versions of FFmpeg
EXTRA_PARAMS="${EXTRA_PARAMS} -DFFMPEG_DISABLED_VIDEO_ENCODERS='libsvtav1'"

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

case "${architecture}" in
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

if [[ "${architecture}" = arm32v7 ]]; then
    EXTRA_PARAMS="${EXTRA_PARAMS} -DNOSIMDNEON=ON"
fi

export PATH="${BREW_PREFIX}/bin:${PATH}"
export LDFLAGS="${LDFLAGS} -L${BREW_PREFIX}/lib"
export CPPFLAGS="${CPPFLAGS} -I${BREW_PREFIX}/include"
export PKG_CONFIG_PATH="${BREW_PREFIX}/lib/pkgconfig:${PKG_CONFIG_PATH}"

INSTALL_PREFIX=${PWD}/webcamoid-data-${distro}-${COMPILER}
buildDir=build-${distro}-${COMPILER}
mkdir "${buildDir}"
cmake \
    -LA \
    -S . \
    -B "${buildDir}" \
    -G "Ninja" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
    -DCMAKE_C_COMPILER="${COMPILER_C}" \
    -DCMAKE_CXX_COMPILER="${COMPILER_CXX}" \
    -DCMAKE_PREFIX_PATH="${BREW_PREFIX}" \
    -DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}" \
    -DDAILY_BUILD="${DAILY_BUILD}" \
    ${EXTRA_PARAMS}
cmake --build "${buildDir}" --parallel "$(nproc)"
cmake --install "${buildDir}"
