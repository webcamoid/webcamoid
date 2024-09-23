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

export PYTHONPATH="${PWD}/DeployTools"

export JAVA_HOME=$(readlink -f /usr/bin/java | sed 's:bin/java::')
export ANDROID_HOME="${PWD}/build/android-sdk"
export ANDROID_NDK="${PWD}/build/android-ndk"
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
export KEYSTORE_PATH="${PWD}/keystores/debug.keystore"
nArchs=$(echo "${TARGET_ARCH}" | tr ':' ' ' | wc -w)
lastArch=$(echo "${TARGET_ARCH}" | awk -F: '{print $NF}')

cat << EOF > speedup_apk_build.conf
[AndroidAPK]
gradleParallel = true
gradleDaemon = true
gradleConfigureOnDemand = true
EOF

cat << EOF > package_info_sdkbt.conf
[System]
sdkBuildToolsRevision = ${ANDROID_BUILD_TOOLS_VERSION}
EOF

if [ "${nArchs}" = 1 ]; then
    arch_=${lastArch}

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

    export PATH="${PWD}/build/Qt/${QTVER_ANDROID}/gcc_64/libexec:${PWD}/.local/bin:${ORIG_PATH}"
    export PATH="${PWD}/build/Qt/${QTVER_ANDROID}/android_${arch_}/bin:${PATH}"
    export PACKAGES_DIR=${PWD}/webcamoid-packages/android
    export BUILD_PATH=${PWD}/build-${lastArch}

    python3 DeployTools/deploy.py \
        -d "${BUILD_PATH}/android-build" \
        -c "${BUILD_PATH}/package_info.conf" \
        -c "${BUILD_PATH}/package_info_android.conf" \
        -c "${PWD}/package_info_sdkbt.conf" \
        -c "${PWD}/speedup_apk_build.conf" \
        -o "${PACKAGES_DIR}"
else
    export PACKAGES_DIR=${PWD}/webcamoid-packages/android
    mkdir -p "${PWD}/webcamoid-data"

    abiFilters=$(echo "${TARGET_ARCH}" | tr ":" ",")
    cat << EOF > package_info_multiarch.conf
[Android]
ndkABIFilters = $abiFilters
EOF

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

        export PATH="${PWD}/build/Qt/${QTVER_ANDROID}/gcc_64/libexec:${PWD}/.local/bin:${ORIG_PATH}"
        export PATH="${PWD}/build/Qt/${QTVER_ANDROID}/android_${arch_}/bin:${PATH}"
        export BUILD_PATH=${PWD}/build-${abi}

        python3 DeployTools/deploy.py \
            -r \
            -d "${BUILD_PATH}/android-build" \
            -c "${BUILD_PATH}/package_info.conf" \
            -c "${BUILD_PATH}/package_info_android.conf" \
            -c "${PWD}/package_info_sdkbt.conf" \
            -c "${PWD}/package_info_multiarch.conf"
        cp -rf "${BUILD_PATH}/android-build"/* "${PWD}/webcamoid-data"
    done

    # Create the multi-architecture package

    cat << EOF > package_info_hide_arch.conf
[Package]
targetArch = any
hideArch = true
EOF

    cat << EOF > build_aab_package.conf
[AndroidAPK]
packageTypes = apk, aab
EOF

    python3 DeployTools/deploy.py \
        -s \
        -d "${PWD}/webcamoid-data" \
        -c "${PWD}/build/package_info.conf" \
        -c "${PWD}/build/package_info_android.conf" \
        -c "${PWD}/package_info_sdkbt.conf" \
        -c "${PWD}/package_info_hide_arch.conf" \
        -c "${PWD}/speedup_apk_build.conf" \
        -c "${PWD}/build_aab_package.conf" \
        -o "${PACKAGES_DIR}"
fi
