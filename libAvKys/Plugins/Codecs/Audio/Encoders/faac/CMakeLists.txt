# Webcamoid, webcam capture application.
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

project(AudioEncoder_faac LANGUAGES CXX)

include(CheckCXXSourceCompiles)
include(../../../../../cmake/ProjectCommons.cmake)

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
find_package(PkgConfig)

set(SOURCES
    src/audioencoderfaac.h
    src/audioencoderfaacelement.h
    src/audioencoderfaac.cpp
    src/audioencoderfaacelement.cpp
    AudioEncoderFaac.qrc
    pspec.json)

pkg_check_modules(FAAC faac)

if (NOT NOFAAC AND FAAC_FOUND)
    qt_add_plugin(AudioEncoder_faac
                  SHARED
                  CLASS_NAME AudioEncoderFaac)
    target_sources(AudioEncoder_faac PRIVATE
                   ${SOURCES})
else ()
    add_library(AudioEncoder_faac EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(AudioEncoder_faac PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(AudioEncoder_faac PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(AudioEncoder_faac avkys)
target_include_directories(AudioEncoder_faac
                           PUBLIC
                           ${FAAC_INCLUDE_DIRS}
                           PRIVATE
                           ../../../../../Lib/src)
target_compile_definitions(AudioEncoder_faac PRIVATE AVKYS_PLUGIN_AUDIOENCODER_FAAC)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_directories(AudioEncoder_faac
                        PUBLIC
                        ${FAAC_LIBRARY_DIRS})
target_link_libraries(AudioEncoder_faac
                      ${QT_LIBS}
                      ${FAAC_LIBRARIES}
                      avkys)
set(QML_IMPORT_PATH ../../../../../Lib/share/qml)

if (NOT NOFAAC AND FAAC_FOUND)
    install(TARGETS AudioEncoder_faac
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
