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

if [ -z "${DISABLE_CCACHE}" ]; then
    EXTRA_PARAMS="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_OBJCXX_COMPILER_LAUNCHER=ccache"
fi

if [ "${UPLOAD}" == 1 ]; then
    EXTRA_PARAMS="${EXTRA_PARAMS} -DNOGSTREAMER=ON -DNOLIBAVDEVICE=ON"
fi

export JAVA_HOME=$(readlink -f /usr/bin/java | sed 's:bin/java::')
export ANDROID_HOME="${PWD}/build/android-sdk"
export ANDROID_NDK="${PWD}/build/android-ndk"
export ANDROID_NDK_HOME=${ANDROID_NDK}
export ANDROID_NDK_HOST=linux-x86_64
export ANDROID_NDK_PLATFORM=android-${ANDROID_PLATFORM}
export ANDROID_NDK_ROOT=${ANDROID_NDK}
export ANDROID_SDK_ROOT=${ANDROID_HOME}
export PATH="${JAVA_HOME}/bin/java:${PATH}"
export PATH="$PATH:${ANDROID_HOME}/tools:${ANDROID_HOME}/tools/bin"
export PATH="${PATH}:${ANDROID_HOME}/platform-tools"
export PATH="${PATH}:${ANDROID_HOME}/emulator"
export PATH="${PATH}:${ANDROID_NDK}"
export ORIG_PATH="${PATH}"
mkdir -p build

for arch_ in $(echo "${TARGET_ARCH}" | tr ":" "\n"); do
    export PATH="${PWD}/build/Qt/${QTVER_ANDROID}/android/bin:${ORIG_PATH}"
    buildDir=build-${arch_}
    mkdir "${buildDir}"
    cmake \
        -LA \
        -S . \
        -B "${buildDir}" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_COMPILER="${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang" \
        -DCMAKE_CXX_COMPILER="${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++" \
        -DANDROID_NATIVE_API_LEVEL="${ANDROID_PLATFORM}" \
        -DANDROID_PLATFORM="${ANDROID_PLATFORM}" \
        -DANDROID_NDK="${ANDROID_NDK}" \
        -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK}/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="${arch_}" \
        -DANDROID_STL=c++_shared \
        -DCMAKE_FIND_ROOT_PATH="$(qmake -query QT_INSTALL_PREFIX)" \
        -DANDROID_SDK_ROOT="${ANDROID_HOME}" \
        ${EXTRA_PARAMS} \
        -DDAILY_BUILD="${DAILY_BUILD}"
    cmake --build "${buildDir}" --parallel "${NJOBS}"
    cp -vf "${buildDir}/package_info.conf" build/
    cp -vf "${buildDir}/package_info_android.conf" build/
done
