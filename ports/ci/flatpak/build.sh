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

appId=org.webcamoid.Webcamoid
manifestFile=${appId}.yml

cat << EOF > "${manifestFile}"
app-id: ${appId}
runtime: org.kde.Platform
runtime-version: '${RUNTIME_VERSION}'
sdk: org.kde.Sdk
command: webcamoid
finish-args:
  - --share=ipc
  - --socket=x11
  - --socket=wayland
  - --filesystem=host
  - --device=dri
modules:
  - name: webcamoid
    buildsystem: cmake-ninja
    config-opts:
      - -LA
      - -DCMAKE_BUILD_TYPE=Release
    sources:
      - type: dir
        path: .
EOF

flatpak-builder --user --install webcamoid-build --force-clean "${manifestFile}"
