# Webcamoid, webcam capture application.
# Copyright (C) 2019  Gonzalo Exequiel Pedone
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

CONFIG += console c++11

android {
    TARGET_ARCH = $$ANDROID_TARGET_ARCH
} else: msvc {
    TARGET_ARCH = $${QMAKE_TARGET.arch}
    TARGET_ARCH = $$basename(TARGET_ARCH)
    TARGET_ARCH = $$replace(TARGET_ARCH, x64, x86_64)
} else {
    TARGET_ARCH = $$system($${QMAKE_CXX} -dumpmachine)
    TARGET_ARCH = $$split(TARGET_ARCH, -)
    TARGET_ARCH = $$first(TARGET_ARCH)
}

COMPILER = $$basename(QMAKE_CXX)
COMPILER = $$replace(COMPILER, \+\+, pp)
COMPILER = $$join(COMPILER, _)

CONFIG(debug, debug|release) {
    COMMONS_BUILD_PATH = debug/Qt$${QT_VERSION}/$${COMPILER}/$${TARGET_ARCH}
} else {
    COMMONS_BUILD_PATH = release/Qt$${QT_VERSION}/$$COMPILER/$${TARGET_ARCH}
}

android: COMMONS_BUILD_PATH = $${COMMONS_BUILD_PATH}/$${ANDROID_PLATFORM}

BIN_DIR = $${COMMONS_BUILD_PATH}/bin
