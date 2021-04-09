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

project(VideoCapture_cmio LANGUAGES CXX)

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
find_library(AVFOUNDATION NAMES AVFoundation)
find_library(COREFOUNDATION NAMES CoreFoundation)
find_library(COREMEDIA NAMES CoreMedia)
find_library(COREMEDIAIO NAMES CoreMediaIO)
find_library(COREVIDEO NAMES CoreVideo)
find_library(FOUNDATION NAMES Foundation)

set(SOURCES
    ../capture.cpp
    ../capture.h
    src/capturecmio.h
    src/capturecmio.mm
    src/deviceobserver.h
    src/deviceobserver.mm
    src/plugin.cpp
    src/plugin.h
    pspec.json)

if (NOT NOAVFOUNDATION
    AND AVFOUNDATION
    AND COREFOUNDATION
    AND COREMEDIA
    AND COREMEDIAIO
    AND COREVIDEO
    AND FOUNDATION)
    add_library(VideoCapture_cmio SHARED ${SOURCES})
else ()
    add_library(VideoCapture_cmio SHARED EXCLUDE_FROM_ALL ${SOURCES})
endif ()

add_dependencies(VideoCapture_cmio avkys)
target_include_directories(VideoCapture_cmio
                           PRIVATE
                           ..
                           ../../../../Lib/src)
target_compile_definitions(VideoCapture_cmio PRIVATE AVKYS_PLUGIN_VIDEOCAPTURE_CMIO)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(VideoCapture_cmio
                      PUBLIC
                      ${QT_LIBS}
                      PRIVATE
                      avkys)

if (NOT NOAVFOUNDATION
    AND AVFOUNDATION
    AND COREFOUNDATION
    AND COREMEDIA
    AND COREMEDIAIO
    AND COREVIDEO
    AND FOUNDATION)
    target_link_libraries(VideoCapture_cmio
                          PUBLIC
                          ${AVFOUNDATION}
                          ${COREFOUNDATION}
                          ${COREMEDIA}
                          ${COREMEDIAIO}
                          ${COREVIDEO}
                          ${FOUNDATION})
endif ()

if (WIN32)
    install(TARGETS avkys RUNTIME DESTINATION ${PLUGINSDIR})
elseif (NOT ANDROID)
    install(TARGETS avkys DESTINATION ${PLUGINSDIR})
endif ()
