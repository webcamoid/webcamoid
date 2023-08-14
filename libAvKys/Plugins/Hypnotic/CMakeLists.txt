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

project(Hypnotic LANGUAGES CXX)

include(../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Gui
    Qml)
find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
add_library(Hypnotic SHARED
            src/hypnotic.h
            src/hypnoticelement.h
            src/hypnotic.cpp
            src/hypnoticelement.cpp
            Hypnotic.qrc
            pspec.json)

if (WIN32)
    set_target_properties(Hypnotic PROPERTIES
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${PLUGINSDIR})
elseif (NOT ANDROID)
    set_target_properties(Hypnotic PROPERTIES
                          LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${PLUGINSDIR})
endif ()

add_dependencies(Hypnotic avkys)
target_include_directories(Hypnotic
                           PRIVATE ../../Lib/src)
target_compile_definitions(Hypnotic PRIVATE AVKYS_PLUGIN_HYPNOTIC)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(Hypnotic avkys ${QT_LIBS})

if (WIN32)
    install(TARGETS Hypnotic RUNTIME DESTINATION ${PLUGINSDIR})
elseif (NOT ANDROID)
    install(TARGETS Hypnotic DESTINATION ${PLUGINSDIR})
endif ()
