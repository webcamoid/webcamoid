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

project(VideoEncoder_x264 LANGUAGES CXX)

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
    src/videoencoderx264.cpp
    src/videoencoderx264.h
    src/videoencoderx264element.cpp
    src/videoencoderx264element.h
    pspec.json)

pkg_check_modules(LIBX264 x264)

if (NOT NOLIBX264 AND LIBX264_FOUND)
    qt_add_plugin(VideoEncoder_x264
                  SHARED
                  CLASS_NAME VideoEncoderX264)
    target_sources(VideoEncoder_x264 PRIVATE
                   ${SOURCES})
else ()
    add_library(VideoEncoder_x264 EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(VideoEncoder_x264 PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(VideoEncoder_x264 PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(VideoEncoder_x264 avkys)
target_include_directories(VideoEncoder_x264
                           PUBLIC
                           ${LIBX264_INCLUDE_DIRS}
                           PRIVATE
                           ../../../../../Lib/src)
target_compile_definitions(VideoEncoder_x264 PRIVATE AVKYS_PLUGIN_VIDEOENCODER_LIBX264)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_directories(VideoEncoder_x264
                        PUBLIC
                        ${LIBX264_LIBRARY_DIRS})
target_link_libraries(VideoEncoder_x264
                      ${QT_LIBS}
                      ${LIBX264_LIBRARIES}
                      avkys)

if (NOT NOLIBX264 AND LIBX264_FOUND)
    install(TARGETS VideoEncoder_x264
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
