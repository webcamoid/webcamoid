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

project(VideoCapture_ndkcamera LANGUAGES CXX)

include(../../../../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Concurrent
    Core)

if (ANDROID)
    list(APPEND QT_COMPONENTS AndroidExtras)
endif ()

find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_library(ANDROIDLIB NAMES android)
find_library(CAMERA2NDK NAMES camera2ndk)
find_library(MEDIANDK NAMES mediandk)

set(SOURCES
    ../../capture.cpp
    ../../capture.h
    src/capturendkcamera.cpp
    src/capturendkcamera.h
    src/plugin.cpp
    src/plugin.h
    pspec.json)

if (NOT NONDKCAMERA AND ANDROIDLIB AND CAMERA2NDK AND MEDIANDK)
    add_library(VideoCapture_ndkcamera SHARED ${SOURCES})
else ()
    add_library(VideoCapture_ndkcamera SHARED EXCLUDE_FROM_ALL ${SOURCES})
endif ()

add_dependencies(VideoCapture_ndkcamera avkys)
target_include_directories(VideoCapture_ndkcamera
                           PRIVATE
                           ../..
                           ../../../../../Lib/src)
target_compile_definitions(VideoCapture_ndkcamera PRIVATE AVKYS_PLUGIN_VIDEOCAPTURE_NDKCAMERA)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(VideoCapture_ndkcamera
                      ${QT_LIBS}
                      avkys)

if (NOT NONDKCAMERA AND ANDROIDLIB AND CAMERA2NDK AND MEDIANDK)
    target_link_libraries(VideoCapture_ndkcamera
                          ${ANDROIDLIB}
                          ${CAMERA2NDK}
                          ${MEDIANDK})
endif ()
