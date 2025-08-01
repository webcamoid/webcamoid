# Webcamoid, webcam capture application.
# Copyright (C) 2022  Gonzalo Exequiel Pedone
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

project(DesktopCapture_pipewire LANGUAGES CXX)

include(../../../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Concurrent
    Widgets)

if (NOT ANDROID)
    list(APPEND QT_COMPONENTS DBus)
endif ()

find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(PkgConfig)

set(SOURCES
    ../screendev.cpp
    ../screendev.h
    src/plugin.cpp
    src/plugin.h
    src/pipewirescreendev.cpp
    src/pipewirescreendev.h
    pspec.json)

pkg_check_modules(PIPEWIRE libpipewire-0.3)

if (PIPEWIRE_FOUND AND NOT ANDROID AND NOT (APPLE OR FAKE_APPLE) AND NOT WIN32 AND NOT NOPIPEWIRE AND NOT NOSCREENCAPTURE)
    qt_add_plugin(DesktopCapture_pipewire
                  SHARED
                  CLASS_NAME Plugin)
    target_sources(DesktopCapture_pipewire PRIVATE
                   ${SOURCES})
else ()
    add_library(DesktopCapture_pipewire EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(DesktopCapture_pipewire PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(DesktopCapture_pipewire PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(DesktopCapture_pipewire avkys)
target_include_directories(DesktopCapture_pipewire PRIVATE
                           PUBLIC
                           ${PIPEWIRE_INCLUDE_DIRS}
                           PRIVATE
                           ..
                           ../../../../Lib/src)
target_compile_definitions(DesktopCapture_pipewire PRIVATE AVKYS_PLUGIN_DESKTOPCAPTURE_PIPEWIRE)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_directories(DesktopCapture_pipewire
                        PUBLIC
                        ${PIPEWIRE_LIBRARY_DIRS})
target_link_libraries(DesktopCapture_pipewire
                      ${QT_LIBS}
                      avkys)

if (PIPEWIRE_DYNLOAD)
    add_definitions(-DUSE_PIPEWIRE_DYNLOAD)
else ()
    target_link_libraries(AudioDevice_pipewire
                          ${PIPEWIRE_LIBRARIES})
endif ()

if (PIPEWIRE_FOUND AND NOT ANDROID AND NOT (APPLE OR FAKE_APPLE) AND NOT WIN32 AND NOT NOPIPEWIRE AND NOT NOSCREENCAPTURE)
    install(TARGETS DesktopCapture_pipewire
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
