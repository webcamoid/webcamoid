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
            src/ak.h
            src/akaudiocaps.h
            src/akaudiopacket.h
            src/akcaps.h
            src/akcommons.h
            src/akelement.h
            src/akfrac.h
            src/akmultimediasourceelement.h
            src/akpacket.h
            src/akplugin.h
            src/akunit.h
            src/akvideocaps.h
            src/akvideopacket.h
            src/qml/akcolorizedimage.h
            src/qml/akpalette.h
            src/qml/akpalettegroup.h
            src/qml/aktheme.h
            src/ak.cpp
            src/akaudiocaps.cpp
            src/akaudiopacket.cpp
            src/akcaps.cpp
            src/akelement.cpp
            src/akfrac.cpp
            src/akmultimediasourceelement.cpp
            src/akpacket.cpp
            src/akunit.cpp
            src/akvideocaps.cpp
            src/akvideopacket.cpp
            src/qml/akcolorizedimage.cpp
            src/qml/akpalette.cpp
            src/qml/akpalettegroup.cpp
            src/qml/aktheme.cpp
            AvKys.qrc)

if (UNIX AND NOT ANDROID AND NOT APPLE)
    set_target_properties(avkys
                          PROPERTIES
                          VERSION ${VERSION}
                          SOVERSION ${VER_MAJ})
endif ()

target_compile_definitions(avkys PRIVATE AVKYS_LIBRARY)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(avkys PRIVATE ${QT_LIBS})
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
