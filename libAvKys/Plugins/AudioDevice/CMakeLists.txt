# Webcamoid, webcam capture application.
# Copyright (C) 2021  Gonzalo Exequiel Pedone
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

cmake_minimum_required(VERSION 3.16)

project(AudioDevice)

add_subdirectory(src)
add_subdirectory(src/alsa)
add_subdirectory(src/androidaudio)
add_subdirectory(src/jack)
add_subdirectory(src/ndkaudio)
add_subdirectory(src/pipewire)
add_subdirectory(src/portaudio)
add_subdirectory(src/pulseaudio)
add_subdirectory(src/qtaudio)
add_subdirectory(src/sdl)
add_subdirectory(src/wasapi)
