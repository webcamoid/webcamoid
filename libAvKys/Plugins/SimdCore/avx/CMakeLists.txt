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

project(SimdCore_avx LANGUAGES CXX)

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
    check_cxx_compiler_flag("/arch:AVX" COMPILER_SUPPORTS_AVX)

    if (COMPILER_SUPPORTS_AVX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX")
    endif()
else ()
    check_cxx_compiler_flag("-mavx" COMPILER_SUPPORTS_AVX)

    if (COMPILER_SUPPORTS_AVX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx")
    endif()
endif()

if (BUILD_PROCESSOR_X86 AND COMPILER_SUPPORTS_AVX)
    qt_add_plugin(SimdCore_avx
                  SHARED
                  CLASS_NAME Plugin)
    target_sources(SimdCore_avx PRIVATE
                   ${SOURCES})
    enable_openmp(SimdCore_avx)
else ()
    add_library(SimdCore_avx EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(SimdCore_avx PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(SimdCore_avx PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(SimdCore_avx avkys)
target_include_directories(SimdCore_avx
                           PRIVATE
                           ../../../Lib/src)
target_compile_definitions(SimdCore_avx PRIVATE AVKYS_PLUGIN_SIMDCORE_AVX)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(SimdCore_avx
                      PUBLIC
                      ${QT_LIBS}
                      avkys)

add_definitions(-DAKSIMD_USE_AVX)

if (BUILD_PROCESSOR_X86 AND COMPILER_SUPPORTS_AVX)
    install(TARGETS SimdCore_avx
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
