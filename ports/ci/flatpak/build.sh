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

appId=io.github.webcamoid.Webcamoid
manifestFile=${appId}.yml

if [ "${GITHUB_SHA}" != "" ]; then
    branch=${GITHUB_REF##*/}
    commitSha=${GITHUB_SHA}
else
    branch=${CIRRUS_BASE_BRANCH}
    commitSha=${CIRRUS_BASE_SHA}
fi

if [ "${branch}" = "" ]; then
    branch=$(git rev-parse --abbrev-ref HEAD)
fi

if [ "${commitSha}" = "" ]; then
    commitSha=$(git rev-parse "origin/${branch}")
fi

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
    sources:
      - type: git
        url: https://github.com/webcamoid/webcamoid.git
        branch: ${branch}
        commit: ${commitSha}
EOF

if [ "${ARM_BUILD}" = 1 ]; then
    EXTRA_PARAMS="--disable-rofiles-fuse"
fi

flatpak-builder \
  --user \
  ${EXTRA_PARAMS} \
  --install webcamoid-build \
  --force-clean "${manifestFile}"
