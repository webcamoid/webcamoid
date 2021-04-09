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

project(VirtualCamera_akvcam LANGUAGES CXX)

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
    src/vcamak.cpp
    src/vcamak.h
    pspec.json)

if (UNIX AND NOT APPLE AND NOT ANDROID)
    add_library(VirtualCamera_akvcam SHARED ${SOURCES})
else ()
    add_library(VirtualCamera_akvcam SHARED EXCLUDE_FROM_ALL ${SOURCES})
endif ()

add_dependencies(VirtualCamera_akvcam avkys)
target_include_directories(VirtualCamera_akvcam PRIVATE
                           ..
                           ../../../../Lib/src)
target_compile_definitions(VirtualCamera_akvcam PRIVATE AVKYS_PLUGIN_VIRTUALCAMERA_AKVCAM)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(VirtualCamera_akvcam PRIVATE avkys ${QT_LIBS})

if (UNIX AND NOT APPLE AND NOT ANDROID)
    install(TARGETS avkys DESTINATION ${PLUGINSDIR})
endif ()
