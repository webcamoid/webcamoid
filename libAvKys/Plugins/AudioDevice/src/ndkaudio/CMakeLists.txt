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

project(AudioDevice_ndkaudio LANGUAGES CXX)

include(../../../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Core)

find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_library(AAUDIO NAMES aaudio)

set(SOURCES
    ../audiodev.cpp
    ../audiodev.h
    src/audiodevndkaudio.cpp
    src/audiodevndkaudio.h
    src/plugin.cpp
    src/plugin.h
    pspec.json)

if (NOT NONDKAUDIO AND AAUDIO)
    qt_add_plugin(AudioDevice_ndkaudio
                  SHARED
                  CLASS_NAME Plugin)
    target_sources(AudioDevice_ndkaudio PRIVATE
                   ${SOURCES})
else ()
    add_library(AudioDevice_ndkaudio EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(AudioDevice_ndkaudio PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(AudioDevice_ndkaudio PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(AudioDevice_ndkaudio avkys)
target_include_directories(AudioDevice_ndkaudio
                           PRIVATE
                           ..
                           ../../../../Lib/src)
target_compile_definitions(AudioDevice_ndkaudio PRIVATE AVKYS_PLUGIN_AUDIODEVICE_NDKAUDIO)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(AudioDevice_ndkaudio
                      PUBLIC
                      ${QT_LIBS}
                      avkys)

if (NOT NONDKAUDIO AND AAUDIO)
    target_link_libraries(AudioDevice_ndkaudio
                          PRIVATE
                          Qt${QT_VERSION_MAJOR}::CorePrivate
                          ${AAUDIO})

    install(TARGETS AudioDevice_ndkaudio
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
