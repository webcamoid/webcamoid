#!/bin/bash

# Webcamoid, webcam capture application.
# Copyright (C) 2019  Gonzalo Exequiel Pedone
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

[ -f environment.sh ] && source environment.sh

set -e

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
    EXTRA_PARAMS="${EXTRA_PARAMS} -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_OBJCXX_COMPILER_LAUNCHER=ccache"
fi

if [ "${PLATFORM}" = x86 ]; then
    export PATH=/mingw32/bin:$PATH
else
    export PATH=/mingw64/bin:$PATH
fi

INSTALL_PREFIX=${PWD}/webcamoid-data-${COMPILER}-${PLATFORM}
buildDir=build-${COMPILER}-${PLATFORM}
mkdir "${buildDir}"
cmake \
    -LA \
    -S . \
    -B "${buildDir}" \
    -G "MSYS Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
    -DCMAKE_C_COMPILER="${COMPILER_C}" \
    -DCMAKE_CXX_COMPILER="${COMPILER_CXX}" \
    -DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}" \
    ${EXTRA_PARAMS} \
    -DDAILY_BUILD="${DAILY_BUILD}"
cmake --build "${buildDir}" --parallel "${NJOBS}"
cmake --install "${buildDir}"
