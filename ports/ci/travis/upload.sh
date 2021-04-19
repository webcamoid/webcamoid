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

if [ ! -z "${USE_WGET}" ]; then
    export DOWNLOAD_CMD="wget -nv -c"
else
    export DOWNLOAD_CMD="curl --retry 10 -sS -kLOC -"
fi

if [[ ! -z "$DAILY_BUILD" || ! -z "$RELEASE_BUILD" ]]; then
    if [ -z "$DAILY_BUILD" ]; then
        version=$(grep -re '^version[[:space:]]*=[[:space:]]*' build/package_info.conf | awk -F=  '{print $2}' | tr -d ' ')
        publish=false
    else
        version=daily-$TRAVIS_BRANCH
        publish=true
    fi

    # Upload to Bintray

    curl -fL https://getcli.jfrog.io | sh

    ./jfrog bt config \
        --user=hipersayanx \
        --key=$BT_KEY \
        --licenses=GPL-3.0-or-later

    path=webcamoid-packages

    for f in $(find $path -type f); do
        packagePath=${f#$path/}
        folder=$(dirname $packagePath)

        ./jfrog bt upload \
            --user=hipersayanx \
            --key=$BT_KEY \
            --override=true \
            --publish=$publish \
            $f \
            webcamoid/webcamoid/webcamoid/$version \
            $folder/
    done

    # Upload to Github Releases
    upload=false

    if [[ ! -z "$DAILY_BUILD" && "$upload" == true ]]; then
        hub=''

        if [ "${TRAVIS_OS_NAME}" = linux ]; then
            hub=hub-linux-amd64-${GITHUB_HUBVER}
        else
            hub=hub-darwin-amd64-${GITHUB_HUBVER}
        fi

        cd ${TRAVIS_BUILD_DIR}
        ${DOWNLOAD_CMD} https://github.com/github/hub/releases/download/v${GITHUB_HUBVER}/${hub}.tgz || true
        tar xzf ${hub}.tgz
        mkdir -p .local
        cp -rf ${hub}/* .local/

        export PATH="${PWD}/.local/bin:${PATH}"

        hubTag=$(hub release -df '%T %t%n' | grep 'Daily Build' | awk '{print $1}' | sed 's/.*://')

        if [ -z "$hubTag" ]; then
            hub release create -p -m 'Daily Build' daily
            hubTag=$(hub release -df '%T %t%n' | grep 'Daily Build' | awk '{print $1}' | sed 's/.*://')
        fi

        if [ ! -z "$hubTag" ]; then
            path=webcamoid-packages

            for f in $(find $path -type f); do
                hubTag=$(hub release -df '%T %t%n' | grep 'Daily Build' | awk '{print $1}' | sed 's/.*://')
                hub release edit -m 'Daily Build' -a "$f" "$hubTag"
            done
        fi
    fi
fi
