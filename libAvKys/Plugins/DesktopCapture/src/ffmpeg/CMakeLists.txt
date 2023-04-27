# Webcamoid, webcam capture application.
# Copyright (C) 2023  Gonzalo Exequiel Pedone
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

project(DesktopCapture_ffmpeg LANGUAGES CXX)

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
find_package(PkgConfig)

set(SOURCES
    ../screendev.cpp
    ../screendev.h
    src/plugin.cpp
    src/plugin.h
    src/ffmpegdev.cpp
    src/ffmpegdev.h
    pspec.json)
set(MODULES
    libavformat
    libavcodec
    libswscale
    libavutil)

if (NOT NOLIBAVDEVICE)
    list(APPEND MODULES libavdevice)
    add_definitions(-DHAVE_LIBAVDEVICE)
endif ()

pkg_check_modules(FFMPEG ${MODULES})

link_directories(${FFMPEG_LIBRARY_DIRS})

if (NOT NOFFMPEGSCREENCAP AND FFMPEG_FOUND AND NOT ANDROID)
    add_library(DesktopCapture_ffmpeg SHARED ${SOURCES})
else ()
    add_library(DesktopCapture_ffmpeg SHARED EXCLUDE_FROM_ALL ${SOURCES})
endif ()

if (WIN32)
    set_target_properties(DesktopCapture_ffmpeg PROPERTIES
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${PLUGINSDIR})
elseif (NOT ANDROID)
    set_target_properties(DesktopCapture_ffmpeg PROPERTIES
                          LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${PLUGINSDIR})
endif ()

add_dependencies(DesktopCapture_ffmpeg avkys)
target_include_directories(DesktopCapture_ffmpeg
                           PUBLIC
                           ${FFMPEG_INCLUDE_DIRS}
                           PRIVATE
                           ..
                           ../../../../Lib/src)
target_compile_definitions(DesktopCapture_ffmpeg PRIVATE AVKYS_PLUGIN_DESKTOPCAPTURE_FFMPEG)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(DesktopCapture_ffmpeg
                      ${QT_LIBS}
                      ${FFMPEG_LIBRARIES}
                      avkys)

if (NOT NOFFMPEGSCREENCAP AND FFMPEG_FOUND)
    if (WIN32)
        install(TARGETS DesktopCapture_ffmpeg RUNTIME DESTINATION ${PLUGINSDIR})
    elseif (NOT ANDROID)
        install(TARGETS DesktopCapture_ffmpeg DESTINATION ${PLUGINSDIR})
    endif ()
endif ()
