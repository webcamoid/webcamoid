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

project(VideoMuxer_webm LANGUAGES CXX)

include(CheckCXXSourceCompiles)
include(../../../../../cmake/ProjectCommons.cmake)

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

set(CMAKE_REQUIRED_INCLUDES "${LIBWEBM_INCLUDE_DIRS}")
set(CMAKE_REQUIRED_LINK_DIRECTORIES "${LIBWEBM_LIBRARY_DIRS}")
set(CMAKE_REQUIRED_LIBRARIES webm)
check_cxx_source_compiles("
#include <mkvparser/mkvreader.h>
#include <mkvmuxer/mkvmuxer.h>
#include <mkvmuxer/mkvmuxertypes.h>
#include <mkvmuxer/mkvwriter.h>

int main()
{
    mkvmuxer::MkvWriter writer;

    return 0;
}" LIBWEBM_FOUND)
unset(CMAKE_REQUIRED_LIBRARIES)
unset(CMAKE_REQUIRED_LINK_DIRECTORIES)
unset(CMAKE_REQUIRED_INCLUDES)

set(SOURCES
    src/videomuxerwebm.h
    src/videomuxerwebmelement.h
    src/videomuxerwebm.cpp
    src/videomuxerwebmelement.cpp
    pspec.json)

if (NOT NOLIBWEBM AND LIBWEBM_FOUND)
    qt_add_plugin(VideoMuxer_webm
                  SHARED
                  CLASS_NAME VideoMuxerWebm)
    target_sources(VideoMuxer_webm PRIVATE
                   ${SOURCES})
else ()
    add_library(VideoMuxer_webm EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(VideoMuxer_webm PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(VideoMuxer_webm PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(VideoMuxer_webm avkys)
target_include_directories(VideoMuxer_webm
                           PUBLIC
                           ${LIBWEBM_INCLUDE_DIRS}
                           PRIVATE
                           ../../../../../Lib/src)
target_compile_definitions(VideoMuxer_webm PRIVATE AVKYS_PLUGIN_VIDEOMUXER_WEBM)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_directories(VideoMuxer_webm
                        PUBLIC
                        ${LIBWEBM_LIBRARY_DIRS})
target_link_libraries(VideoMuxer_webm
                      ${QT_LIBS}
                      avkys
                      webm)

if (NOT NOLIBWEBM AND LIBWEBM_FOUND)
    install(TARGETS VideoMuxer_webm
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
