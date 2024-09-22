#!/bin/bash

# Webcamoid, webcam capture application.
# Copyright (C) 2022  Gonzalo Exequiel Pedone
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

if [ ! -z "${GITHUB_SHA}" ]; then
    export GIT_COMMIT_HASH="${GITHUB_SHA}"
elif [ ! -z "${CIRRUS_CHANGE_IN_REPO}" ]; then
    export GIT_COMMIT_HASH="${CIRRUS_CHANGE_IN_REPO}"
fi

export GIT_BRANCH_NAME=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)

if [ -z "${GIT_BRANCH_NAME}" ]; then
    if [ ! -z "${GITHUB_REF_NAME}" ]; then
        export GIT_BRANCH_NAME="${GITHUB_REF_NAME}"
    elif [ ! -z "${CIRRUS_BRANCH}" ]; then
        export GIT_BRANCH_NAME="${CIRRUS_BRANCH}"
    else
        export GIT_BRANCH_NAME=master
    fi
fi

appId=io.github.webcamoid.Webcamoid
manifestFile=${appId}.yml

cat << EOF > "${manifestFile}"
app-id: ${appId}
runtime: org.kde.Platform
runtime-version: '${RUNTIME_VERSION}'
sdk: org.kde.Sdk
command: webcamoid
rename-icon: webcamoid
rename-appdata-file: ${appId}.metainfo.xml
rename-desktop-file: webcamoid.desktop
finish-args:
  - --share=ipc
  - --share=network
  - --socket=fallback-x11
  - --socket=wayland
  - --socket=pulseaudio
  - --filesystem=xdg-pictures
  - --filesystem=xdg-videos
  - --device=all
  - --talk-name=org.freedesktop.Flatpak
modules:
  - name: webcamoid
    buildsystem: cmake-ninja
    config-opts:
      - -LA
      - -DCMAKE_BUILD_TYPE=Release
      - -DDAILY_BUILD="${DAILY_BUILD}"
      - -DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
    sources:
      - type: git
        url: https://github.com/webcamoid/webcamoid.git
        branch: ${GIT_BRANCH_NAME}
        commit: ${GIT_COMMIT_HASH}
EOF

if [ "${ARM_BUILD}" = 1 ]; then
    EXTRA_PARAMS="--disable-rofiles-fuse"
fi

flatpak-builder \
  --user \
  ${EXTRA_PARAMS} \
  --install webcamoid-build \
  --force-clean "${manifestFile}"
