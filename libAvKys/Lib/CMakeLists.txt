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

project(avkys LANGUAGES CXX)

include(../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Quick
    Widgets)
find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
add_library(avkys SHARED
            src/ak.cpp
            src/ak.h
            src/akaudiocaps.cpp
            src/akaudiocaps.h
            src/akaudioconverter.cpp
            src/akaudioconverter.h
            src/akaudiopacket.cpp
            src/akaudiopacket.h
            src/akcaps.cpp
            src/akcaps.h
            src/akcolorcomponent.cpp
            src/akcolorcomponent.h
            src/akcolorconvert.h
            src/akcolorconvert.cpp
            src/akcolorplane.cpp
            src/akcolorplane.h
            src/akcommons.h
            src/akcompressedvideocaps.cpp
            src/akcompressedvideocaps.h
            src/akcompressedvideopacket.cpp
            src/akcompressedvideopacket.h
            src/akelement.cpp
            src/akelement.h
            src/akfrac.cpp
            src/akfrac.h
            src/akmultimediasourceelement.cpp
            src/akmultimediasourceelement.h
            src/akpacket.cpp
            src/akpacket.h
            src/akpacketbase.cpp
            src/akpacketbase.h
            src/akplugin.h
            src/akplugininfo.cpp
            src/akplugininfo.h
            src/akpluginmanager.cpp
            src/akpluginmanager.h
            src/aksubtitlecaps.cpp
            src/aksubtitlecaps.h
            src/aksubtitlepacket.cpp
            src/aksubtitlepacket.h
            src/akunit.cpp
            src/akunit.h
            src/akvideocaps.cpp
            src/akvideocaps.h
            src/akvideoconverter.cpp
            src/akvideoconverter.h
            src/akvideoformatspec.cpp
            src/akvideoformatspec.h
            src/akvideomixer.cpp
            src/akvideomixer.h
            src/akvideopacket.cpp
            src/akvideopacket.h
            src/qml/akcolorizedimage.cpp
            src/qml/akcolorizedimage.h
            src/qml/akpalette.cpp
            src/qml/akpalette.h
            src/qml/akpalettegroup.cpp
            src/qml/akpalettegroup.h
            src/qml/aktheme.cpp
            src/qml/aktheme.h
            src/qml/akutils.cpp
            src/qml/akutils.h
            AvKys.qrc)

if (UNIX AND NOT ANDROID AND NOT APPLE)
    set_target_properties(avkys
                          PROPERTIES
                          VERSION ${VERSION}
                          SOVERSION ${VER_MAJ})
endif ()

if (WIN32)
    set_target_properties(avkys PROPERTIES
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${BINDIR})
elseif (NOT ANDROID)
    set_target_properties(avkys PROPERTIES
                          LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${LIBDIR})
endif ()

target_compile_definitions(avkys PRIVATE AVKYS_LIBRARY)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(avkys ${QT_LIBS})
add_definitions(-DAKCOMMONS_LIBRARY
                -DBINDIR="${BINDIR}"
                -DPLUGINSDIR="${PLUGINSDIR}"
                -DEXTENSION_SHLIB="${CMAKE_SHARED_LIBRARY_SUFFIX}"
                -DPREFIX_SHLIB="${CMAKE_SHARED_LIBRARY_PREFIX}"
                -DPLATFORM_TARGET_SUFFIX="${CMAKE_ANDROID_ARCH_ABI}")

if (WIN32)
    install(TARGETS avkys RUNTIME DESTINATION ${BINDIR})
elseif (NOT ANDROID)
    install(TARGETS avkys DESTINATION ${LIBDIR})
endif ()
