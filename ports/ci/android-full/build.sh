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
export ANDROID_NDK_HOST=linux-x86_64
export ANDROID_NDK_PLATFORM=android-${ANDROID_PLATFORM}
export PATH="${JAVA_HOME}/bin/java:${PATH}"
export ORIG_PATH="${PATH}"

LRELEASE_TOOL="${PWD}/Qt/${QTVER_ANDROID}/gcc_64/bin/lrelease"
LUPDATE_TOOL="${PWD}/Qt/${QTVER_ANDROID}/gcc_64/bin/lupdate"

mkdir -p build

for arch_ in $(echo "${TARGET_ARCH}" | tr ":" "\n"); do
    abi=${arch_}

    case "${arch_}" in
        arm64-v8a)
            arch_=arm64_v8a
            ;;
        armeabi-v7a)
            arch_=armv7
            ;;
        *)
            ;;
    esac

    envArch=${arch_}

    case "${arch_}" in
        arm64-v8a)
            envArch=aarch64
            ;;
        armeabi-v7a)
            envArch=armv7a-eabi
            ;;
        x86)
            envArch=arm64_v8a
            ;;
        x86_64)
            envArch=x86-64
            ;;
        *)
            ;;
    esac

    export PATH="${ORIG_PATH}"x

    if which android-env > /dev/null; then
        source android-env "${envArch}"
    fi

    export ANDROID_SDK=${ANDROID_HOME}
    export ANDROID_NDK=${ANDROID_NDK_HOME}
    export ANDROID_SDK_ROOT=${ANDROID_HOME}
    export ANDROID_NDK_ROOT=${ANDROID_NDK_HOME}
    export PATH="${ANDROID_HOME}/tools:${ANDROID_HOME}/tools/bin"
    export PATH="${ANDROID_HOME}/platform-tools:${PATH}"
    export PATH="${ANDROID_HOME}/emulator:${PATH}"
    export PATH="${ANDROID_NDK_HOME}:${PATH}"
    export PATH="${PWD}/Qt/${QTVER_ANDROID}/gcc_64/libexec:${PATH}"
    export PATH="${PWD}/Qt/${QTVER_ANDROID}/android_${arch_}/bin:${PATH}"
    buildDir=build-${abi}
    mkdir -p "${buildDir}"
    qt-cmake \
        -LA \
        -S . \
        -B "${buildDir}" \
        -G "Unix Makefiles" \
        -DCMAKE_BUILD_TYPE=Release \
        -DQT_HOST_PATH="${PWD}/Qt/${QTVER_ANDROID}/gcc_64" \
        -DANDROID_PLATFORM="${ANDROID_PLATFORM}" \
        -DANDROID_SDK_ROOT="${ANDROID_HOME}" \
        -DANDROID_NDK_ROOT="${ANDROID_NDK_HOME}" \
        -DLRELEASE_TOOL="${LRELEASE_TOOL}" \
        -DLUPDATE_TOOL="${LUPDATE_TOOL}" \
        ${EXTRA_PARAMS} \
        -DDAILY_BUILD="${DAILY_BUILD}"
    cmake --build "${buildDir}" --parallel "${NJOBS}"
    cp -vf "${buildDir}/package_info.conf" build/
    cp -vf "${buildDir}/package_info_android.conf" build/
done
