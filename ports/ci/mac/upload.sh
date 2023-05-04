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

branch=$(git rev-parse --abbrev-ref HEAD)

if [[ "${UPLOAD}" != 1 || "${branch}" != master ]]; then
    exit 0
fi

brew install gh

if [[ "$CIRRUS_RELEASE" == "" ]]; then
    releaseName=daily-build
else
    releaseName=$CIRRUS_RELEASE
fi

gh release upload "$releaseName" webcamoid-packages/mac/* --clobber -R "$CIRRUS_REPO_FULL_NAME"
