#!/bin/bash

# Webcamoid, webcam capture application.
# Copyright (C) 2024  Gonzalo Exequiel Pedone
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

if [ -z "${DISABLE_CCACHE}" ]; then
    EXTRA_PARAMS="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_OBJCXX_COMPILER_LAUNCHER=ccache"
fi

if [ "${UPLOAD}" == 1 ]; then
    EXTRA_PARAMS="${EXTRA_PARAMS} -DNOGSTREAMER=ON -DNOLIBAVDEVICE=ON"
fi

if [ "${ENABLE_ADS}" == 1 ]; then
    EXTRA_PARAMS="${EXTRA_PARAMS} -DENABLE_ANDROID_ADS=ON"

    if [ "${DAILY_BUILD}" != 1 ]; then
        echo "Setting real ads"
        EXTRA_PARAMS="${EXTRA_PARAMS} -DANDROID_AD_APPID='${ANDROID_AD_APPID}'"
        EXTRA_PARAMS="${EXTRA_PARAMS} -DANDROID_AD_UNIT_ID_APP_OPEN='${ANDROID_AD_UNIT_ID_APP_OPEN}'"
        EXTRA_PARAMS="${EXTRA_PARAMS} -DANDROID_AD_UNIT_ID_BANNER='${ANDROID_AD_UNIT_ID_BANNER}'"
        EXTRA_PARAMS="${EXTRA_PARAMS} -DANDROID_AD_UNIT_ID_ADAPTIVE_BANNER='${ANDROID_AD_UNIT_ID_ADAPTIVE_BANNER}'"
        EXTRA_PARAMS="${EXTRA_PARAMS} -DANDROID_AD_UNIT_ID_ADAPTIVE_INTERSTITIAL='${ANDROID_AD_UNIT_ID_ADAPTIVE_INTERSTITIAL}'"
    else
        echo "Setting test ads"
    fi
fi

export JAVA_HOME=$(readlink -f /usr/bin/java | sed 's:bin/java::')
export ANDROID_HOME="/opt/android-sdk"
export ANDROID_NDK="/opt/android-ndk"
export ANDROID_NDK_HOME=${ANDROID_NDK}
export ANDROID_NDK_HOST=linux-x86_64
export ANDROID_NDK_PLATFORM=android-${ANDROID_MINIMUM_PLATFORM}
export ANDROID_NDK_ROOT=${ANDROID_NDK}
export ANDROID_SDK_ROOT=${ANDROID_HOME}
export PATH="${JAVA_HOME}/bin/java:${PATH}"
export PATH="$PATH:${ANDROID_HOME}/tools:${ANDROID_HOME}/tools/bin"
export PATH="${PATH}:${ANDROID_HOME}/platform-tools"
export PATH="${PATH}:${ANDROID_HOME}/emulator"
export PATH="${PATH}:${ANDROID_NDK}"
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
        arm64_v8a)
            envArch=aarch64
            ;;
        armv7)
            envArch=armv7a-eabi
            ;;
        x86_64)
            envArch=x86-64
            ;;
        *)
            ;;
    esac

    export ANDROID_EXTERNAL_LIBS=/opt/android-libs
    export ANDROID_PREFIX=${ANDROID_EXTERNAL_LIBS}/${envArch}
    export ANDROID_PREFIX_LIB=${ANDROID_PREFIX}/lib
    export ANDROID_PREFIX_SHARE=${ANDROID_PREFIX}/share
    export PKG_CONFIG_SYSROOT_DIR=${ANDROID_PREFIX}
    export PKG_CONFIG_LIBDIR=${ANDROID_PREFIX_LIB}/pkgconfig:${ANDROID_PREFIX_SHARE}/pkgconfig
    export PATH="${PWD}/Qt/${QTVER_ANDROID}/gcc_64/libexec:${ORIG_PATH}"
    export PATH="${PWD}/Qt/${QTVER_ANDROID}/android_${arch_}/bin:${PATH}"
    QMAKE_EXECUTABLE="${PWD}/Qt/${QTVER_ANDROID}/android_${arch_}/bin/qmake"
    buildDir=build-${abi}
    mkdir -p "${buildDir}"
    qt-cmake \
        -LA \
        -S . \
        -B "${buildDir}" \
        -G "Unix Makefiles" \
        -DCMAKE_BUILD_TYPE=Release \
        -DQT_HOST_PATH="${PWD}/Qt/${QTVER_ANDROID}/gcc_64" \
        -DANDROID_PLATFORM="${ANDROID_MINIMUM_PLATFORM}" \
        -DANDROID_SDK_ROOT="${ANDROID_HOME}" \
        -DANDROID_NDK_ROOT="${ANDROID_NDK}" \
        -DQT_QMAKE_EXECUTABLE="${QMAKE_EXECUTABLE}" \
        -DLRELEASE_TOOL="${LRELEASE_TOOL}" \
        -DLUPDATE_TOOL="${LUPDATE_TOOL}" \
        -DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}" \
        -DQT_NO_GLOBAL_APK_TARGET_PART_OF_ALL=ON \
        ${EXTRA_PARAMS} \
        -DDAILY_BUILD="${DAILY_BUILD}"
    cmake --build "${buildDir}" --parallel "${NJOBS}"
    cp -vf "${buildDir}/package_info.conf" build/
    cp -vf "${buildDir}/package_info_android.conf" build/
done
