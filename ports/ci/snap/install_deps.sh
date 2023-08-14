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

if [ "${ARM_BUILD}" != 1 ]; then
    SUDO_CMD=sudo
fi

${SUDO_CMD} apt-get -qq -y update
${SUDO_CMD} apt-get -qq -y upgrade
${SUDO_CMD} snap install lxd
${SUDO_CMD} snap install snapcraft --classic
${SUDO_CMD} lxd init --auto
${SUDO_CMD} usermod -aG lxd  "$USER"
