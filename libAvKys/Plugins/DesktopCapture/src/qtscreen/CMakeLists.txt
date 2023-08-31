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

project(DesktopCapture_qtscreen LANGUAGES CXX)

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
find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             Multimedia)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             Multimedia)

if (Qt${QT_VERSION_MAJOR}Multimedia_FOUND)
    list(APPEND QT_COMPONENTS Multimedia)
endif ()

set(SOURCES
    ../screendev.cpp
    ../screendev.h
    src/plugin.cpp
    src/plugin.h
    src/qtscreendev.cpp
    src/qtscreendev.h
    pspec.json)

if (NOT NOQTSCREENCAPTURE AND Qt${QT_VERSION_MAJOR}Multimedia_FOUND)
    qt_add_plugin(DesktopCapture_qtscreen ${SOURCES})
else ()
    add_library(DesktopCapture_qtscreen EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(DesktopCapture_qtscreen PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${PLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${PLUGINSDIR})
add_dependencies(DesktopCapture_qtscreen avkys)
target_include_directories(DesktopCapture_qtscreen PRIVATE
                           ..
                           ../../../../Lib/src)
target_compile_definitions(DesktopCapture_qtscreen PRIVATE AVKYS_PLUGIN_DESKTOPCAPTURE_QTSCREEN)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(DesktopCapture_qtscreen avkys ${QT_LIBS})

if (NOT NOQTSCREENCAPTURE AND Qt${QT_VERSION_MAJOR}Multimedia_FOUND)
    install(TARGETS DesktopCapture_qtscreen
            LIBRARY DESTINATION ${PLUGINSDIR}
            RUNTIME DESTINATION ${PLUGINSDIR})
endif ()
