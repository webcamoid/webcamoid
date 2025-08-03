# Webcamoid, camera capture application.
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

project(DesktopCapture_androidscreensrc LANGUAGES CXX)

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

set(SOURCES
    ../../screendev.cpp
    ../../screendev.h
    androidscreendev.cpp
    androidscreendev.h
    plugin.h
    plugin.cpp
    pspec.json)

if (ANDROID AND NOT NOSCREENCAPTURE)
    qt_add_plugin(DesktopCapture_androidscreensrc
                  SHARED
                  CLASS_NAME Plugin)
    target_sources(DesktopCapture_androidscreensrc PRIVATE
                   ${SOURCES})
else ()
    add_library(DesktopCapture_androidscreensrc EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(DesktopCapture_androidscreensrc PROPERTIES
                      OUTPUT_NAME DesktopCapture_androidscreen
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(DesktopCapture_androidscreensrc PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(DesktopCapture_androidscreensrc avkys)
target_include_directories(DesktopCapture_androidscreensrc
                           PRIVATE
                           ../..
                           ../../../../../Lib/src)
target_compile_definitions(DesktopCapture_androidscreensrc PRIVATE AVKYS_PLUGIN_DESKTOPCAPTURE_ANDROIDSCREEN)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(DesktopCapture_androidscreensrc
                      PUBLIC
                      ${QT_LIBS}
                      avkys)

if (ANDROID)
    target_link_libraries(DesktopCapture_androidscreensrc
                          PRIVATE
                          Qt${QT_VERSION_MAJOR}::CorePrivate)

    if (NOT NOSCREENCAPTURE)
        install(TARGETS DesktopCapture_androidscreensrc
                LIBRARY DESTINATION ${AKPLUGINSDIR}
                RUNTIME DESTINATION ${AKPLUGINSDIR})
    endif ()
endif ()
