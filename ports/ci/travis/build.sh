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

INSTALL_PREFIX=${TRAVIS_BUILD_DIR}/webcamoid-data

if [ "${COMPILER}" = clang ]; then
    COMPILER_C=clang
    COMPILER_CXX=clang++
else
    COMPILER_C=gcc
    COMPILER_CXX=g++
fi

if [ ! -z "${ARCH_ROOT_MINGW}" ]; then
    COMPILER_C=${ARCH_ROOT_MINGW}-w64-mingw32-${COMPILER_C}
    COMPILER_CXX=${ARCH_ROOT_MINGW}-w64-mingw32-${COMPILER_CXX}
fi

if [ -z "${DISABLE_CCACHE}" ]; then
    EXTRA_PARAMS="-DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
fi

if [ ! -z "${DAILY_BUILD}" ] || [ ! -z "${RELEASE_BUILD}" ]; then
    if [ "${TRAVIS_OS_NAME}" = linux ]; then
        EXTRA_PARAMS="${EXTRA_PARAMS} -DNOGSTREAMER=ON -DNOLIBAVDEVICE=ON"
    elif [ "${TRAVIS_OS_NAME}" = osx ]; then
        EXTRA_PARAMS="${EXTRA_PARAMS}  -DNOGSTREAMER=ON -DNOJACK=ON -DNOLIBAVDEVICE=ON -DNOLIBUVC=ON -DNOPULSEAUDIO=ON"
    fi
fi

if [ "${ARCH_ROOT_BUILD}" = 1 ]; then
    EXEC='sudo ./root.x86_64/bin/arch-chroot root.x86_64'
elif [ "${TRAVIS_OS_NAME}" = linux ] && [ -z "${ANDROID_BUILD}" ]; then
    EXEC="docker exec ${DOCKERSYS}"
fi

BUILDSCRIPT=dockerbuild.sh

