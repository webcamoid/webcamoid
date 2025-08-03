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

project(VideoEncoder_svtav1 LANGUAGES CXX)

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
find_package(PkgConfig)

set(SOURCES
    src/videoencodersvtav1.cpp
    src/videoencodersvtav1.h
    src/videoencodersvtav1element.cpp
    src/videoencodersvtav1element.h
    pspec.json)

pkg_check_modules(SVTAV1 SvtAv1Enc)

if (NOT NOSVTAV1 AND SVTAV1_FOUND)
    qt_add_plugin(VideoEncoder_svtav1
                  SHARED
                  CLASS_NAME VideoEncoderVpx)
    target_sources(VideoEncoder_svtav1 PRIVATE
                   ${SOURCES})
else ()
    add_library(VideoEncoder_svtav1 EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(VideoEncoder_svtav1 PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(VideoEncoder_svtav1 PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(VideoEncoder_svtav1 avkys)
target_include_directories(VideoEncoder_svtav1
                           PUBLIC
                           ${SVTAV1_INCLUDE_DIRS}
                           PRIVATE
                           ../../../../../Lib/src)
target_compile_definitions(VideoEncoder_svtav1 PRIVATE AVKYS_PLUGIN_VIDEOENCODER_SVTAV1)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_directories(VideoEncoder_svtav1
                        PUBLIC
                        ${SVTAV1_LIBRARY_DIRS})
target_link_libraries(VideoEncoder_svtav1
                      ${QT_LIBS}
                      ${SVTAV1_LIBRARIES}
                      avkys)

if (NOT NOSVTAV1 AND SVTAV1_FOUND)
    install(TARGETS VideoEncoder_svtav1
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
