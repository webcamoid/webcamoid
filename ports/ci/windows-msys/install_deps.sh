#!/bin/bash

# akvirtualcamera, virtual camera for Mac and Windows.
# Copyright (C) 2025  Gonzalo Exequiel Pedone
#
# akvirtualcamera is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# akvirtualcamera is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with akvirtualcamera. If not, see <http://www.gnu.org/licenses/>.
#
# Web-Site: http://webcamoid.github.io/

if [ ! -z "${USE_WGET}" ]; then
    export DOWNLOAD_CMD="wget -nv -c"
else
    export DOWNLOAD_CMD="curl --retry 10 -sS -kLOC -"
fi

# Install NSIS

if [ -z "${NSIS_VERSION}" ]; then
    NSIS_VERSION=3.11
fi

nsis=nsis-${NSIS_VERSION}.zip
url="https://sourceforge.net/projects/nsis/files/NSIS%20${NSIS_VERSION:0:1}/${NSIS_VERSION}/${nsis}"

echo "Downloading ${url}"
${DOWNLOAD_CMD} "${url}"

if [ -e ${nsis} ]; then
    echo "installing ${nsis}"

    NSIS_INSTALL_DIR="/c/Program Files (x86)"
    mkdir -p "${NSIS_INSTALL_DIR}"
    unzip -q "${nsis}" -d "${NSIS_INSTALL_DIR}"
    mv -f "${NSIS_INSTALL_DIR}/nsis-${NSIS_VERSION}" "${NSIS_INSTALL_DIR}/NSIS"
fi
