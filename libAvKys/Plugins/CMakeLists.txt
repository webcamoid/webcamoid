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

project(Plugins)

# Base plugins

add_subdirectory(AudioDevice)
add_subdirectory(AudioGen)
add_subdirectory(DesktopCapture)
add_subdirectory(ImageSrc)
add_subdirectory(MultiSink)
add_subdirectory(MultiSrc)
add_subdirectory(VideoCapture)
add_subdirectory(VirtualCamera)

# Video effects

# Base video effects.
# These are required by some of the base plugins to work fine.
add_subdirectory(Flip)
add_subdirectory(SwapRB)

set(NOVIDEOEFFECTS OFF CACHE STRING "No build video effects")

if (NOT NOVIDEOEFFECTS)
    add_subdirectory(AdjustHSL)
    add_subdirectory(Aging)
    add_subdirectory(AnalogTV)
    add_subdirectory(AspectRatio)
    add_subdirectory(Blur)
    add_subdirectory(Cartoon)
    add_subdirectory(ChangeHSL)
    add_subdirectory(Charify)
    add_subdirectory(Cinema)
    add_subdirectory(ColorFilter)
    add_subdirectory(ColorReplace)
    add_subdirectory(ColorTap)
    add_subdirectory(ColorTransform)
    add_subdirectory(Contrast)
    add_subdirectory(Convolve)
    add_subdirectory(Crop)
    add_subdirectory(DelayGrab)
    add_subdirectory(Denoise)
    add_subdirectory(Dice)
    add_subdirectory(Distort)
    add_subdirectory(Dizzy)
    add_subdirectory(Edge)
    add_subdirectory(Emboss)
    add_subdirectory(Equalize)
    add_subdirectory(FaceDetect)
    add_subdirectory(FaceTrack)
    add_subdirectory(FalseColor)
    add_subdirectory(Fire)
    add_subdirectory(FrameOverlap)
    add_subdirectory(Gamma)
    add_subdirectory(GrayScale)
    add_subdirectory(Halftone)
    add_subdirectory(Hypnotic)
    add_subdirectory(Implode)
    add_subdirectory(Invert)
    add_subdirectory(Life)
    add_subdirectory(Matrix)
    add_subdirectory(MatrixTransform)
    add_subdirectory(Nervous)
    add_subdirectory(Normalize)
    add_subdirectory(OilPaint)
    add_subdirectory(Opacity)
    add_subdirectory(Otsu)
    add_subdirectory(Photocopy)
    add_subdirectory(Pixelate)
    add_subdirectory(Quark)
    add_subdirectory(Radioactive)
    add_subdirectory(Ripple)
    add_subdirectory(Rotate)
    add_subdirectory(Saturated)
    add_subdirectory(ScanLines)
    add_subdirectory(Shagadelic)
    add_subdirectory(Swirl)
    add_subdirectory(Temperature)
    add_subdirectory(Vignette)
    add_subdirectory(Warhol)
    add_subdirectory(Warp)
    add_subdirectory(Wave)
    add_subdirectory(Zoom)
endif ()
