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

project(VideoEncoder_svtvp9 LANGUAGES CXX)

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
    src/videoencodersvtvp9.cpp
    src/videoencodersvtvp9.h
    src/videoencodersvtvp9element.cpp
    src/videoencodersvtvp9element.h
    pspec.json)

pkg_check_modules(SVTVP9 SvtVp9Enc)

if (NOT NOSVTVP9 AND SVTVP9_FOUND)
    qt_add_plugin(VideoEncoder_svtvp9
                  SHARED
                  CLASS_NAME VideoEncoderVpx)
    target_sources(VideoEncoder_svtvp9 PRIVATE
                   ${SOURCES})
else ()
    add_library(VideoEncoder_svtvp9 EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(VideoEncoder_svtvp9 PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(VideoEncoder_svtvp9 PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(VideoEncoder_svtvp9 avkys)
target_include_directories(VideoEncoder_svtvp9
                           PUBLIC
                           ${SVTVP9_INCLUDE_DIRS}
                           PRIVATE
                           ../../../../../Lib/src)
target_compile_definitions(VideoEncoder_svtvp9 PRIVATE AVKYS_PLUGIN_VIDEOENCODER_SVTVP9)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_directories(VideoEncoder_svtvp9
                        PUBLIC
                        ${SVTVP9_LIBRARY_DIRS})
target_link_libraries(VideoEncoder_svtvp9
                      ${QT_LIBS}
                      ${SVTVP9_LIBRARIES}
                      avkys)

if (NOT NOSVTVP9 AND SVTVP9_FOUND)
    install(TARGETS VideoEncoder_svtvp9
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
