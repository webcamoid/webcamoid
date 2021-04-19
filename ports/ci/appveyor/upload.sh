#!/bin/bash

# Webcamoid, webcam capture application.
# Copyright (C) 2021  Gonzalo Exequiel Pedone
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

[ -f environment.sh ] && source environment.sh

if [ ! -z "${USE_WGET}" ]; then
    export DOWNLOAD_CMD="wget -nv -c"
else
    export DOWNLOAD_CMD="curl --retry 10 -sS -kLOC -"
fi

if [[ ! -z "$DAILY_BUILD" || ! -z "$RELEASE_BUILD" ]]; then
    if [ -z "$DAILY_BUILD" ]; then
        VER_MAJ=$(grep -re '^VER_MAJ[[:space:]]*=[[:space:]]*' commons.pri | awk '{print $3}')
        VER_MIN=$(grep -re '^VER_MIN[[:space:]]*=[[:space:]]*' commons.pri | awk '{print $3}')
        VER_PAT=$(grep -re '^VER_PAT[[:space:]]*=[[:space:]]*' commons.pri | awk '{print $3}')
        version=$VER_MAJ.$VER_MIN.$VER_PAT-$APPVEYOR_REPO_BRANCH
        publish=false
    else
        version=daily-$APPVEYOR_REPO_BRANCH
        publish=true
    fi

    # Upload to Bintray
    choco install -y jfrog-cli

    jfrog bt config \
        --user=hipersayanx \
        --key=$BT_KEY \
        --licenses=GPL-3.0-or-later

    path=webcamoid-packages

    for f in $(find $path -type f); do
        packagePath=${f#$path/}
        folder=$(dirname $packagePath)

        jfrog bt upload \
            --user=hipersayanx \
            --key=$BT_KEY \
            --override=true \
            --publish=$publish \
            $f \
            webcamoid/webcamoid/webcamoid/$version \
            $folder/
    done
fi
