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
    pkgconf \
    patchelf \
    pipewire \
    portaudio \
    pulseaudio \
    python3 \
    qt5-buildtools \
    qt5-concurrent \
    qt5-linguisttools \
    qt5-multimedia \
    qt5-opengl \
    qt5-qmake \
    qt5-quickcontrols \
    qt5-quickcontrols2 \
    qt5-svg \
    qt5-wayland \
    qt5-xml \
    sdl2 \
    v4l_compat \
    vlc
