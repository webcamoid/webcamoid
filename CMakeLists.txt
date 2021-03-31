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

project(Webcamoid)

include(libAvKys/cmake/ProjectCommons.cmake)

if (ANDROID)
    find_package(Qt5 COMPONENTS Core REQUIRED)
endif ()

add_subdirectory(libAvKys)
add_subdirectory(StandAlone)

if (APPLE)
    set(TARGET_PLATFORM mac)
    set(BUILD_INFO_FILE AkVirtualCamera.plugin/Contents/Resources/build-info.txt)
    set(APP_LIBDIR AkVirtualCamera.plugin/Contents/Frameworks)
    set(MAIN_EXECUTABLE AkVirtualCamera.plugin/Contents/MacOS/AkVirtualCamera)
    set(QTIFW_TARGET_DIR "\@ApplicationsDir\@/AkVirtualCamera")
elseif (WIN32)
    set(TARGET_PLATFORM windows)
    set(BUILD_INFO_FILE share/build-info.txt)
    set(MAIN_EXECUTABLE ${TARGET_ARCH}/AkVCamManager.exe)
    set(APP_LIBDIR ${TARGET_ARCH})

    if (IS_64BITS_TARGET)
        set(QTIFW_TARGET_DIR "\@ApplicationsDirX64\@/AkVirtualCamera")
    else ()
        set(QTIFW_TARGET_DIR "\@ApplicationsDirX86\@/AkVirtualCamera")
    endif()
endif ()

configure_file(package_info.conf.in package_info.conf)

if (ANDROID)
    configure_file(COPYING
                   "${CMAKE_BINARY_DIR}/android-build/${LICENSEDIR}/COPYING"
                   COPYONLY)
else ()
    install(FILES COPYING DESTINATION ${LICENSEDIR})
endif ()

if (UNIX AND NOT ANDROID AND NOT MACX)
    install(FILES webcamoid.desktop DESTINATION ${DATAROOTDIR}/applications)
endif ()
