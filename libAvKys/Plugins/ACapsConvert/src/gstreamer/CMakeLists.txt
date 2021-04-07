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

project(ACapsConvert_gstreamer LANGUAGES CXX)

include(../../../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Concurrent)
find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(PkgConfig)

set(SOURCES
    ../convertaudio.cpp
    ../convertaudio.h
    src/convertaudiogstreamer.cpp
    src/convertaudiogstreamer.h
    src/plugin.cpp
    src/plugin.h
    pspec.json)

pkg_check_modules(GSTREAMER
                  gstreamer-1.0
                  gstreamer-app-1.0
                  gstreamer-audio-1.0
                  gstreamer-pbutils-1.0)
link_directories(${GSTREAMER_LIBRARY_DIRS})

if (NOT NOGSTREAMER AND GSTREAMER_FOUND)
    add_library(ACapsConvert_gstreamer SHARED ${SOURCES})
else ()
    add_library(ACapsConvert_gstreamer SHARED EXCLUDE_FROM_ALL ${SOURCES})
endif ()

add_dependencies(ACapsConvert_gstreamer avkys)
target_include_directories(ACapsConvert_gstreamer
                           PUBLIC
                           ${GSTREAMER_INCLUDE_DIRS}
                           PRIVATE
                           ..
                           ../../../../Lib/src)
target_compile_definitions(ACapsConvert_gstreamer PRIVATE AVKYS_PLUGIN_ACAPSCONVERT_GSTREAMER)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(ACapsConvert_gstreamer
                      PUBLIC
                      ${QT_LIBS}
                      ${GSTREAMER_LIBRARIES}
                      PRIVATE
                      avkys)

if (WIN32)
    install(TARGETS avkys RUNTIME DESTINATION ${INSTALLPLUGINSDIR})
elseif (NOT ANDROID)
    install(TARGETS avkys DESTINATION ${INSTALLPLUGINSDIR})
endif ()
