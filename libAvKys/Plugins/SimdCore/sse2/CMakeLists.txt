# Webcamoid, camera capture application.
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

project(SimdCore_sse2 LANGUAGES CXX)

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

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    check_cxx_compiler_flag("/arch:SSE2" COMPILER_SUPPORTS_SSE2)

    if (COMPILER_SUPPORTS_SSE2)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
    endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(COMPILER_SUPPORTS_SSE2 TRUE)
endif()

if (BUILD_PROCESSOR_X86 AND COMPILER_SUPPORTS_SSE2 AND NOT NOSIMDSSE2)
    qt_add_plugin(SimdCore_sse2
                  SHARED
                  CLASS_NAME Plugin)
    target_sources(SimdCore_sse2 PRIVATE
                   ${SOURCES})
    enable_openmp(SimdCore_sse2)
else ()
    add_library(SimdCore_sse2 EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(SimdCore_sse2 PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(SimdCore_sse2 PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(SimdCore_sse2 avkys)
target_include_directories(SimdCore_sse2
                           PRIVATE
                           ../../../Lib/src)
target_compile_definitions(SimdCore_sse2 PRIVATE AVKYS_PLUGIN_SIMDCORE_SSE2)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(SimdCore_sse2
                      PUBLIC
                      ${QT_LIBS}
                      avkys)

add_definitions(-DAKSIMD_USE_SSE2)

# Check if SSE2 is supported

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND CMAKE_SYSTEM_PROCESSOR MATCHES "i[3456]86")
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-msse2" COMPILER_SUPPORTS_SSE2)

    if (COMPILER_SUPPORTS_SSE2)
        target_compile_options(SimdCore_sse2 PRIVATE -msse2)
    endif()
endif ()

if (BUILD_PROCESSOR_X86 AND COMPILER_SUPPORTS_SSE2 AND NOT NOSIMDSSE2)
    install(TARGETS SimdCore_sse2
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
