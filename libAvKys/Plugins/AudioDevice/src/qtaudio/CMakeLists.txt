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

project(AudioDevice_qtaudio LANGUAGES CXX)

include(../../../../cmake/ProjectCommons.cmake)

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
find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             Multimedia)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             Multimedia)

if (Qt${QT_VERSION_MAJOR}Multimedia_FOUND)
    list(APPEND QT_COMPONENTS Multimedia)
endif ()

set(SOURCES
    ../audiodev.cpp
    ../audiodev.h
    src/audiodevicebuffer.cpp
    src/audiodevicebuffer.h
    src/audiodevqt.cpp
    src/audiodevqt.h
    src/plugin.cpp
    src/plugin.h
    pspec.json)

if (NOT NOQTAUDIO AND Qt${QT_VERSION_MAJOR}Multimedia_FOUND)
    qt_add_plugin(AudioDevice_qtaudio ${SOURCES})
else ()
    add_library(AudioDevice_qtaudio EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(AudioDevice_qtaudio PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})
add_dependencies(AudioDevice_qtaudio avkys)
target_include_directories(AudioDevice_qtaudio
                           PRIVATE
                           ..
                           ../../../../Lib/src)
target_compile_definitions(AudioDevice_qtaudio PRIVATE AVKYS_PLUGIN_AUDIODEVICE_QTAUDIO)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(AudioDevice_qtaudio avkys ${QT_LIBS})

if (NOT NOQTAUDIO AND Qt${QT_VERSION_MAJOR}Multimedia_FOUND)
    install(TARGETS AudioDevice_qtaudio
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
