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

if [ "${COMPILER}" = clang ]; then
    COMPILER_C=clang
    COMPILER_CXX=clang++
else
    COMPILER_C=gcc
    COMPILER_CXX=g++
fi

COMPILER_C=${TARGET_ARCH}-w64-mingw32-${COMPILER_C}
COMPILER_CXX=${TARGET_ARCH}-w64-mingw32-${COMPILER_CXX}

if [ -z "${DISABLE_CCACHE}" ]; then
    EXTRA_PARAMS="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_OBJCXX_COMPILER_LAUNCHER=ccache"
fi

EXEC='sudo ./root.x86_64/bin/arch-chroot root.x86_64'
BUILDSCRIPT=dockerbuild.sh

sudo mount --bind root.x86_64 root.x86_64
sudo mount --bind $HOME root.x86_64/$HOME

QMAKE_CMD=/usr/${TARGET_ARCH}-w64-mingw32/lib/qt/bin/qmake
CMAKE_CMD=${TARGET_ARCH}-w64-mingw32-cmake
PKG_CONFIG=${TARGET_ARCH}-w64-mingw32-pkg-config
LRELEASE_TOOL=/usr/${TARGET_ARCH}-w64-mingw32/lib/qt/bin/lrelease
LUPDATE_TOOL=/usr/${TARGET_ARCH}-w64-mingw32/lib/qt/bin/lupdate

cat << EOF > ${BUILDSCRIPT}
#!/bin/sh

export LC_ALL=C
export HOME=$HOME
export PKG_CONFIG=${PKG_CONFIG}

cd $PWD
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
