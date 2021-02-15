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

if [[ ! -z "$DAILY_BUILD" || ! -z "$RELEASE_BUILD" ]]; then

    path=ports/deploy/packages_auto

    for f in $(find $path -type f); do
        packagePath=${f#$path/}
        folder=$(dirname $packagePath)
        appveyor PushArtifact $f
    done
fi
