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

project(VideoEncoder_av1 LANGUAGES CXX)

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
    src/videoencoderav1.cpp
    src/videoencoderav1.h
    src/videoencoderav1element.cpp
    src/videoencoderav1element.h
    pspec.json)

pkg_check_modules(AOMAV1 aom)

if (NOT NOAOMAV1 AND AOMAV1_FOUND)
    qt_add_plugin(VideoEncoder_av1
                  SHARED
                  CLASS_NAME VideoEncoderAv1)
    target_sources(VideoEncoder_av1 PRIVATE
                   ${SOURCES})
else ()
    add_library(VideoEncoder_av1 EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(VideoEncoder_av1 PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(VideoEncoder_av1 PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(VideoEncoder_av1 avkys)
target_include_directories(VideoEncoder_av1
                           PUBLIC
                           ${AOMAV1_INCLUDE_DIRS}
                           PRIVATE
                           ../../../../../Lib/src)
target_compile_definitions(VideoEncoder_av1 PRIVATE AVKYS_PLUGIN_VIDEOENCODER_AOMAV1)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_directories(VideoEncoder_av1
                        PUBLIC
                        ${AOMAV1_LIBRARY_DIRS})
target_link_libraries(VideoEncoder_av1
                      ${QT_LIBS}
                      ${AOMAV1_LIBRARIES}
                      avkys)

if (NOT NOAOMAV1 AND AOMAV1_FOUND)
    install(TARGETS VideoEncoder_av1
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
