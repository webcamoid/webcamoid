# Webcamoid, webcam capture application.
# Copyright (C) 2025  Gonzalo Exequiel Pedone
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

project(SimdCore)

add_subdirectory(avx)
add_subdirectory(avx2)
add_subdirectory(mmx)
add_subdirectory(neon)
add_subdirectory(rvv)
add_subdirectory(sse)
add_subdirectory(sse2)
add_subdirectory(sse4_1)
add_subdirectory(sve)
