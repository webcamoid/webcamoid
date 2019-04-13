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
    if [ "${CXX}" = clang++ ]; then
        UNUSEDARGS="-Qunused-arguments"
    fi

    COMPILER="ccache ${CXX} ${UNUSEDARGS}"
else
    COMPILER=${CXX}
fi

if [ "${ARCH_ROOT_BUILD}" = 1 ]; then
    EXEC='sudo ./root.x86_64/bin/arch-chroot root.x86_64'
elif [ "${TRAVIS_OS_NAME}" = linux ] && [ -z "${ANDROID_BUILD}" ]; then
    if [ -z "${DAILY_BUILD}" ]; then
        EXEC="docker exec ${DOCKERSYS}"
    else
        EXEC="docker exec -e DAILY_BUILD=1 ${DOCKERSYS}"
    fi
fi

BUILDSCRIPT=dockerbuild.sh

if [ "${DOCKERIMG}" = ubuntu:xenial ]; then
    cat << EOF > ${BUILDSCRIPT}
#!/bin/sh

source /opt/qt${PPAQTVER}/bin/qt${PPAQTVER}-env.sh
EOF

    chmod +x ${BUILDSCRIPT}
fi

if [ "${ANDROID_BUILD}" = 1 ]; then
    export PATH=$PWD/build/Qt/${QTVER}/android_${TARGET_ARCH}/bin:$PATH
    env | grep ANDROID_
#     export ANDROID_HOME=/opt/android-sdk
#     export ANDROID_NDK=/opt/android-ndk
#     export ANDROID_NDK_HOME=/opt/android-ndk
#     export ANDROID_NDK_PLATFORM=android-22
     export ANDROID_NDK_ROOT=$PWD/build/android-ndk-${NDKVER}
#     export ANDROID_SDK_ROOT=/opt/android-sdk
    qmake -query
    qmake -spec ${COMPILESPEC} Webcamoid.pro \
        CONFIG+=silent
elif [ "${ARCH_ROOT_BUILD}" = 1 ]; then
    sudo mount --bind root.x86_64 root.x86_64
    sudo mount --bind $HOME root.x86_64/$HOME

    if [ -z "${ARCH_ROOT_MINGW}" ]; then
        QMAKE_CMD=qmake
    else
        QMAKE_CMD=/usr/${ARCH_ROOT_MINGW}-w64-mingw32/lib/qt/bin/qmake
    fi

    cat << EOF > ${BUILDSCRIPT}
#!/bin/sh

export LC_ALL=C
export HOME=$HOME
EOF

    if [ ! -z "${DAILY_BUILD}" ]; then
        cat << EOF >> ${BUILDSCRIPT}
export DAILY_BUILD=1
EOF
    fi

    cat << EOF >> ${BUILDSCRIPT}
cd $TRAVIS_BUILD_DIR
${QMAKE_CMD} -query
${QMAKE_CMD} -spec ${COMPILESPEC} Webcamoid.pro \
    CONFIG+=silent \
    QMAKE_CXX="${COMPILER}"
EOF
    chmod +x ${BUILDSCRIPT}
    sudo cp -vf ${BUILDSCRIPT} root.x86_64/$HOME/

    ${EXEC} bash $HOME/${BUILDSCRIPT}
elif [ "${TRAVIS_OS_NAME}" = linux ]; then
    export PATH=$HOME/.local/bin:$PATH

    if [ "${DOCKERSYS}" = debian ]; then
        if [ "${DOCKERIMG}" = ubuntu:xenial ]; then
            if [ -z "${DAILY_BUILD}" ] && [ -z "${RELEASE_BUILD}" ]; then
                cat << EOF >> ${BUILDSCRIPT}
#!/bin/sh

qmake -query
qmake -spec ${COMPILESPEC} Webcamoid.pro \
    CONFIG+=silent \
    QMAKE_CXX="${COMPILER}"
EOF
            else
                cat << EOF >> ${BUILDSCRIPT}
#!/bin/sh

qmake -query
qmake -spec ${COMPILESPEC} Webcamoid.pro \
    CONFIG+=silent \
    QMAKE_CXX="${COMPILER}" \
    NOGSTREAMER=1 \
    NOQTAUDIO=1
