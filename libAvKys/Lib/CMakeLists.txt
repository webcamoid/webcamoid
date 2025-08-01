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
qt_add_library(avkys
               src/ak.cpp
               src/ak.h
               src/akalgorithm.h
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
               src/akcolorconvert.cpp
               src/akcolorconvert.h
               src/akcolorplane.cpp
               src/akcolorplane.h
               src/akcommons.h
               src/akcompressedaudiocaps.cpp
               src/akcompressedaudiocaps.h
               src/akcompressedaudiopacket.cpp
               src/akcompressedaudiopacket.h
               src/akcompressedcaps.cpp
               src/akcompressedcaps.h
               src/akcompressedpacket.cpp
               src/akcompressedpacket.h
               src/akcompressedvideocaps.cpp
               src/akcompressedvideocaps.h
               src/akcompressedvideopacket.cpp
               src/akcompressedvideopacket.h
               src/akcpufeatures.cpp
               src/akcpufeatures.h
               src/akfrac.cpp
               src/akfrac.h
               src/akmenuoption.cpp
               src/akmenuoption.h
               src/akpacket.cpp
               src/akpacket.h
               src/akpacketbase.cpp
               src/akpacketbase.h
               src/akplugininfo.cpp
               src/akplugininfo.h
               src/akpluginmanager.cpp
               src/akpluginmanager.h
               src/akpropertyoption.cpp
               src/akpropertyoption.h
               src/aksimd.h
               src/aksimd.cpp
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
               src/iak/akaudioencoder.cpp
               src/iak/akaudioencoder.h
               src/iak/akelement.cpp
               src/iak/akelement.h
               src/iak/akmultimediasourceelement.cpp
               src/iak/akmultimediasourceelement.h
               src/iak/akplugin.h
               src/iak/aksimdoptimizations.h
               src/iak/akvideoencoder.cpp
               src/iak/akvideoencoder.h
               src/iak/akvideomuxer.cpp
               src/iak/akvideomuxer.h
               src/qml/akcolorizedimage.cpp
               src/qml/akcolorizedimage.h
               src/qml/akfontsettings.cpp
               src/qml/akfontsettings.h
               src/qml/akpalette.cpp
               src/qml/akpalette.h
               src/qml/akpalettegroup.cpp
               src/qml/akpalettegroup.h
               src/qml/aktheme.cpp
               src/qml/aktheme.h
               src/qml/akutils.cpp
               src/qml/akutils.h
               src/simd/akavx.h
               src/simd/akavx2.h
               src/simd/akmmx.h
               src/simd/akneon.h
               src/simd/akrvv.h
               src/simd/akscalar.h
               src/simd/aksse.h
               src/simd/aksse2.h
               src/simd/aksse4_1.h
               src/simd/aksve.h
               AvKys.qrc)

enable_openmp(avkys)

if (UNIX AND NOT ANDROID AND NOT (APPLE OR FAKE_APPLE))
    set_target_properties(avkys
                          PROPERTIES
                          VERSION ${VERSION}
                          SOVERSION ${VER_MAJ})
endif ()

set_target_properties(avkys PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${LIBDIR}
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR})

if (IPO_IS_SUPPORTED)
    set_target_properties(avkys PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

target_compile_definitions(avkys PRIVATE AVKYS_LIBRARY)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(avkys PRIVATE ${QT_LIBS})
add_definitions(-DAKCOMMONS_LIBRARY
                -DBINDIR="${BINDIR}"
                -DAKPLUGINSDIR="${AKPLUGINSDIR}"
                -DEXTENSION_SHLIB="${CMAKE_SHARED_LIBRARY_SUFFIX}"
                -DPREFIX_SHLIB="${CMAKE_SHARED_LIBRARY_PREFIX}"
                -DPLATFORM_TARGET_SUFFIX="${CMAKE_ANDROID_ARCH_ABI}")

install(TARGETS avkys
        LIBRARY DESTINATION ${LIBDIR}
        RUNTIME DESTINATION ${BINDIR})
