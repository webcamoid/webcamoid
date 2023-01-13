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

project(AudioDevice_opensl LANGUAGES CXX)

include(../../../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
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
find_library(OPENSLES NAMES OpenSLES)

set(SOURCES
    ../audiodev.cpp
    ../audiodev.h
    src/audiodevopensl.cpp
    src/audiodevopensl.h
    src/plugin.cpp
    src/plugin.h
    pspec.json)

if (NOT NOOPENSL AND OPENSLES)
    add_library(AudioDevice_opensl SHARED ${SOURCES})
else ()
    add_library(AudioDevice_opensl SHARED EXCLUDE_FROM_ALL ${SOURCES})
endif ()

if (WIN32)
    set_target_properties(AudioDevice_opensl PROPERTIES
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${PLUGINSDIR})
elseif (NOT ANDROID)
    set_target_properties(AudioDevice_opensl PROPERTIES
                          LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${PLUGINSDIR})
endif ()

add_dependencies(AudioDevice_opensl avkys)
target_include_directories(AudioDevice_opensl
                           PRIVATE
                           ..
                           ../../../../Lib/src)
target_compile_definitions(AudioDevice_opensl PRIVATE AVKYS_PLUGIN_AUDIODEVICE_OPENSL)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(AudioDevice_opensl
                      ${QT_LIBS}
                      avkys)

if (NOT NOOPENSL AND OPENSLES)
    target_link_libraries(AudioDevice_opensl ${OPENSLES})
endif ()

if (NOT NOOPENSL AND OPENSLES)
    if (WIN32)
        install(TARGETS AudioDevice_opensl RUNTIME DESTINATION ${PLUGINSDIR})
    elseif (NOT ANDROID)
        install(TARGETS AudioDevice_opensl DESTINATION ${PLUGINSDIR})
    endif ()
endif ()
