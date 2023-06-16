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

apt-get -qq -y upgrade
apt-get -qq -y update
apt-get -qq -y install \
    ccache \
    cmake \
    gradle \
    libxkbcommon-x11-0 \
    make \
    ninja-build \
    openjdk-8-jdk \
    openjdk-8-jre \
    python3-pip
update-alternatives --set java /usr/lib/jvm/java-8-openjdk-amd64/jre/bin/java
