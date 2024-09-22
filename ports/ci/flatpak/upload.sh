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
set -o errexit

if [ "${UPLOAD}" != 1 ]; then
    exit 0
fi

branch=$(git rev-parse --abbrev-ref HEAD)

if [[ "${DAILY_BUILD}" = 1 && "${branch}" != master ]]; then
    exit 0
fi

apt-get -qq -y install gh

if [ "${DAILY_BUILD}" = 1 ]; then
    releaseName=daily-build
else
    verMaj=$(grep VER_MAJ libAvKys/cmake/ProjectCommons.cmake | awk '{print $2}' | tr -d ')' | head -n 1)
    verMin=$(grep VER_MIN libAvKys/cmake/ProjectCommons.cmake | awk '{print $2}' | tr -d ')' | head -n 1)
    verPat=$(grep VER_PAT libAvKys/cmake/ProjectCommons.cmake | awk '{print $2}' | tr -d ')' | head -n 1)
    releaseName=${verMaj}.${verMin}.${verPat}
fi

gh release upload "$releaseName" webcamoid-packages/linux/* --clobber -R "webcamoid/WebcamoidPrivate"
