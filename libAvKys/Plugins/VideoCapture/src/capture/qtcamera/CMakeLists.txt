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

project(VideoCapture_qtcamera LANGUAGES CXX)

include(../../../../../cmake/ProjectCommons.cmake)

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
find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             Multimedia)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             Multimedia)

if (Qt${QT_VERSION_MAJOR}Multimedia_FOUND)
    list(APPEND QT_COMPONENTS Multimedia)
endif ()

set(SOURCES
    ../../capture.cpp
    ../../capture.h
    src/captureqt.cpp
    src/captureqt.h
    src/plugin.cpp
    src/plugin.h
    src/videosurface.cpp
    src/videosurface.h
    pspec.json)

if (NOT NOQTCAMERA AND Qt${QT_VERSION_MAJOR}Multimedia_FOUND)
    add_library(VideoCapture_qtcamera SHARED ${SOURCES})
else ()
    add_library(VideoCapture_qtcamera SHARED EXCLUDE_FROM_ALL ${SOURCES})
endif ()

if (WIN32)
    set_target_properties(VideoCapture_qtcamera PROPERTIES
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${PLUGINSDIR})
elseif (NOT ANDROID)
    set_target_properties(VideoCapture_qtcamera PROPERTIES
                          LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${PLUGINSDIR})
endif ()

add_dependencies(VideoCapture_qtcamera avkys)
target_include_directories(VideoCapture_qtcamera
                           PRIVATE
                           ../..
                           ../../../../../Lib/src)
target_compile_definitions(VideoCapture_qtcamera PRIVATE AVKYS_PLUGIN_VIDEOCAPTURE_QTCAMERA)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(VideoCapture_qtcamera avkys ${QT_LIBS})

if (NOT NOQTCAMERA AND Qt${QT_VERSION_MAJOR}Multimedia_FOUND)
    if (WIN32)
        install(TARGETS VideoCapture_qtcamera RUNTIME DESTINATION ${PLUGINSDIR})
    elseif (NOT ANDROID)
        install(TARGETS VideoCapture_qtcamera DESTINATION ${PLUGINSDIR})
    endif ()
endif ()
