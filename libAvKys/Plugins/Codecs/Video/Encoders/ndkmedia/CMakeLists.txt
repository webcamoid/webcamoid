# Webcamoid, camera capture application.
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

project(VideoEncoder_ndkmedia LANGUAGES CXX)

include(../../../../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Core
    Gui)
find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_library(NDKMEDIA NAMES mediandk)

set(SOURCES
    src/videoencoderndkmedia.cpp
    src/videoencoderndkmedia.h
    src/videoencoderndkmediaelement.cpp
    src/videoencoderndkmediaelement.h
    pspec.json)

if (NOT NONDKMEDIA AND NDKMEDIA)
    qt_add_plugin(VideoEncoder_ndkmedia
                  SHARED
                  CLASS_NAME VideoEncoderNDKMedia)
    target_sources(VideoEncoder_ndkmedia PRIVATE
                   ${SOURCES})
else ()
    add_library(VideoEncoder_ndkmedia EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(VideoEncoder_ndkmedia PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(VideoEncoder_ndkmedia PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(VideoEncoder_ndkmedia avkys)
target_include_directories(VideoEncoder_ndkmedia
                           PRIVATE
                           ../../../../../Lib/src)
target_compile_definitions(VideoEncoder_ndkmedia PRIVATE AVKYS_PLUGIN_VIDEOENCODER_NDKMEDIA)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(VideoEncoder_ndkmedia
                      ${QT_LIBS}
                      avkys)

if (NOT NONDKMEDIA AND NDKMEDIA)
    target_link_libraries(VideoEncoder_ndkmedia ${NDKMEDIA})
endif ()

if (NOT NONDKMEDIA AND NDKMEDIA)
    install(TARGETS VideoEncoder_ndkmedia
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
