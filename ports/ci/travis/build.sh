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

if [ "${TRAVIS_COMPILER}" = clang ]; then
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
    EXTRA_PARAMS="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_OBJCXX_COMPILER_LAUNCHER=ccache"
fi

if [ ! -z "${DAILY_BUILD}" ] || [ ! -z "${RELEASE_BUILD}" ]; then
    if [ "${TRAVIS_OS_NAME}" = linux ]; then
        EXTRA_PARAMS="${EXTRA_PARAMS} -DNOGSTREAMER=ON -DNOLIBAVDEVICE=ON"
    elif [ "${TRAVIS_OS_NAME}" = osx ]; then
        EXTRA_PARAMS="${EXTRA_PARAMS} -DNOGSTREAMER=ON -DNOJACK=ON -DNOLIBAVDEVICE=ON -DNOLIBUVC=ON -DNOPULSEAUDIO=ON"
    fi
fi

if [ "${ARCH_ROOT_BUILD}" = 1 ]; then
    EXEC='sudo ./root.x86_64/bin/arch-chroot root.x86_64'
elif [ "${TRAVIS_OS_NAME}" = linux ] && [ -z "${ANDROID_BUILD}" ]; then
    EXEC="docker exec ${DOCKERSYS}"
fi

BUILDSCRIPT=dockerbuild.sh

if [ "${DOCKERIMG}" = ubuntu:focal ]; then
    cat << EOF > ${BUILDSCRIPT}
#!/bin/sh
source /opt/qt${PPAQTVER}/bin/qt${PPAQTVER}-env.sh
EOF

    chmod +x ${BUILDSCRIPT}
fi

