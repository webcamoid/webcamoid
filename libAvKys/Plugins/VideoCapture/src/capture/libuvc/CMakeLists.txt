# Webcamoid, webcam capture application.
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

project(VideoCapture_libuvc LANGUAGES CXX)

include(../../../../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Concurrent)
find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(PkgConfig)

set(SOURCES
    ../../capture.cpp
    ../../capture.h
    src/capturelibuvc.cpp
    src/capturelibuvc.h
    src/plugin.cpp
    src/plugin.h
    src/usbglobals.cpp
    src/usbglobals.h
    src/usbids.cpp
    src/usbids.h
    libuvc.qrc
    pspec.json)
pkg_check_modules(LIBUVC
                  libusb-1.0
                  libuvc>=0.0.6)

if (NOT NOLIBUSB AND NOT NOLIBUVC AND LIBUVC_FOUND)
    qt_add_plugin(VideoCapture_libuvc
                  SHARED
                  CLASS_NAME Plugin)
    target_sources(VideoCapture_libuvc PRIVATE
                   ${SOURCES})
else ()
    add_library(VideoCapture_libuvc EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(VideoCapture_libuvc PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(VideoCapture_libuvc PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(VideoCapture_libuvc avkys)
target_include_directories(VideoCapture_libuvc
                           PUBLIC
                           ${LIBUVC_INCLUDE_DIRS}
                           PRIVATE
                           ../..
                           ../../../../../Lib/src)
target_compile_definitions(VideoCapture_libuvc PRIVATE AVKYS_PLUGIN_VIDEOCAPTURE_LIBUVC)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_directories(VideoCapture_libuvc
                        PUBLIC
                        ${LIBUVC_LIBRARY_DIRS})
target_link_libraries(VideoCapture_libuvc
                      ${QT_LIBS}
                      ${LIBUVC_LIBRARIES}
                      avkys)

if (NOT NOLIBUSB AND NOT NOLIBUVC AND LIBUVC_FOUND)
    install(TARGETS VideoCapture_libuvc
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
