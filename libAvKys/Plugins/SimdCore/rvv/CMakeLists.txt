# Webcamoid, webcam capture application.
# Copyright (C) 2025  Gonzalo Exequiel Pedone
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

project(SimdCore_rvv LANGUAGES CXX)

include(../../../cmake/ProjectCommons.cmake)

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

set(SOURCES
    src/simdcorervv.cpp
    src/simdcorervv.h
    src/plugin.cpp
    src/plugin.h
    pspec.json)

if (BUILD_PROCESSOR_RISCV)
    qt_add_plugin(SimdCore_rvv
                  SHARED
                  CLASS_NAME Plugin)
    target_sources(SimdCore_rvv PRIVATE
                   ${SOURCES})
else ()
    add_library(SimdCore_rvv EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(SimdCore_rvv PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(SimdCore_rvv PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(SimdCore_rvv avkys)
target_include_directories(SimdCore_rvv
                           PRIVATE
                           ../../../Lib/src)
target_compile_definitions(SimdCore_rvv PRIVATE AVKYS_PLUGIN_SIMDCORE_RVV)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(SimdCore_rvv
                      PUBLIC
                      ${QT_LIBS}
                      avkys)

if (BUILD_PROCESSOR_RISCV)
    target_link_libraries(SimdCore_rvv
                          PRIVATE
                          Qt${QT_VERSION_MAJOR}::CorePrivate)

    install(TARGETS SimdCore_rvv
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
