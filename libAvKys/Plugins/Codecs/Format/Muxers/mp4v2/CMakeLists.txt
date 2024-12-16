# Webcamoid, webcam capture application.
# Copyright (C) 2024  Gonzalo Exequiel Pedone
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

project(VideoMuxer_mp4v2 LANGUAGES CXX)

include(CheckCXXSourceCompiles)
include(../../../../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Gui
    Qml)
find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(PkgConfig)

set(SOURCES
    src/videomuxermp4v2.h
    src/videomuxermp4v2element.h
    src/videomuxermp4v2.cpp
    src/videomuxermp4v2element.cpp
    VideoMuxerMp4V2.qrc
    pspec.json)

pkg_check_modules(LIBMP4V2 mp4v2)

if (NOT NOLIBMP4V2 AND LIBMP4V2_FOUND)
    qt_add_plugin(VideoMuxer_mp4v2
                  SHARED
                  CLASS_NAME VideoMuxerMp4V2)
    target_sources(VideoMuxer_mp4v2 PRIVATE
                   ${SOURCES})
else ()
    add_library(VideoMuxer_mp4v2 EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(VideoMuxer_mp4v2 PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(VideoMuxer_mp4v2 PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(VideoMuxer_mp4v2 avkys)
target_include_directories(VideoMuxer_mp4v2
                           PUBLIC
                           ${LIBMP4V2_INCLUDE_DIRS}
                           PRIVATE
                           ../../../../../Lib/src)
target_compile_definitions(VideoMuxer_mp4v2 PRIVATE AVKYS_PLUGIN_VIDEOMUXER_MP4V2)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_directories(VideoMuxer_mp4v2
                        PUBLIC
                        ${LIBMP4V2_LIBRARY_DIRS})
target_link_libraries(VideoMuxer_mp4v2
                      ${QT_LIBS}
                      avkys
                      ${LIBMP4V2_LIBRARIES})
set(QML_IMPORT_PATH ../../../../../Lib/share/qml)

if (NOT NOLIBMP4V2 AND LIBMP4V2_FOUND)
    install(TARGETS VideoMuxer_mp4v2
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
