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
    export ANDROID_NDK_ROOT=$PWD/build/android-ndk-${NDKVER}
    qmake -spec ${COMPILESPEC} Webcamoid.pro \
        CONFIG+=silent
elif [ "${ARCH_ROOT_BUILD}" = 1 ]; then
    sudo mount --bind root.x86_64 root.x86_64
    sudo mount --bind ${PWD} root.x86_64/home/user/webcamoid

    cat << EOF > ${BUILDSCRIPT}
#!/bin/sh

cd /home/user/webcamoid
qmake -spec ${COMPILESPEC} Webcamoid.pro \
    CONFIG+=silent \
    QMAKE_CXX="${COMPILER}"
EOF
    chmod +x ${BUILDSCRIPT}
    sudo cp -vf ${BUILDSCRIPT} root.x86_64/home/user/

    ${EXEC} bash /home/user/${BUILDSCRIPT}
elif [ "${TRAVIS_OS_NAME}" = linux ]; then
    export PATH=$HOME/.local/bin:$PATH

    if [ "${DOCKERSYS}" = debian ]; then
        if [ "${DOCKERIMG}" = ubuntu:xenial ]; then
            if [ -z "${DAILY_BUILD}" ]; then
                cat << EOF >> ${BUILDSCRIPT}
#!/bin/sh

qmake -spec ${COMPILESPEC} Webcamoid.pro \
    CONFIG+=silent \
    QMAKE_CXX="${COMPILER}"
EOF
            else
                cat << EOF >> ${BUILDSCRIPT}
#!/bin/sh

qmake -spec ${COMPILESPEC} Webcamoid.pro \
    CONFIG+=silent \
    QMAKE_CXX="${COMPILER}" \
    NOGSTREAMER=1 \
    NOQTAUDIO=1
EOF
            fi

            ${EXEC} bash ${BUILDSCRIPT}
        else
            ${EXEC} qmake -qt=5 -spec ${COMPILESPEC} Webcamoid.pro \
                CONFIG+=silent \
                QMAKE_CXX="${COMPILER}"
        fi
    else
        ${EXEC} qmake-qt5 -spec ${COMPILESPEC} Webcamoid.pro \
            CONFIG+=silent \
            QMAKE_CXX="${COMPILER}"
    fi
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    if [ -z "${DAILY_BUILD}" ]; then
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

cd /home/user/webcamoid
make -j${NJOBS}
EOF
    chmod +x ${BUILDSCRIPT}
    sudo cp -vf ${BUILDSCRIPT} root.x86_64/home/user/

    ${EXEC} bash /home/user/${BUILDSCRIPT}
    sudo umount root.x86_64/home/user/webcamoid
    sudo umount root.x86_64
else
    ${EXEC} make -j${NJOBS}
fi
