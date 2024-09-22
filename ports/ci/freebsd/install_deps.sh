#!/bin/sh

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

set -e
set -o errexit

pkg update -f
pkg install -y \
    alsa-lib \
    ccache \
    cmake \
    ffmpeg \
    git \
    gstreamer1 \
    gstreamer1-plugins-good \
    jackit \
    libuvc \
    libXext \
    libXfixes \
    pkgconf \
    patchelf \
    pipewire \
    portaudio \
    pulseaudio \
    python3 \
    qt6-base \
    qt6-declarative \
    qt6-multimedia \
    qt6-svg \
    qt6-tools \
    qt6-wayland \
    sdl2 \
    v4l_compat \
    vlc
