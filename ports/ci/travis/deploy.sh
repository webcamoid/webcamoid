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

if [ "${ARCH_ROOT_BUILD}" = 1 ]; then
    EXEC='sudo ./root.x86_64/bin/arch-chroot root.x86_64'
elif [ "${TRAVIS_OS_NAME}" = linux ] && [ -z "${ANDROID_BUILD}" ]; then
    if [ -z "${DAILY_BUILD}" ]; then
        EXEC="docker exec ${DOCKERSYS}"
    else
        EXEC="docker exec -e DAILY_BUILD=1 ${DOCKERSYS}"
    fi
fi

DEPLOYSCRIPT=deployscript.sh

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
    export KEYSTORE_PATH="${PWD}/keystores/debug.keystore"
    nArchs=$(echo "${TARGET_ARCH}" | tr ':' ' ' | wc -w)
    lastArch=$(echo "${TARGET_ARCH}" | awk -F: '{print $NF}')

    if [ "${nArchs}" = 1 ]; then
        export PATH="${PWD}/build/Qt/${QTVER}/android/bin:${PWD}/.local/bin:${ORIG_PATH}"
        export BUILD_PATH=${PWD}/build-webcamoid-${lastArch}

        python3 ports/deploy/deploy.py
    else
        pkgMerge=

        for arch_ in $(echo "${TARGET_ARCH}" | tr ":" "\n"); do
            if [ ! -z "${pkgMerge}" ]; then
                pkgMerge=${pkgMerge}:
            fi

            pkgMerge=${pkgMerge}${PWD}/build-webcamoid-${arch_}
        done

        for arch_ in $(echo "${TARGET_ARCH}" | tr ":" "\n"); do
            export PATH="${PWD}/build/Qt/${QTVER}/android/bin:${PWD}/.local/bin:${ORIG_PATH}"
            export BUILD_PATH=${PWD}/build-webcamoid-${arch_}

            if [ "${arch_}" = "${lastArch}" ]; then
                export PACKAGES_PREPARE_ONLY=0
                export PACKAGES_MERGE="${pkgMerge}"
            else
                export PACKAGES_PREPARE_ONLY=1
            fi

            export NO_SHOW_PKG_DATA_INFO=1

            python3 ports/deploy/deploy.py
        done
    fi

    mkdir -p "${PWD}/ports/deploy/packages_auto"
    cp -rvf "${PWD}/build-webcamoid-${lastArch}/ports/deploy/packages_auto"/* \
            "${PWD}/ports/deploy/packages_auto"
elif [ "${ARCH_ROOT_BUILD}" = 1 ]; then
    sudo mount --bind root.x86_64 root.x86_64
    sudo mount --bind $HOME root.x86_64/$HOME

    cat << EOF > ${DEPLOYSCRIPT}
#!/bin/sh

export LC_ALL=C
export HOME=$HOME
export PATH="$TRAVIS_BUILD_DIR/.local/bin:\$PATH"
export WINEPREFIX=/opt/.wine
cd $TRAVIS_BUILD_DIR
EOF

    if [ ! -z "${DAILY_BUILD}" ]; then
        cat << EOF >> ${DEPLOYSCRIPT}
export DAILY_BUILD=1
EOF
    fi

    cat << EOF >> ${DEPLOYSCRIPT}
python ports/deploy/deploy.py
EOF
    chmod +x ${DEPLOYSCRIPT}
    sudo cp -vf ${DEPLOYSCRIPT} root.x86_64/$HOME/

    ${EXEC} bash $HOME/${DEPLOYSCRIPT}
    sudo umount root.x86_64/$HOME
    sudo umount root.x86_64
elif [ "${TRAVIS_OS_NAME}" = linux ]; then
    cat << EOF > ${DEPLOYSCRIPT}
#!/bin/sh

export PATH="\$PWD/.local/bin:\$PATH"
xvfb-run --auto-servernum python3 ports/deploy/deploy.py
EOF

    chmod +x ${DEPLOYSCRIPT}

    ${EXEC} bash ${DEPLOYSCRIPT}
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    ${EXEC} python3 ports/deploy/deploy.py
fi