if [ "${ANDROID_BUILD}" = 1 ]; then
    export JAVA_HOME=$(readlink -f /usr/bin/java | sed 's:bin/java::')
    export ANDROID_HOME="${PWD}/build/android-sdk"
    export ANDROID_NDK="${PWD}/build/android-ndk"
    export ANDROID_NDK_HOME=${ANDROID_NDK}
    export ANDROID_NDK_PLATFORM=android-${ANDROID_PLATFORM}
    export ANDROID_NDK_ROOT=${ANDROID_NDK}
    export ANDROID_SDK_ROOT=${ANDROID_HOME}
    export PATH="${JAVA_HOME}/bin/java:${PATH}"
    export PATH="$PATH:${ANDROID_HOME}/tools:${ANDROID_HOME}/tools/bin"
    export PATH="${PATH}:${ANDROID_HOME}/platform-tools"
    export PATH="${PATH}:${ANDROID_HOME}/emulator"
    export PATH="${PATH}:${ANDROID_NDK}"
    export ORIG_PATH="${PATH}"

    for arch_ in $(echo "${TARGET_ARCH}" | tr ":" "\n"); do
        export PATH="${PWD}/build/Qt/${QTVER_ANDROID}/android/bin:${ORIG_PATH}"
        mkdir build-${arch_}
        cd build-${arch_}
        qmake -query
        cmake \
            -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_PREFIX_PATH=${INSTALL_PREFIX} \
            -DCMAKE_C_COMPILER=${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang \
            -DCMAKE_CXX_COMPILER=${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++ \
            -DANDROID_NATIVE_API_LEVEL=${ANDROID_PLATFORM} \
            -DANDROID_NDK=${ANDROID_NDK} \
            -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake \
            -DANDROID_ABI=${arch_} \
            -DANDROID_STL=c++_shared \
            -DCMAKE_FIND_ROOT_PATH=$(qmake -query QT_INSTALL_PREFIX) \
            -DANDROID_SDK=${ANDROID_HOME} \
            -DDAILY_BUILD=${DAILY_BUILD} \
            ${EXTRA_PARAMS} \
            ..
        cmake --build .
        cmake --build . --target install
    done
elif [ "${ARCH_ROOT_BUILD}" = 1 ]; then
    sudo mount --bind root.x86_64 root.x86_64
    sudo mount --bind $HOME root.x86_64/$HOME

    if [ -z "${ARCH_ROOT_MINGW}" ]; then
        QMAKE_CMD=qmake
        CMAKE_CMD=cmake
        PKG_CONFIG=pkg-config
    else
        QMAKE_CMD=/usr/${ARCH_ROOT_MINGW}-w64-mingw32/lib/qt/bin/qmake
        CMAKE_CMD=${ARCH_ROOT_MINGW}-w64-mingw32-cmake
        PKG_CONFIG=${ARCH_ROOT_MINGW}-w64-mingw32-pkg-config
    fi

    cat << EOF > ${BUILDSCRIPT}
#!/bin/sh

export LC_ALL=C
export HOME=$HOME
export PKG_CONFIG=${PKG_CONFIG}

mkdir $TRAVIS_BUILD_DIR/build
cd $TRAVIS_BUILD_DIR/build
${QMAKE_CMD} -query
${CMAKE_CMD} \
    -DQT_QMAKE_EXECUTABLE=${QMAKE_CMD} \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=${INSTALL_PREFIX} \
    -DCMAKE_C_COMPILER="${COMPILER_C}" \
    -DCMAKE_CXX_COMPILER="${COMPILER_CXX}" \
    -DDAILY_BUILD=${DAILY_BUILD} \
    ${EXTRA_PARAMS} \
    ..
${CMAKE_CMD} --build .
${CMAKE_CMD} --build . --target install
EOF
    chmod +x ${BUILDSCRIPT}
    sudo cp -vf ${BUILDSCRIPT} root.x86_64/$HOME/
    ${EXEC} bash $HOME/${BUILDSCRIPT}
    sudo umount root.x86_64/$HOME
    sudo umount root.x86_64
elif [ "${TRAVIS_OS_NAME}" = linux ]; then
    export PATH=$HOME/.local/bin:$PATH

    if [ "${DOCKERSYS}" = debian ]; then
        cat << EOF >> ${BUILDSCRIPT}
#!/bin/sh

mkdir build
cd build
qmake -qt=5 -query
cmake \
    -DQT_QMAKE_EXECUTABLE="qmake -qt=5" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=${INSTALL_PREFIX} \
    -DCMAKE_C_COMPILER="${COMPILER_C}" \
    -DCMAKE_CXX_COMPILER="${COMPILER_CXX}" \
    -DDAILY_BUILD=${DAILY_BUILD} \
    ${EXTRA_PARAMS} \
    ..
cmake --build .
cmake --build . --target install
EOF
    else
        cat << EOF >> ${BUILDSCRIPT}
#!/bin/sh

mkdir build
cd build
qmake-qt5 -query
echo "Current directory: \${PWD}"
echo "INSTALL_PREFIX: ${INSTALL_PREFIX}"

cmake \
    -DQT_QMAKE_EXECUTABLE=qmake-qt5 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=${INSTALL_PREFIX} \
    -DCMAKE_C_COMPILER="${COMPILER_C}" \
    -DCMAKE_CXX_COMPILER="${COMPILER_CXX}" \
    -DDAILY_BUILD=${DAILY_BUILD} \
    ${EXTRA_PARAMS} \
    ..
cmake --build .
cmake --build . --target install
EOF
    fi

    chmod +x ${BUILDSCRIPT}
    ${EXEC} bash ${BUILDSCRIPT}
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    mkdir build
    cd build
    /usr/local/Cellar/qt@5/bin/qmake -query
    cmake \
        -DQT_QMAKE_EXECUTABLE=/usr/local/Cellar/qt@5/bin/qmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH=${INSTALL_PREFIX} \
        -DCMAKE_C_COMPILER="${COMPILER_C}" \
        -DCMAKE_CXX_COMPILER="${COMPILER_CXX}" \
        -DDAILY_BUILD=${DAILY_BUILD} \
        ${EXTRA_PARAMS} \
        ..
    cmake --build .
    cmake --build . --target install
fi
