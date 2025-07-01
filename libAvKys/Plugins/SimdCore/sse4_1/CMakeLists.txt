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

project(SimdCore_sse4_1 LANGUAGES CXX)

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

# Check if AVX2 is supported

include(CheckCXXCompilerFlag)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    check_cxx_compiler_flag("/arch:SSE4.2" COMPILER_SUPPORTS_SSE4_1)

    if (COMPILER_SUPPORTS_SSE4_1)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE4.2")
    endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    check_cxx_compiler_flag("-msse4.1" COMPILER_SUPPORTS_SSE4_1)

    if (COMPILER_SUPPORTS_SSE4_1)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")
    endif()
endif()

if (BUILD_PROCESSOR_X86 AND COMPILER_SUPPORTS_SSE4_1 AND NOT NOSIMDSSE4_1)
    qt_add_plugin(SimdCore_sse4_1
                  SHARED
                  CLASS_NAME Plugin)
    target_sources(SimdCore_sse4_1 PRIVATE
                   ${SOURCES})
    enable_openmp(SimdCore_sse4_1)
else ()
    add_library(SimdCore_sse4_1 EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(SimdCore_sse4_1 PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(SimdCore_sse4_1 PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(SimdCore_sse4_1 avkys)
target_include_directories(SimdCore_sse4_1
                           PRIVATE
                           ../../../Lib/src)
target_compile_definitions(SimdCore_sse4_1 PRIVATE AVKYS_PLUGIN_SIMDCORE_SSE4_1)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(SimdCore_sse4_1
                      PUBLIC
                      ${QT_LIBS}
                      avkys)

add_definitions(-DAKSIMD_USE_SSE4_1)

if (BUILD_PROCESSOR_X86 AND COMPILER_SUPPORTS_SSE4_1 AND NOT NOSIMDSSE4_1)
    install(TARGETS SimdCore_sse4_1
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
