# Webcamoid, webcam capture application.
# Copyright (C) 2016  Gonzalo Exequiel Pedone
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

TEMPLATE = subdirs

CONFIG(debug, debug|release): CONFIG += ordered

# Base plugins
SUBDIRS += \
    ACapsConvert \
    AudioDevice \
    AudioGen \
    DesktopCapture \
    Multiplex \
    MultiSink \
    MultiSrc \
    VideoCapture

isEmpty(NOVCAM): {
    CONFIG(config_cmio) \
    | CONFIG(config_dshow) \
    | CONFIG(config_v4l2): SUBDIRS += \
        VirtualCamera
}

# Video effects
isEmpty(NOVIDEOEFFECTS): SUBDIRS += \
    Aging \
    Blur \
    Cartoon \
    ChangeHSL \
    Charify \
    Cinema \
    ColorFilter \
    ColorReplace \
    ColorTap \
    ColorTransform \
    Convolve \
    DelayGrab \
    Denoise \
    Dice \
    Distort \
    Dizzy \
    Edge \
    Emboss \
    Equalize \
    FaceDetect \
    FaceTrack \
    FalseColor \
    Fire \
    FrameOverlap \
    GrayScale \
    Halftone \
    Hypnotic \
    Implode \
    Invert \
    Life \
    Matrix \
    MatrixTransform \
    Nervous \
    Normalize \
    OilPaint \
    Photocopy \
    Pixelate \
    PrimariesColors \
    Quark \
    Radioactive \
    Ripple \
    ScanLines \
    Scroll \
    Shagadelic \
    Swirl \
    Temperature \
    Vignette \
    Warhol \
    Warp \
    Wave
