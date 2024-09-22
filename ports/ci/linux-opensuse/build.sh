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
set -o errexit

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

QMAKE_EXECUTABLE=/usr/bin/qmake6
LRELEASE_TOOL=/usr/bin/lrelease6
LUPDATE_TOOL=/usr/bin/lupdate6

export PATH=$HOME/.local/bin:$PATH
INSTALL_PREFIX=${PWD}/webcamoid-data-${COMPILER}
buildDir=build-${COMPILER}
mkdir "${buildDir}"
cmake \
    -LA \
    -S . \
    -B "${buildDir}" \
    -DQT_QMAKE_EXECUTABLE=qmake-qt6 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
    -DCMAKE_C_COMPILER="${COMPILER_C}" \
    -DCMAKE_CXX_COMPILER="${COMPILER_CXX}" \
    -DQT_QMAKE_EXECUTABLE="${QMAKE_EXECUTABLE}" \
    -DLRELEASE_TOOL="${LRELEASE_TOOL}" \
    -DLUPDATE_TOOL="${LUPDATE_TOOL}" \
    -DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}" \
    ${EXTRA_PARAMS} \
    -DGST_PLUGINS_SCANNER_PATH=/usr/libexec/gstreamer-1.0/gst-plugin-scanner-x86_64 \
    -DDAILY_BUILD="${DAILY_BUILD}"
cmake --build "${buildDir}" --parallel "${NJOBS}"
cmake --install "${buildDir}"
