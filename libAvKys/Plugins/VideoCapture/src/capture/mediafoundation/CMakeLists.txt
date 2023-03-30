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

project(VideoCapture_mediafoundation LANGUAGES CXX)

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
set(CMAKE_REQUIRED_LIBRARIES
    mf
    mfplat
    mfreadwrite
    mfuuid
    ole32
    strmiids)
check_cxx_source_compiles("
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <strmif.h>
#include <dbt.h>
#include <uuids.h>

int main()
{
    MFEnumDeviceSources(nullptr, nullptr, nullptr);

    return 0;
}" HAVE_MEDIAFOUNDATION)
unset(CMAKE_REQUIRED_LIBRARIES)

set(SOURCES
    ../../capture.cpp
    ../../capture.h
    src/capturemmf.cpp
    src/capturemmf.h
    src/plugin.cpp
    src/plugin.h
    pspec.json)

if (NOT NOMEDIAFOUNDATION AND WIN32 AND HAVE_MEDIAFOUNDATION)
    add_library(VideoCapture_mediafoundation SHARED ${SOURCES})
else ()
    add_library(VideoCapture_mediafoundation SHARED EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(VideoCapture_mediafoundation PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${PLUGINSDIR})
add_dependencies(VideoCapture_mediafoundation avkys)
target_include_directories(VideoCapture_mediafoundation
                           PRIVATE
                           ../..
                           ../../../../../Lib/src)
target_compile_definitions(VideoCapture_mediafoundation PRIVATE AVKYS_PLUGIN_VIDEOCAPTURE_MEDIAFOUNDATION)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(VideoCapture_mediafoundation
                      ${QT_LIBS}
                      avkys
                      mf
                      mfplat
                      mfreadwrite
                      mfuuid
                      ole32
                      strmiids)

if (NOT NOMEDIAFOUNDATION AND WIN32 AND HAVE_MEDIAFOUNDATION)
    install(TARGETS VideoCapture_mediafoundation RUNTIME DESTINATION ${PLUGINSDIR})
endif ()
