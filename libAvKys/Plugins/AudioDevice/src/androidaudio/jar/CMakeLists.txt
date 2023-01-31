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

cmake_minimum_required(VERSION 3.16)

project(AudioDevice_androidaudiojar)

include(../../../../../cmake/ProjectCommons.cmake)

set(SOURCES_PREFIX src/org/webcamoid/plugins/AudioDevice/submodules/androidaudio)
set(SOURCES ${SOURCES_PREFIX}/AkAndroidAudioCallbacks.java)

if (ANDROID)
    include(UseJava)
    find_package(Java ${ANDROID_JAVA_VERSION})

    if (Java_FOUND)
        set(CMAKE_JAVA_COMPILE_FLAGS "-source" "${ANDROID_JAVA_VERSION}" "-target" "${ANDROID_JAVA_VERSION}")
        add_jar(AudioDevice_androidaudiojar
                SOURCES ${SOURCES}
                INCLUDE_JARS
                ${ANDROID_JAR_DIRECTORY}/android.jar
                OUTPUT_NAME AkAudioDevice_androidaudio
                OUTPUT_DIR ${CMAKE_BINARY_DIR}/android-build/libs)
    endif ()
endif ()

add_custom_target(AudioDevice_androidaudiojar_sources SOURCES ${SOURCES})
