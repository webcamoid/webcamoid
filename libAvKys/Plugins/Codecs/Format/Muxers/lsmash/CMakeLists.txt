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

project(VideoMuxer_lsmash LANGUAGES CXX)

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
find_package(PkgConfig)

set(SOURCES
    src/videomuxerlsmash.h
    src/videomuxerlsmashelement.h
    src/videomuxerlsmash.cpp
    src/videomuxerlsmashelement.cpp
    pspec.json)

pkg_check_modules(LSMASH liblsmash)

if (NOT NOLSMASH AND LSMASH_FOUND)
    qt_add_plugin(VideoMuxer_lsmash
                  SHARED
                  CLASS_NAME VideoMuxerLSmash)
    target_sources(VideoMuxer_lsmash PRIVATE
                   ${SOURCES})
else ()
    add_library(VideoMuxer_lsmash EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(VideoMuxer_lsmash PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(VideoMuxer_lsmash PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(VideoMuxer_lsmash avkys)
target_include_directories(VideoMuxer_lsmash
                           PUBLIC
                           ${LSMASH_INCLUDE_DIRS}
                           PRIVATE
                           ../../../../../Lib/src)
target_compile_definitions(VideoMuxer_lsmash PRIVATE AVKYS_PLUGIN_VIDEOMUXER_LSMASH)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_directories(VideoMuxer_lsmash
                        PUBLIC
                        ${LSMASH_LIBRARY_DIRS})
target_link_libraries(VideoMuxer_lsmash
                      ${QT_LIBS}
                      avkys
                      ${LSMASH_LIBRARIES})

if (NOT NOLSMASH AND LSMASH_FOUND)
    install(TARGETS VideoMuxer_lsmash
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
