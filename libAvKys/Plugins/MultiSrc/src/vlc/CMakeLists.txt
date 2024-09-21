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

project(MultiSrc_vlc LANGUAGES CXX)

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

set(SOURCES
    ../mediasource.cpp
    ../mediasource.h
    src/mediasourcevlc.cpp
    src/mediasourcevlc.h
    src/plugin.cpp
    src/plugin.h
    pspec.json)

set(VLC_INSTALL_PATH "" CACHE PATH "The path where VLC is installed")
pkg_check_modules(LIBVLC libvlc)

if (APPLE AND NOT LIBVLC_FOUND)
    message("-- Checking for 'VLC'")

    # Check user defined VLC path first.

    if (NOT "${VLC_INSTALL_PATH}" STREQUAL "" AND EXISTS "${VLC_INSTALL_PATH}")
        set(VLCAPP_DIR "${VLCAPP_INSTALL_DIR}")
    endif()

    # Check VLC installed by homebrew.

    if ("${VLCAPP_DIR}" STREQUAL "")
        set(VLCAPPBIN "/usr/local/bin/vlc")

        if (EXISTS ${VLCAPPBIN})
            get_filename_component(VLCAPPBIN_PATH ${VLCAPPBIN} REALPATH)
            get_filename_component(VLCAPP_INSTALL_DIR ${VLCAPPBIN_PATH} DIRECTORY)
            set(VLCAPP_DIR "${VLCAPP_INSTALL_DIR}/VLC.app")
        endif()
    endif()

    # Check VLC manually installed by the user.

    if ("${VLCAPP_DIR}" STREQUAL "")
        set(VLCAPPDIR "/Applications/VLC.app")

        if (EXISTS "${VLCAPPDIR}")
            set(VLCAPP_DIR "${VLCAPPDIR}")
        endif()
    endif()

    if (NOT "${VLCAPP_DIR}" STREQUAL "")
        set(VLCAPP_INCLUDE_DIRS "${VLCAPP_DIR}/Contents/MacOS/include")
        set(VLCAPP_LIBRARY_DIRS "${VLCAPP_DIR}/Contents/MacOS/lib")

        if (EXISTS ${VLCAPP_INCLUDE_DIRS} AND EXISTS ${VLCAPP_LIBRARY_DIRS})
            message("--   Found VLC in '${VLCAPP_DIR}'")
            set(VLCAPP_FOUND TRUE)
            set(VLCAPP_LIBRARIES vlc vlccore)
        else ()
            message("--   No 'VLC' found")
        endif ()
    else ()
        message("--   No 'VLC' found")
    endif ()
endif ()

add_definitions(-DBINDIR="${BINDIR}"
                -DVLC_INSTALL_PATH="${VLC_INSTALL_PATH}"
                -DVLC_PLUGINS_PATH="${VLC_PLUGINS_PATH}")

if (NOT NOVLC AND (LIBVLC_FOUND OR VLCAPP_FOUND))
    qt_add_plugin(MultiSrc_vlc
                  SHARED
                  CLASS_NAME Plugin)
    target_sources(MultiSrc_vlc PRIVATE
                   ${SOURCES})
else ()
    add_library(MultiSrc_vlc EXCLUDE_FROM_ALL ${SOURCES})
endif ()

set_target_properties(MultiSrc_vlc PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${AKPLUGINSDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(MultiSrc_vlc PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(MultiSrc_vlc avkys)
target_include_directories(MultiSrc_vlc
                           PUBLIC
                           ${LIBVLC_INCLUDE_DIRS}
                           ${VLCAPP_INCLUDE_DIRS}
                           PRIVATE
                           ..
                           ../../../../Lib/src)
target_compile_definitions(MultiSrc_vlc PRIVATE AVKYS_PLUGIN_MULTISRC_LIBVLC)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_directories(MultiSrc_vlc
                        PUBLIC
                        ${LIBVLC_LIBRARY_DIRS}
                        ${VLCAPP_LIBRARY_DIRS})
target_link_libraries(MultiSrc_vlc
                      ${QT_LIBS}
                      ${LIBVLC_LIBRARIES}
                      ${VLCAPP_LIBRARIES}
                      avkys)

if (NOT NOVLC AND (LIBVLC_FOUND OR VLCAPP_FOUND))
    install(TARGETS MultiSrc_vlc
            LIBRARY DESTINATION ${AKPLUGINSDIR}
            RUNTIME DESTINATION ${AKPLUGINSDIR})
endif ()
