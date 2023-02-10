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

project(VideoCapture_dshow LANGUAGES CXX)

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

set(SOURCES
    ../../capture.cpp
    ../../capture.h
    src/capturedshow.cpp
    src/capturedshow.h
    src/framegrabber.cpp
    src/framegrabber.h
    src/plugin.cpp
    src/plugin.h
    src/samplegrabber.h
    pspec.json)

if (NOT NODSHOW AND WIN32)
    add_library(VideoCapture_dshow SHARED ${SOURCES})
else ()
    add_library(VideoCapture_dshow SHARED EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(VideoCapture_dshow PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${PLUGINSDIR})
add_dependencies(VideoCapture_dshow avkys)
target_include_directories(VideoCapture_dshow
                           PRIVATE
                           ../..
                           ../../../../../Lib/src)
target_compile_definitions(VideoCapture_dshow PRIVATE AVKYS_PLUGIN_VIDEOCAPTURE_DSHOW)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(VideoCapture_dshow
                      ${QT_LIBS}
                      avkys
                      strmiids
                      uuid
                      ole32
                      oleaut32)

if (NOT NODSHOW AND WIN32)
    install(TARGETS VideoCapture_dshow RUNTIME DESTINATION ${PLUGINSDIR})
endif ()
