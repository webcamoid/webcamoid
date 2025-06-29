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

project(SimdCore_sse LANGUAGES CXX)

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
    ../commons/simdcore.cpp
    ../commons/simdcore.h
    ../commons/plugin.cpp
    ../commons/plugin.h
    pspec.json)

if (BUILD_PROCESSOR_X86)
    qt_add_plugin(SimdCore_sse
                  SHARED
                  CLASS_NAME Plugin)
    target_sources(SimdCore_sse PRIVATE
                   ${SOURCES})
    enable_openmp(SimdCore_sse)
else ()
    add_library(SimdCore_sse EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(SimdCore_sse PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(SimdCore_sse PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(SimdCore_sse avkys)
target_include_directories(SimdCore_sse
                           PRIVATE
                           ../../../Lib/src)
target_compile_definitions(SimdCore_sse PRIVATE AVKYS_PLUGIN_SIMDCORE_SSE)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(SimdCore_sse
                      PUBLIC
                      ${QT_LIBS}
                      avkys)

add_definitions(-DAKSIMD_USE_SSE)

if (BUILD_PROCESSOR_X86)
    install(TARGETS SimdCore_sse
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
