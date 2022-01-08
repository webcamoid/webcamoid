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

set(QT_VERSION_MAJOR 5 CACHE STRING "Qt version to compile with")

if (QT_VERSION_MAJOR LESS_EQUAL 5)
    set(QT_MINIMUM_VERSION 5.15 CACHE INTERNAL "")
else ()
    set(QT_MINIMUM_VERSION 6.0 CACHE INTERNAL "")
endif ()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VER_MAJ 9)
set(VER_MIN 0)
set(VER_PAT 0)
set(VERSION ${VER_MAJ}.${VER_MIN}.${VER_PAT})

set(DAILY_BUILD OFF CACHE BOOL "Mark this as a daily build")
set(NOALSA OFF CACHE BOOL "Disable ALSA support")
set(NOAVFOUNDATION OFF CACHE BOOL "Disable AVFoundation support")
set(NOCOREMEDIAIO OFF CACHE BOOL "Disable CoreMediaIO support")
set(NOCOREAUDIO OFF CACHE BOOL "Disable Core Audio support")
set(NODSHOW OFF CACHE BOOL "Disable DirectShow support")
set(NOFFMPEG OFF CACHE BOOL "Disable FFmpeg support")
set(NOLIBAVDEVICE OFF CACHE BOOL "Disable libavdevice support in FFmpeg")
set(NOGSTREAMER OFF CACHE BOOL "Disable GStreamer support")
set(NOJACK OFF CACHE BOOL "Disable JACK support")
set(NOLIBUVC OFF CACHE BOOL "Disable libuvc  support")
set(NOMEDIAFOUNDATION OFF CACHE BOOL "Disable Microsoft Media Foundation support")
set(NONDKAUDIO OFF CACHE BOOL "Disable Android NDK Audio support")
set(NONDKCAMERA OFF CACHE BOOL "Disable Android NDK Camera support")
set(NONDKMEDIA OFF CACHE BOOL "Disable Android NDK Media support")
set(NOOPENSL OFF CACHE BOOL "Disable OpenSL ES support")
set(NOPULSEAUDIO OFF CACHE BOOL "Disable PulseAudio support")
set(NOV4L2 OFF CACHE BOOL "Disable V4L2 support")
set(NOV4LUTILS OFF CACHE BOOL "Disable V4l-utils support")
set(NOWASAPI OFF CACHE BOOL "Disable WASAPI support")

if (APPLE)
    set(EXECPREFIX Webcamoid.app/Contents)
    set(BINDIR ${EXECPREFIX}/MacOS)
    set(LIBDIR ${EXECPREFIX}/Frameworks)
    set(PLUGINSDIR ${EXECPREFIX}/Plugins/avkys)
    set(DATAROOTDIR ${EXECPREFIX}/Resources)
    set(LICENSEDIR ${DATAROOTDIR})
    set(OUTPUT_VLC_PLUGINS_DIR ${EXECPREFIX}/Plugins/vlc)
    set(OUTPUT_GST_PLUGINS_DIR ${EXECPREFIX}/Plugins/gstreamer-1.0)
elseif (ANDROID)
    set(EXECPREFIX "")
    set(BINDIR libs/${CMAKE_ANDROID_ARCH_ABI})
    set(LIBDIR ${BINDIR})
    set(PLUGINSDIR ${BINDIR})
    set(DATAROOTDIR assets)
    set(LICENSEDIR ${DATAROOTDIR})
    set(OUTPUT_VLC_PLUGINS_DIR ${LIBDIR})
    set(OUTPUT_GST_PLUGINS_DIR ${LIBDIR})
else ()
    include(GNUInstallDirs)

    set(EXECPREFIX "")
    set(BINDIR ${CMAKE_INSTALL_BINDIR})
    set(LIBDIR ${CMAKE_INSTALL_LIBDIR})
    set(PLUGINSDIR ${CMAKE_INSTALL_LIBDIR}/avkys
        CACHE FILEPATH "Plugins install directory")
    set(DATAROOTDIR ${CMAKE_INSTALL_DATAROOTDIR})
    set(LICENSEDIR ${DATAROOTDIR}/licenses/webcamoid)
    set(OUTPUT_VLC_PLUGINS_DIR ${LIBDIR}/vlc/plugins)
    set(OUTPUT_GST_PLUGINS_DIR ${LIBDIR}/gstreamer-1.0)
endif ()

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
add_definitions(-DQT_DEPRECATED_WARNINGS)

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x060000) # disables all the APIs deprecated before Qt 6.0.0

if (DAILY_BUILD)
    add_definitions(-DDAILY_BUILD)
endif ()

set(ANDROID_JAVA_VERSION 1.6 CACHE STRING "Mimimum Java version to use in Android")
set(ANDROID_JAR_DIRECTORY ${ANDROID_SDK}/platforms/android-${ANDROID_NATIVE_API_LEVEL} CACHE INTERNAL "")

# Force prefix and suffix. This fix broken MinGW builds in CI environments.
if (WIN32 AND NOT MSVC)
    set(CMAKE_STATIC_LIBRARY_PREFIX "lib")
    set(CMAKE_STATIC_LIBRARY_SUFFIX ".a")
    set(CMAKE_SHARED_LIBRARY_PREFIX "lib")
    set(CMAKE_IMPORT_LIBRARY_PREFIX "lib")
    set(CMAKE_IMPORT_LIBRARY_SUFFIX ".dll.a")
    set(CMAKE_LINK_LIBRARY_SUFFIX "")
endif ()

set(VLC_PLUGINS_PATH "${OUTPUT_VLC_PLUGINS_DIR}" CACHE PATH "VLC plugins search path")
set(GST_PLUGINS_PATH "${OUTPUT_GST_PLUGINS_DIR}" CACHE PATH "GStreamer plugins search path")

# Guess gst-plugin-scanner path.

find_package(PkgConfig)
pkg_check_modules(GSTREAMER_PKG gstreamer-1.0)

if (GSTREAMER_PKG_FOUND)
    pkg_get_variable(GST_PLUGINS_SCANNER_DIR gstreamer-1.0 pluginscannerdir)

    if (NOT "${GST_PLUGINS_SCANNER_DIR}" STREQUAL "")
        file(GLOB GST_PLUGINS_SCANNER
             ${GST_PLUGINS_SCANNER_DIR}/gst-plugin-scanner*)
    endif ()
endif ()

set(GST_PLUGINS_SCANNER_PATH "${GST_PLUGINS_SCANNER}" CACHE FILEPATH "GStreamer plugins scanner utility path")

# Sudoer tool search directory

set(EXTRA_SUDOER_TOOL_DIR "" CACHE PATH "Additional sudoer tool search directory")