EOF
            fi

            ${EXEC} bash ${BUILDSCRIPT}
        else
            ${EXEC} qmake -qt=5 -query
            ${EXEC} qmake -qt=5 -spec ${COMPILESPEC} Webcamoid.pro \
                CONFIG+=silent \
                QMAKE_CXX="${COMPILER}"
        fi
    else
        ${EXEC} qmake-qt5 -query
        ${EXEC} qmake-qt5 -spec ${COMPILESPEC} Webcamoid.pro \
            CONFIG+=silent \
            QMAKE_CXX="${COMPILER}"
    fi
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    ${EXEC} qmake -query

    if [ -z "${DAILY_BUILD}" ] && [ -z "${RELEASE_BUILD}" ]; then
        ${EXEC} qmake -spec ${COMPILESPEC} Webcamoid.pro \
            CONFIG+=silent \
            QMAKE_CXX="${COMPILER}"
    else
        ${EXEC} qmake -spec ${COMPILESPEC} Webcamoid.pro \
            CONFIG+=silent \
            QMAKE_CXX="${COMPILER}" \
            NOGSTREAMER=1 \
            NOJACK=1 \
            NOLIBUVC=1 \
            NOPULSEAUDIO=1 \
            NOQTAUDIO=1
    fi
fi

if [ -z "${NJOBS}" ]; then
    NJOBS=4
fi

if [ "${ARCH_ROOT_BUILD}" = 1 ]; then
    cat << EOF > ${BUILDSCRIPT}
#!/bin/sh

export LC_ALL=C
export HOME=$HOME
EOF

    if [ ! -z "${DAILY_BUILD}" ]; then
        cat << EOF >> ${BUILDSCRIPT}
export DAILY_BUILD=1
EOF
    fi

    cat << EOF >> ${BUILDSCRIPT}
cd $TRAVIS_BUILD_DIR
make -j${NJOBS}
EOF
    chmod +x ${BUILDSCRIPT}
    sudo cp -vf ${BUILDSCRIPT} root.x86_64/$HOME/

    ${EXEC} bash $HOME/${BUILDSCRIPT}
    sudo umount root.x86_64/$HOME
    sudo umount root.x86_64
else
    ${EXEC} make -j${NJOBS}
fi

if [ "${ARCH_ROOT_BUILD}" = 1 ] && [ ! -z "${ARCH_ROOT_MINGW}" ]; then
    if [ "$ARCH_ROOT_MINGW" = x86_64 ]; then
        mingw_arch=i686
        mingw_compiler=${COMPILER/x86_64/i686}
        mingw_dstdir=x86
    else
        mingw_arch=x86_64
        mingw_compiler=${COMPILER/i686/x86_64}
        mingw_dstdir=x64
    fi

    echo
    echo "Building $mingw_arch virtual camera driver"
    echo
    sudo mount --bind root.x86_64 root.x86_64
    sudo mount --bind $HOME root.x86_64/$HOME

    cat << EOF > ${BUILDSCRIPT}
#!/bin/sh

export LC_ALL=C
export HOME=$HOME
mkdir -p $TRAVIS_BUILD_DIR/akvcam
cd $TRAVIS_BUILD_DIR/akvcam
/usr/${mingw_arch}-w64-mingw32/lib/qt/bin/qmake \
    -spec ${COMPILESPEC} \
    ../libAvKys/Plugins/VirtualCamera/VirtualCamera.pro \
    CONFIG+=silent \
    QMAKE_CXX="${mingw_compiler}" \
    VIRTUALCAMERAONLY=1
make -j${NJOBS}
EOF
    chmod +x ${BUILDSCRIPT}
    sudo cp -vf ${BUILDSCRIPT} root.x86_64/$HOME/

    ${EXEC} bash $HOME/${BUILDSCRIPT}

    sudo mkdir -p libAvKys/Plugins/VirtualCamera/src/dshow/VirtualCamera/AkVirtualCamera.plugin/${mingw_dstdir}
    sudo cp -rvf \
        akvcam/src/dshow/VirtualCamera/AkVirtualCamera.plugin/${mingw_dstdir}/* \
        libAvKys/Plugins/VirtualCamera/src/dshow/VirtualCamera/AkVirtualCamera.plugin/${mingw_dstdir}/

    sudo umount root.x86_64/$HOME
    sudo umount root.x86_64
fi
