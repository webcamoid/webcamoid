#!/bin/bash

# Webcamoid, webcam capture application.
# Copyright (C) 2023  Gonzalo Exequiel Pedone
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

if [ "${UPLOAD}" != 1 ]; then
    exit 0
fi

git config --global --add safe.directory /sources
branch=$(git rev-parse --abbrev-ref HEAD)

if [[ "${DAILY_BUILD}" = 1 && "${branch}" != master ]]; then
    exit 0
fi

apt-get -qq -y install 7zip p7zip-full

if [ -z "${ARCHITECTURE}" ]; then
    architecture="${DOCKERIMG%%/*}"
else
    case "${ARCHITECTURE}" in
        aarch64)
            architecture=arm64v8
            ;;
        armv7)
            architecture=arm32v7
            ;;
        *)
            architecture=${ARCHITECTURE}
            ;;
    esac
fi

7z a -p"${FILE_PASSWORD}" -mx1 -mmt4 "webcamoid-packages-${architecture}.7z" webcamoid-packages/
