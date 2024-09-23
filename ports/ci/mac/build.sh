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

COMPILER_C=clang
COMPILER_CXX=clang++

if [ -z "${DISABLE_CCACHE}" ]; then
    EXTRA_PARAMS="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_OBJCXX_COMPILER_LAUNCHER=ccache"
fi

if [ "${UPLOAD}" == 1 ]; then
    EXTRA_PARAMS="${EXTRA_PARAMS} -DNOGSTREAMER=ON -DNOJACK=ON -DNOLIBUVC=ON -DNOPULSEAUDIO=ON"
fi

export PATH="${HOMEBREW_PATH}/opt/qt@6/bin:$PATH"
export LDFLAGS="$LDFLAGS -L${HOMEBREW_PATH}/opt/qt@6/lib"
export CPPFLAGS="$CPPFLAGS -I${HOMEBREW_PATH}/opt/qt@6/include"
export PKG_CONFIG_PATH="${HOMEBREW_PATH}/opt/qt@6/lib/pkgconfig:$PKG_CONFIG_PATH"
export MACOSX_DEPLOYMENT_TARGET="10.14"
INSTALL_PREFIX=${PWD}/webcamoid-data

mkdir build
cmake \
    -LA \
    -S . \
    -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
    -DCMAKE_C_COMPILER="${COMPILER_C}" \
    -DCMAKE_CXX_COMPILER="${COMPILER_CXX}" \
    -DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}" \
    ${EXTRA_PARAMS} \
    -DDAILY_BUILD="${DAILY_BUILD}"
cmake --build build --parallel "${NJOBS}"
cmake --install build
