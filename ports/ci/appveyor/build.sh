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

if [ ! -z "${DAILY_BUILD}" ] || [ ! -z "${RELEASE_BUILD}" ]; then
    EXTRA_PARAMS="-DNOGSTREAMER=ON -DNOLIBAVDEVICE=ON"
fi

if [ "${COMPILER}" = clang ]; then
    COMPILER_C=clang
    COMPILER_CXX=clang++
else
    COMPILER_C=gcc
    COMPILER_CXX=g++
fi

if [ -z "${DISABLE_CCACHE}" ]; then
    EXTRA_PARAMS="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
fi

if [ "${PLATFORM}" = x86 ]; then
    export PATH=/mingw32/bin:$PATH
else
    export PATH=/mingw64/bin:$PATH

#     export MINGW_CHOST=x86_64-w64-mingw32
#     export MINGW_PACKAGE_PREFIX=mingw-w64-x86_64
#     export MINGW_PREFIX=/mingw64
#     export MSYSCON=mintty.exe
#
#     export MSYSTEM=MINGW64
#     export MSYSTEM_CARCH=x86_64
#     export MSYSTEM_CHOST=x86_64-w64-mingw32
#     export MSYSTEM_PREFIX=/mingw64
fi

INSTALL_PREFIX=${APPVEYOR_BUILD_FOLDER}/webcamoid-data

mkdir build
cd build
qmake -query
echo
env
echo
cmake -LA .
echo
cmake \
    -G "MSYS Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
    -DCMAKE_C_COMPILER="${COMPILER_C}" \
    -DCMAKE_CXX_COMPILER="${COMPILER_CXX}" \
    -DDAILY_BUILD=${DAILY_BUILD} \
    ${EXTRA_PARAMS} \
    ..
cmake --build .
cmake --build . --target install
