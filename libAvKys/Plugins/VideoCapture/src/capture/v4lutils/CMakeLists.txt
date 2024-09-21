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

project(VideoCapture_v4lutils LANGUAGES CXX)

include(CheckCXXSourceCompiles)
include(../../../../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Concurrent
    Core)
find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(PkgConfig)

check_cxx_source_compiles("
#ifndef __FreeBSD__
    #error Not BSD
#endif

int main()
{
    return 0;
}" IS_BSD)

if (IS_BSD)
    set(CMAKE_REQUIRED_INCLUDES /usr/local/include)
endif ()

check_cxx_source_compiles("
#include <linux/videodev2.h>

int main()
{
    return 0;
}" HAVE_V4L2)

set(SOURCES
    ../../capture.cpp
    ../../capture.h
    ../v4l2sys/src/capturev4l2.cpp
    ../v4l2sys/src/capturev4l2.h
    ../v4l2sys/src/plugin.cpp
    ../v4l2sys/src/plugin.h
    ../v4l2sys/src/ioctldefs.h
    pspec.json)

pkg_check_modules(LIBUSB libusb-1.0)

if (NOT NOLIBUSB AND LIBUSB_FOUND)
    list(APPEND SOURCES
         ../v4l2sys/src/guid.h
         ../v4l2sys/src/uvcextendedcontrols.h
         ../v4l2sys/src/guid.cpp
         ../v4l2sys/src/uvcextendedcontrols.cpp
         ../v4l2sys/v4l2.qrc)
endif ()

pkg_check_modules(V4LUTILS libv4l2)

if (NOT NOV4L2 AND HAVE_V4L2 AND V4LUTILS_FOUND AND NOT ANDROID)
    qt_add_plugin(VideoCapture_v4lutils
                  SHARED
                  CLASS_NAME Plugin)
    target_sources(VideoCapture_v4lutils PRIVATE
                   ${SOURCES})
else ()
    add_library(VideoCapture_v4lutils EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(VideoCapture_v4lutils PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(VideoCapture_v4lutils PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(VideoCapture_v4lutils avkys)
target_include_directories(VideoCapture_v4lutils
                           PUBLIC
                           ${V4LUTILS_INCLUDE_DIRS}
                           PRIVATE
                           ../..
                           ../../../../../Lib/src)
target_compile_definitions(VideoCapture_v4lutils PRIVATE AVKYS_PLUGIN_VIDEOCAPTURE_V4LUTILS)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_directories(VideoCapture_v4lutils
                        PUBLIC
                        ${V4LUTILS_LIBRARY_DIRS})
target_link_libraries(VideoCapture_v4lutils
                      ${QT_LIBS}
                      ${V4LUTILS_LIBRARIES}
                      avkys)
add_definitions(-DHAVE_V4LUTILS)

if (NOT NOLIBUSB AND LIBUSB_FOUND)
    target_link_directories(VideoCapture_v4lutils
                            PUBLIC
                            ${LIBUSB_LIBRARY_DIRS})
    target_include_directories(VideoCapture_v4lutils
                               PUBLIC
                               ${LIBUSB_INCLUDE_DIRS})
    target_link_libraries(VideoCapture_v4lutils
                          ${LIBUSB_LIBRARIES})
    add_definitions(-DHAVE_LIBUSB)
endif ()

add_definitions(-DBINDIR="${BINDIR}"
                -DDATAROOTDIR="${DATAROOTDIR}")

if (NOT NOV4L2 AND HAVE_V4L2 AND V4LUTILS_FOUND AND NOT ANDROID)
    install(TARGETS VideoCapture_v4lutils
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
