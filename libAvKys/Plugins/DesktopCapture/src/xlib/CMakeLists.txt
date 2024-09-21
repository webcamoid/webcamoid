# Webcamoid, webcam capture application.
# Copyright (C) 2023  Gonzalo Exequiel Pedone
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

project(DesktopCapture_xlib LANGUAGES CXX)

include(../../../../cmake/ProjectCommons.cmake)

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
find_package(PkgConfig)

set(SOURCES
    ../screendev.cpp
    ../screendev.h
    src/plugin.cpp
    src/plugin.h
    src/xlibdev.cpp
    src/xlibdev.h
    pspec.json)

pkg_check_modules(XLIB x11)
pkg_check_modules(XEXT xext)
pkg_check_modules(XFIXES xfixes)
pkg_check_modules(XRANDR xrandr)

if (XLIB_FOUND AND NOT ANDROID AND NOT WIN32 AND NOT NOXLIBSCREENCAP AND NOT NOSCREENCAPTURE)
    qt_add_plugin(DesktopCapture_xlib
                  SHARED
                  CLASS_NAME Plugin)
    target_sources(DesktopCapture_xlib PRIVATE
                   ${SOURCES})
else ()
    add_library(DesktopCapture_xlib EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(DesktopCapture_xlib PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(DesktopCapture_xlib PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

if (XEXT_FOUND)
    add_definitions(-DHAVE_XEXT_SUPPORT)
endif ()

if (XFIXES_FOUND)
    add_definitions(-DHAVE_XFIXES_SUPPORT)
endif ()

if (XRANDR_FOUND)
    add_definitions(-DHAVE_XRANDR_SUPPORT)
endif ()

add_dependencies(DesktopCapture_xlib avkys)
target_include_directories(DesktopCapture_xlib
                           PUBLIC
                           ${XLIB_INCLUDE_DIRS}
                           ${XEXT_INCLUDE_DIRS}
                           ${XFIXES_INCLUDE_DIRS}
                           ${XRANDR_INCLUDE_DIRS}
                           PRIVATE
                           ..
                           ../../../../Lib/src)
target_compile_definitions(DesktopCapture_xlib PRIVATE AVKYS_PLUGIN_DESKTOPCAPTURE_XLIB)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)

target_link_directories(DesktopCapture_xlib
                        PUBLIC
                        ${XLIB_LIBRARY_DIRS}
                        ${XEXT_LIBRARY_DIRS}
                        ${XFIXES_LIBRARY_DIRS}
                        ${XRANDR_LIBRARY_DIRS})
target_link_libraries(DesktopCapture_xlib
                      ${QT_LIBS}
                      ${XLIB_LIBRARIES}
                      ${XEXT_LIBRARIES}
                      ${XFIXES_LIBRARIES}
                      ${XRANDR_LIBRARIES}
                      avkys)

if (XLIB_FOUND AND NOT ANDROID AND NOT WIN32 AND NOT NOXLIBSCREENCAP AND NOT NOSCREENCAPTURE)
    install(TARGETS DesktopCapture_xlib
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
