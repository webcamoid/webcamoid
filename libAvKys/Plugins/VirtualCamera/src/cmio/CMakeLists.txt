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

project(VirtualCamera_cmio LANGUAGES CXX)

include(../../../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Gui)
find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
set(SOURCES
    ../vcam.cpp
    ../vcam.h
    src/plugin.cpp
    src/plugin.h
    src/vcamcmio.cpp
    src/vcamcmio.h
    pspec.json)

if (APPLE)
    add_library(VirtualCamera_cmio SHARED ${SOURCES})
else ()
    add_library(VirtualCamera_cmio SHARED EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(VirtualCamera_cmio PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${PLUGINSDIR})
add_dependencies(VirtualCamera_cmio avkys)
target_include_directories(VirtualCamera_cmio PRIVATE
                           ..
                           ../../../../Lib/src)
target_compile_definitions(VirtualCamera_cmio PRIVATE AVKYS_PLUGIN_VIRTUALCAMERA_CMIO)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(VirtualCamera_cmio avkys ${QT_LIBS})

if (APPLE)
    install(TARGETS VirtualCamera_cmio DESTINATION ${PLUGINSDIR})
endif ()
