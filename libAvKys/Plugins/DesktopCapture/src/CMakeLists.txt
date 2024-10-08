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

project(DesktopCaptureSrc LANGUAGES CXX)

include(../../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Qml)
find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
qt_add_plugin(DesktopCaptureSrc
              SHARED
              CLASS_NAME DesktopCapture)
target_sources(DesktopCaptureSrc PRIVATE
               desktopcapture.cpp
               desktopcapture.h
               desktopcaptureelement.cpp
               desktopcaptureelement.h
               screendev.cpp
               screendev.h
               DesktopCapture.qrc
               pspec.json)

set_target_properties(DesktopCaptureSrc PROPERTIES
                      OUTPUT_NAME DesktopCapture
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(DesktopCaptureSrc PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(DesktopCaptureSrc avkys)
target_include_directories(DesktopCaptureSrc
                           PRIVATE ../../../Lib/src)
target_compile_definitions(DesktopCaptureSrc PRIVATE AVKYS_PLUGIN_DESKTOPCAPTURE)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(DesktopCaptureSrc avkys ${QT_LIBS})

install(TARGETS DesktopCaptureSrc
        LIBRARY DESTINATION ${AKPLUGINSDIR}
        RUNTIME DESTINATION ${AKPLUGINSDIR})
