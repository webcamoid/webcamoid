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

project(DesktopCapture_avfoundation LANGUAGES CXX)

include(../../../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Concurrent
    Widgets)
find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_library(FOUNDATION NAMES Foundation)
find_library(AVFOUNDATION NAMES AVFoundation)
find_library(COREGRAPHICS NAMES CoreGraphics)
find_library(COREMEDIA NAMES CoreMedia)
find_library(COREVIDEO NAMES CoreVideo)

set(SOURCES
    ../screendev.cpp
    ../screendev.h
    src/avfoundationscreendev.h
    src/avfoundationscreendev.mm
    src/framegrabber.h
    src/framegrabber.mm
    src/plugin.cpp
    src/plugin.h
    pspec.json)

if (NOT NOAVFOUNDATION
    AND FOUNDATION
    AND AVFOUNDATION
    AND COREGRAPHICS
    AND COREMEDIA
    AND COREVIDEO)
    add_library(DesktopCapture_avfoundation SHARED ${SOURCES})
else ()
    add_library(DesktopCapture_avfoundation SHARED EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(DesktopCapture_avfoundation PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${PLUGINSDIR})
add_dependencies(DesktopCapture_avfoundation avkys)
target_include_directories(DesktopCapture_avfoundation
                           PRIVATE
                           ..
                           ../../../../Lib/src)
target_compile_definitions(DesktopCapture_avfoundation PRIVATE AVKYS_PLUGIN_DESKTOPCAPTURE_AVFOUNDATION)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(DesktopCapture_avfoundation
                      ${QT_LIBS}
                      avkys)

if (NOT NOAVFOUNDATION
    AND FOUNDATION
    AND AVFOUNDATION
    AND COREGRAPHICS
    AND COREMEDIA
    AND COREVIDEO)
    target_link_libraries(DesktopCapture_avfoundation
                          ${FOUNDATION}
                          ${AVFOUNDATION}
                          ${COREGRAPHICS}
                          ${COREMEDIA}
                          ${COREVIDEO})
endif ()

install(TARGETS avkys DESTINATION ${PLUGINSDIR})
