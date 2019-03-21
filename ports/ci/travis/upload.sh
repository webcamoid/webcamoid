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

if [ ! -z "$DAILY_BUILD" ] && [ "$APPVEYOR_REPO_BRANCH" == "master" ]; then
    curl -fL https://getcli.jfrog.io | sh

    ./jfrog bt config \
        --user=hipersayanx \
        --key=$BTKEY \
        --licenses=GPLv3+

    path=ports/deploy/packages_auto
    version=daily
    publish=true

    for f in $(find $path -type f); do
        packagePath=${f#$path/}
        folder=$(dirname $packagePath)

        ./jfrog bt upload \
            --user=hipersayanx \
            --key=$BTKEY \
            --override=true \
            --publish=$publish \
            $f \
            webcamoid/webcamoid/webcamoid/$version \
            $folder/
    done
fi
