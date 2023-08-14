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

project(VideoCapture_v4l2sys LANGUAGES CXX)

include(CheckCXXSourceCompiles)
include(../../../../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Concurrent
    Core)
find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)

check_cxx_source_compiles("
#ifndef __FreeBSD__
    #error Not BSD
#endif

int main()
{
    return 0;
}" IS_BSD)

if (IS_BSD)
    set(CMAKE_REQUIRED_INCLUDES /usr/local/include)
endif ()

check_cxx_source_compiles("
#include <linux/videodev2.h>

int main()
{
    return 0;
}" HAVE_V4L2)

set(SOURCES
    ../../capture.cpp
    ../../capture.h
    src/capturev4l2.cpp
    src/capturev4l2.h
    src/plugin.cpp
    src/plugin.h
    pspec.json)

if (NOT NOV4L2 AND HAVE_V4L2 AND NOT ANDROID)
    add_library(VideoCapture_v4l2sys SHARED ${SOURCES})
else ()
    add_library(VideoCapture_v4l2sys SHARED EXCLUDE_FROM_ALL ${SOURCES})
endif ()

if (NOT ANDROID)
    set_target_properties(VideoCapture_v4l2sys PROPERTIES
                          LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${PLUGINSDIR})
endif ()

add_dependencies(VideoCapture_v4l2sys avkys)
target_include_directories(VideoCapture_v4l2sys
                           PRIVATE
                           ../..
                           ../../../../../Lib/src)
target_compile_definitions(VideoCapture_v4l2sys PRIVATE AVKYS_PLUGIN_VIDEOCAPTURE_V4L2SYS)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(VideoCapture_v4l2sys ${QT_LIBS} avkys)

if (NOT NOV4L2 AND HAVE_V4L2 AND NOT ANDROID)
    install(TARGETS VideoCapture_v4l2sys DESTINATION ${PLUGINSDIR})
endif ()
