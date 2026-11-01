# Webcamoid, camera capture application.
# Copyright (C) 2026  Gonzalo Exequiel Pedone
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

project(TextSrc LANGUAGES CXX)

include(../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Concurrent
    Gui
    Qml)
find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
qt_add_plugin(TextSrc
              SHARED
              CLASS_NAME TextSrc)
target_sources(TextSrc PRIVATE
               src/textsrc.h
               src/textsrcelement.h
               src/textsrc.cpp
               src/textrenderer.h
               src/textsrcelement.cpp
               src/textrenderer.cpp
               TextSrc.qrc
               pspec.json)

set_target_properties(TextSrc PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(TextSrc PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(TextSrc avkys)
target_include_directories(TextSrc
                           PRIVATE ../../Lib/src)
target_compile_definitions(TextSrc PRIVATE AVKYS_PLUGIN_TEXTSRC)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(TextSrc avkys ${QT_LIBS})

install(TARGETS TextSrc
        LIBRARY DESTINATION ${AKPLUGINSDIR}
        RUNTIME DESTINATION ${AKPLUGINSDIR})
