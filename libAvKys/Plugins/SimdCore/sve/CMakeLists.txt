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

project(SimdCore_sve LANGUAGES CXX)

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

# Check if SVE is supported

include(CheckCXXCompilerFlag)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    check_cxx_compiler_flag("-march=armv8-a+sve" COMPILER_SUPPORTS_SVE)

    if (COMPILER_SUPPORTS_SVE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a+sve")
    endif()
endif ()

if (BUILD_PROCESSOR_ARM AND COMPILER_SUPPORTS_SVE AND NOT NOSIMDSVE)
    qt_add_plugin(SimdCore_sve
                  SHARED
                  CLASS_NAME Plugin)
    target_sources(SimdCore_sve PRIVATE
                   ${SOURCES})
    enable_openmp(SimdCore_sve)
else ()
    add_library(SimdCore_sve EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(SimdCore_sve PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(SimdCore_sve PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(SimdCore_sve avkys)
target_include_directories(SimdCore_sve
                           PRIVATE
                           ../../../Lib/src)
target_compile_definitions(SimdCore_sve PRIVATE AVKYS_PLUGIN_SIMDCORE_SVE)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(SimdCore_sve
                      PUBLIC
                      ${QT_LIBS}
                      avkys)

add_definitions(-DAKSIMD_USE_SVE)

if (BUILD_PROCESSOR_ARM AND COMPILER_SUPPORTS_SVE AND NOT NOSIMDSVE)
    install(TARGETS SimdCore_sve
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
