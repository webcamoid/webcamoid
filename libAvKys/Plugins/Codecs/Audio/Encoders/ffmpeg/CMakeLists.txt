# Webcamoid, camera capture application.
# Copyright (C) 2024  Gonzalo Exequiel Pedone
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

project(AudioEncoder_ffmpeg LANGUAGES CXX)

include(../../../../../cmake/ProjectCommons.cmake)

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
find_package(PkgConfig)

set(SOURCES
    src/audioencoderffmpeg.cpp
    src/audioencoderffmpeg.h
    src/audioencoderffmpegelement.cpp
    src/audioencoderffmpegelement.h
    pspec.json)

pkg_check_modules(FFMPEG
                  libavformat
                  libavcodec
                  libavutil)

if (NOT NOFFMPEG AND FFMPEG_FOUND)
    qt_add_plugin(AudioEncoder_ffmpeg
                  SHARED
                  CLASS_NAME AudioEncoderFFmpeg)
    target_sources(AudioEncoder_ffmpeg PRIVATE
                   ${SOURCES})
else ()
    add_library(AudioEncoder_ffmpeg EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(AudioEncoder_ffmpeg PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(AudioEncoder_ffmpeg PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(AudioEncoder_ffmpeg avkys)
target_include_directories(AudioEncoder_ffmpeg
                           PUBLIC
                           ${FFMPEG_INCLUDE_DIRS}
                           PRIVATE
                           ../../../../../Lib/src)
target_compile_definitions(AudioEncoder_ffmpeg PRIVATE AVKYS_PLUGIN_AUDIOENCODER_FFMPEG)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_directories(AudioEncoder_ffmpeg
                        PUBLIC
                        ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(AudioEncoder_ffmpeg
                      ${QT_LIBS}
                      ${FFMPEG_LIBRARIES}
                      avkys)

if (NOT NOFFMPEG AND FFMPEG_FOUND)
    install(TARGETS AudioEncoder_ffmpeg
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
