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

if [ "${ARM_BUILD}" != 1 ]; then
    SUDO_CMD=sudo
fi

${SUDO_CMD} apt-get -y install software-properties-common
${SUDO_CMD} add-apt-repository ppa:flatpak/stable
${SUDO_CMD} apt-get -qq -y update
${SUDO_CMD} apt-get -qq -y upgrade
${SUDO_CMD} apt-get -y install \
    flatpak \
    flatpak-builder

flatpak --user remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak --user -y install \
    "org.kde.Platform//${RUNTIME_VERSION}" \
    "org.kde.Sdk//${RUNTIME_VERSION}"

if [ "${ARM_BUILD}" = 1 ]; then
    git config --global --add safe.directory /sources
fi
