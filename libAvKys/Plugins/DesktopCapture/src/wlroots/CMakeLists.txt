# Webcamoid, camera capture application.
# Copyright (C) 2026  Gonzalo Exequiel Pedone
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

project(DesktopCapture_wlroots LANGUAGES CXX)

include(../../../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Concurrent
    Widgets)

find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(PkgConfig)

# Find Wayland client library
pkg_check_modules(WAYLAND_CLIENT wayland-client)
pkg_check_modules(WAYLAND_SCANNER wayland-scanner)

# Allow user to specify an alternative path for wayland-protocols
# Usage: cmake -DWAYLAND_PROTOCOLS_DIR=/custom/path/to/protocols ..
if (NOT DEFINED WAYLAND_PROTOCOLS_DIR)
    # Try pkg-config first for standard paths
    if (PkgConfig_FOUND)
        pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
    endif()

    if (NOT WAYLAND_PROTOCOLS_DIR)
        set(WAYLAND_PROTOCOLS_DIR /usr/share/wayland-protocols)
    endif()
endif()

# Generate protocol bindings from XML
set(PROTOCOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/share/protocols)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(PROTOCOLS
    wlr-screencopy-unstable-v1
    ext-image-capture-source-v1
    ext-image-copy-capture-v1
    xdg-output-unstable-v1
    ext-foreign-toplevel-list-v1)

set(GENERATED_SOURCES)
set(GENERATED_HEADERS)

if (WAYLAND_SCANNER_FOUND)
    # Find wayland-scanner executable
    find_program(WAYLAND_SCANNER_EXE wayland-scanner)

    if (WAYLAND_SCANNER_EXE)
        foreach(PROTO ${PROTOCOLS})
            # Try local protocol first, then user-specified, then system
            set(PROTO_XML ${PROTOCOLS_DIR}/${PROTO}.xml)

            if (NOT EXISTS ${PROTO_XML})
                # Try user-specified wayland-protocols dir
                find_file(USER_PROTO_XML
                          NAMES ${PROTO}.xml
                          PATHS ${WAYLAND_PROTOCOLS_DIR}
                                ${WAYLAND_PROTOCOLS_DIR}/share/wayland-protocols
                          PATH_SUFFIXES unstable/wlr-screencopy
                                        staging/ext-image-capture-source
                                        staging/ext-image-copy-capture
                                        unstable/xdg-output
                                        staging/ext-foreign-toplevel-list
                          NO_DEFAULT_PATH)

                if (USER_PROTO_XML)
                    set(PROTO_XML ${USER_PROTO_XML})
                endif()
            endif()

            if (NOT EXISTS ${PROTO_XML})
                # Try system wayland-protocols (fallback)
                find_file(SYSTEM_PROTO_XML
                          NAMES ${PROTO}.xml
                          PATHS /usr/share/wayland-protocols
                                /usr/local/share/wayland-protocols
                                ${WAYLAND_PROTOCOLS_DIR}
                                ${WAYLAND_PROTOCOLS_DIR}/share/wayland-protocols
                          PATH_SUFFIXES unstable/wlr-screencopy
                                        staging/ext-image-capture-source
                                        staging/ext-image-copy-capture
                                        unstable/xdg-output
                                        staging/ext-foreign-toplevel-list
                          NO_DEFAULT_PATH)

                if (SYSTEM_PROTO_XML)
                    set(PROTO_XML ${SYSTEM_PROTO_XML})
                endif()
            endif()

            if (EXISTS ${PROTO_XML})
                string(REPLACE "-" "_" PROTO_SAFE ${PROTO})
                set(PROTO_HEADER ${GENERATED_DIR}/${PROTO}-client-protocol.h)
                set(PROTO_CODE ${GENERATED_DIR}/${PROTO}-client-protocol.c)

                add_custom_command(
                    OUTPUT ${PROTO_HEADER}
                    COMMAND ${WAYLAND_SCANNER_EXE} client-header
                            ${PROTO_XML} ${PROTO_HEADER}
                    DEPENDS ${PROTO_XML}
                    COMMENT "Generating ${PROTO} client header")

                add_custom_command(
                    OUTPUT ${PROTO_CODE}
                    COMMAND ${WAYLAND_SCANNER_EXE} private-code
                            ${PROTO_XML} ${PROTO_CODE}
                    DEPENDS ${PROTO_XML}
                    COMMENT "Generating ${PROTO} client code")

                list(APPEND GENERATED_HEADERS ${PROTO_HEADER})
                list(APPEND GENERATED_SOURCES ${PROTO_CODE})
            else()
                message(WARNING "Protocol ${PROTO} not found")
            endif()
        endforeach()
    endif()
endif()

set(SOURCES
    ../screendev.cpp
    ../screendev.h
    src/plugin.cpp
    src/plugin.h
    src/wlrootsdev.cpp
    src/wlrootsdev.h
    ${GENERATED_HEADERS}
    ${GENERATED_SOURCES}
    pspec.json)

if (WAYLAND_CLIENT_FOUND
    AND NOT ANDROID
    AND NOT (APPLE OR FAKE_APPLE)
    AND NOT WIN32
    AND NOT NOWLROOTS
    AND NOT NOSCREENCAPTURE)
    qt_add_plugin(DesktopCapture_wlroots
                  SHARED
                  CLASS_NAME Plugin)
    target_sources(DesktopCapture_wlroots PRIVATE
                   ${SOURCES})
else ()
    add_library(DesktopCapture_wlroots EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(DesktopCapture_wlroots PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(DesktopCapture_wlroots PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(DesktopCapture_wlroots avkys)
target_include_directories(DesktopCapture_wlroots PRIVATE
                           PUBLIC
                           ${WAYLAND_CLIENT_INCLUDE_DIRS}
                           ${GENERATED_DIR}
                           PRIVATE
                           ..
                           ../../../../Lib/src)
target_compile_definitions(DesktopCapture_wlroots PRIVATE AVKYS_PLUGIN_DESKTOPCAPTURE_WLROOTS)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_directories(DesktopCapture_wlroots
                        PUBLIC
                        ${WAYLAND_CLIENT_LIBRARY_DIRS})
target_link_libraries(DesktopCapture_wlroots
                      ${QT_LIBS}
                      avkys
                      ${WAYLAND_CLIENT_LIBRARIES})

if (WAYLAND_CLIENT_FOUND
    AND NOT ANDROID
    AND NOT (APPLE OR FAKE_APPLE)
    AND NOT WIN32
    AND NOT NOWLROOTS
    AND NOT NOSCREENCAPTURE)
    install(TARGETS DesktopCapture_wlroots
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
