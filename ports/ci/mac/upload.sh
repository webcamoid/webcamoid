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

if [[ "${UPLOAD}" != 1 ]]; then
    exit 0
fi

if [[ "$CIRRUS_RELEASE" == "" ]]; then
    releaseName=daily-build
else
    releaseName=$CIRRUS_RELEASE
fi

file_content_type="application/octet-stream"
files_to_upload=$(find webcamoid-packages/mac -type f)

for fpath in $files_to_upload
do
    echo "Uploading $fpath..."
    name=$(basename "$fpath")
    url_to_upload="https://uploads.github.com/repos/$CIRRUS_REPO_FULL_NAME/releases/$releaseName/assets?name=$name"
    echo "To $url_to_upload"
    curl -X POST \
        --data-binary @$fpath \
        --header "Authorization: token $GITHUB_TOKEN" \
        --header "Content-Type: $file_content_type" \
        $url_to_upload
done
