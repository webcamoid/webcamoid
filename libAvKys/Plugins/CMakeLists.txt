# Webcamoid, camera capture application.
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

project(Plugins)

include(../cmake/ProjectCommons.cmake)

# Base plugins

add_subdirectory(SimdCore)
add_subdirectory(AudioDevice)
add_subdirectory(Codecs)
add_subdirectory(DesktopCapture)
add_subdirectory(FillAudioGaps)
add_subdirectory(ImageSrc)
add_subdirectory(MultiSrc)
add_subdirectory(PacketSync)
add_subdirectory(VideoCapture)
add_subdirectory(VirtualCamera)

# Video effects

# Base video effects.
# These are required by some of the base plugins to work fine.
add_subdirectory(AdjustHSL)
add_subdirectory(Contrast)
add_subdirectory(Flip)
add_subdirectory(FpsControl)
add_subdirectory(Gamma)
add_subdirectory(Rotate)
add_subdirectory(SwapRB)

if (NOT NOVIDEOEFFECTS)
    add_subdirectory(AspectRatio)
    add_subdirectory(ChangeHSL)
    add_subdirectory(ColorFilter)
    add_subdirectory(ColorReplace)
    add_subdirectory(ColorTap)
    add_subdirectory(DelayGrab)
    add_subdirectory(Denoise)
    add_subdirectory(Distort)
    add_subdirectory(FalseColor)
    add_subdirectory(FrameOverlap)
    add_subdirectory(Halftone)
    add_subdirectory(Life)
    add_subdirectory(Nervous)
    add_subdirectory(OilPaint)
    add_subdirectory(Opacity)
    add_subdirectory(Quark)
    add_subdirectory(Ripple)
endif ()
