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

project(AudioDevice_wasapi LANGUAGES CXX)

include(../../../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Core)
find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)

set(SOURCES
    ../audiodev.cpp
    ../audiodev.h
    src/audiodevwasapi.cpp
    src/audiodevwasapi.h
    src/plugin.cpp
    src/plugin.h
    pspec.json)

if (NOT NOWASAPI AND WIN32)
    add_library(AudioDevice_wasapi SHARED ${SOURCES})
else ()
    add_library(AudioDevice_wasapi SHARED EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(AudioDevice_wasapi PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${PLUGINSDIR})
add_dependencies(AudioDevice_wasapi avkys)
target_include_directories(AudioDevice_wasapi
                           PRIVATE
                           ..
                           ../../../../Lib/src)
target_compile_definitions(AudioDevice_wasapi PRIVATE AVKYS_PLUGIN_AUDIODEVICE_WASAPI)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(AudioDevice_wasapi
                      ${QT_LIBS}
                      avkys
                      ole32
                      winmm)

if (NOT NOWASAPI AND WIN32)
    install(TARGETS AudioDevice_wasapi RUNTIME DESTINATION ${PLUGINSDIR})
endif ()