if [ "${ANDROID_BUILD}" = 1 ]; then
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

    for arch_ in $(echo "${TARGET_ARCH}" | tr ":" "\n"); do
        export PATH="${PWD}/build/Qt/${QTVER_ANDROID}/android/bin:${ORIG_PATH}"
        mkdir build-${arch_}
        cmake \
            -S . \
            -B build-${arch_} \
            -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_C_COMPILER=${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang \
            -DCMAKE_CXX_COMPILER=${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++ \
            -DANDROID_NATIVE_API_LEVEL=${ANDROID_PLATFORM} \
            -DANDROID_NDK=${ANDROID_NDK} \
            -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake \
            -DANDROID_ABI=${arch_} \
            -DANDROID_STL=c++_shared \
            -DCMAKE_FIND_ROOT_PATH=$(qmake -query QT_INSTALL_PREFIX) \
            -DANDROID_SDK=${ANDROID_HOME} \
            ${EXTRA_PARAMS} \
            -DDAILY_BUILD=${DAILY_BUILD}
        cmake -LA -S . -B build-${arch_}
        cmake --build build-${arch_} --parallel ${NJOBS}
    done
elif [ "${ARCH_ROOT_BUILD}" = 1 ]; then
    sudo mount --bind root.x86_64 root.x86_64
    sudo mount --bind $HOME root.x86_64/$HOME

    if [ -z "${ARCH_ROOT_MINGW}" ]; then
        QMAKE_CMD=/usr/bin/qmake
        CMAKE_CMD=/usr/bin/cmake
        PKG_CONFIG=/usr/bin/pkg-config
        LRELEASE_TOOL=/usr/bin/lrelease
        LUPDATE_TOOL=/usr/bin/lupdate
    else
        QMAKE_CMD=/usr/${ARCH_ROOT_MINGW}-w64-mingw32/lib/qt/bin/qmake
        CMAKE_CMD=${ARCH_ROOT_MINGW}-w64-mingw32-cmake
        PKG_CONFIG=${ARCH_ROOT_MINGW}-w64-mingw32-pkg-config
        LRELEASE_TOOL=/usr/${ARCH_ROOT_MINGW}-w64-mingw32/lib/qt/bin/lrelease
        LUPDATE_TOOL=/usr/${ARCH_ROOT_MINGW}-w64-mingw32/lib/qt/bin/lupdate
    fi

    cat << EOF > ${BUILDSCRIPT}
#!/bin/sh

export LC_ALL=C
export HOME=$HOME
export PKG_CONFIG=${PKG_CONFIG}

cd $TRAVIS_BUILD_DIR
mkdir build
${CMAKE_CMD} \
    -S . \
    -B build \
    -DQT_QMAKE_EXECUTABLE=${QMAKE_CMD} \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="\${PWD}/webcamoid-data" \
    -DCMAKE_C_COMPILER="${COMPILER_C}" \
    -DCMAKE_CXX_COMPILER="${COMPILER_CXX}" \
    -DLRELEASE_TOOL=${LRELEASE_TOOL} \
    -DLUPDATE_TOOL=${LUPDATE_TOOL} \
    ${EXTRA_PARAMS} \
    -DDAILY_BUILD=${DAILY_BUILD}
cmake -LA -S . -B build
make -C build -j${NJOBS}
make -C build install
EOF
    chmod +x ${BUILDSCRIPT}
    sudo cp -vf ${BUILDSCRIPT} root.x86_64/$HOME/
    ${EXEC} bash $HOME/${BUILDSCRIPT}
    sudo umount root.x86_64/$HOME
    sudo umount root.x86_64
elif [ "${TRAVIS_OS_NAME}" = linux ]; then
    export PATH=$HOME/.local/bin:$PATH

    if [ "${DOCKERSYS}" = debian ]; then
        if [ "${DOCKERIMG}" = ubuntu:focal ]; then
           if [ -z "${DAILY_BUILD}" ] && [ -z "${RELEASE_BUILD}" ]; then
                cat << EOF >> ${BUILDSCRIPT}
#!/bin/sh

mkdir build
cmake \
    -S . \
    -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="\${PWD}/webcamoid-data" \
    -DCMAKE_C_COMPILER="${COMPILER_C}" \
    -DCMAKE_CXX_COMPILER="${COMPILER_CXX}" \
    ${EXTRA_PARAMS} \
    -DDAILY_BUILD=${DAILY_BUILD}
cmake -LA -S . -B build
cmake --build build --parallel ${NJOBS}
cmake --install build
EOF
            else
                cat << EOF >> ${BUILDSCRIPT}
#!/bin/sh

mkdir build
cmake \
    -S . \
    -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="\${PWD}/webcamoid-data" \
    -DCMAKE_C_COMPILER="${COMPILER_C}" \
    -DCMAKE_CXX_COMPILER="${COMPILER_CXX}" \
    ${EXTRA_PARAMS} \
    -DDAILY_BUILD=${DAILY_BUILD}
    -DNOGSTREAMER=TRUE \
    -DNOLIBAVDEVICE=TRUE
cmake -LA -S . -B build
cmake --build build --parallel ${NJOBS}
cmake --install build
EOF
            fi

            ${EXEC} bash ${BUILDSCRIPT}
        else
            cat << EOF >> ${BUILDSCRIPT}
#!/bin/sh

mkdir build
cmake \
    -S . \
    -B build \
    -DQT_QMAKE_EXECUTABLE="qmake -qt=5" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="\${PWD}/webcamoid-data" \
    -DCMAKE_C_COMPILER="${COMPILER_C}" \
    -DCMAKE_CXX_COMPILER="${COMPILER_CXX}" \
    ${EXTRA_PARAMS} \
    -DDAILY_BUILD=${DAILY_BUILD}
cmake -LA -S . -B build
cmake --build build --parallel ${NJOBS}
cmake --install build
EOF
        fi
    else
        cat << EOF >> ${BUILDSCRIPT}
#!/bin/sh

mkdir build
cmake \
    -S . \
    -B build \
    -DQT_QMAKE_EXECUTABLE=qmake-qt5 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="\${PWD}/webcamoid-data" \
    -DCMAKE_C_COMPILER="${COMPILER_C}" \
    -DCMAKE_CXX_COMPILER="${COMPILER_CXX}" \
    ${EXTRA_PARAMS} \
    -DDAILY_BUILD=${DAILY_BUILD}
cmake -LA -S . -B build
cmake --build build --parallel ${NJOBS}
cmake --install build
EOF
    fi

    chmod +x ${BUILDSCRIPT}
    ${EXEC} bash ${BUILDSCRIPT}
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    export PATH="/usr/local/opt/qt@5/bin:$PATH"
    export LDFLAGS="$LDFLAGS -L/usr/local/opt/qt@5/lib"
    export CPPFLAGS="$CPPFLAGS -I/usr/local/opt/qt@5/include"
    export PKG_CONFIG_PATH="/usr/local/opt/qt@5/lib/pkgconfig:$PKG_CONFIG_PATH"

    mkdir build
    cmake \
        -S . \
        -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=${PWD}/webcamoid-data \
        -DCMAKE_C_COMPILER="${COMPILER_C}" \
        -DCMAKE_CXX_COMPILER="${COMPILER_CXX}" \
        ${EXTRA_PARAMS} \
        -DDAILY_BUILD=${DAILY_BUILD}
    cmake -LA -S . -B build
    cmake --build build --parallel ${NJOBS}
    cmake --install build
fi
