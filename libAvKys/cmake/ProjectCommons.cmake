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
set(VER_MIN 1)
set(VER_PAT 0)
set(VERSION ${VER_MAJ}.${VER_MIN}.${VER_PAT})

set(DAILY_BUILD OFF CACHE BOOL "Mark this as a daily build")
set(NOALSA OFF CACHE BOOL "Disable ALSA support")
set(NODSHOW OFF CACHE BOOL "Disable DirectShow support")
set(NOFFMPEG OFF CACHE BOOL "Disable FFmpeg support")
set(NOFFMPEGSCREENCAP OFF CACHE BOOL "Disable FFmpeg screen capture support")
set(NOGSTREAMER OFF CACHE BOOL "Disable GStreamer support")
set(NOJACK OFF CACHE BOOL "Disable JACK support")
set(NOLIBAVDEVICE OFF CACHE BOOL "Disable libavdevice support in FFmpeg")
set(NOLIBUVC OFF CACHE BOOL "Disable libuvc  support")
set(NOMEDIAFOUNDATION OFF CACHE BOOL "Disable Microsoft Media Foundation support")
set(NONDKAUDIO OFF CACHE BOOL "Disable Android NDK Audio support")
set(NONDKCAMERA OFF CACHE BOOL "Disable Android NDK Camera support")
set(NONDKMEDIA OFF CACHE BOOL "Disable Android NDK Media support")
set(NOOPENSL OFF CACHE BOOL "Disable OpenSL ES support")
set(NOPIPEWIRE OFF CACHE BOOL "Disable Pipewire support")
set(NOPORTAUDIO OFF CACHE BOOL "Disable PortAudio support")
set(NOPULSEAUDIO OFF CACHE BOOL "Disable PulseAudio support")
set(NOQTCAMERA OFF CACHE BOOL "Disable video capture using QCamera support")
set(NOSDL OFF CACHE BOOL "Disable SDL support")
set(NOV4L2 OFF CACHE BOOL "Disable V4L2 support")
set(NOV4LUTILS OFF CACHE BOOL "Disable V4l-utils support")
set(NOVLC OFF CACHE BOOL "Disable VLC support")
set(NOWASAPI OFF CACHE BOOL "Disable WASAPI support")

if (APPLE)
    set(EXECPREFIX Webcamoid.app/Contents)
    set(BINDIR ${EXECPREFIX}/MacOS)
    set(LIBDIR ${EXECPREFIX}/Frameworks)
    set(PLUGINSDIR ${EXECPREFIX}/Plugins/avkys)
    set(DATAROOTDIR ${EXECPREFIX}/Resources)
    set(LICENSEDIR ${DATAROOTDIR})
    set(TRANSLATIONSDIR ${DATAROOTDIR}/translations)
    set(OUTPUT_VLC_PLUGINS_DIR ${EXECPREFIX}/Plugins/vlc)
    set(OUTPUT_GST_PLUGINS_DIR ${EXECPREFIX}/Plugins/gstreamer-1.0)
elseif (ANDROID)
    set(EXECPREFIX "")
    set(BINDIR libs/${CMAKE_ANDROID_ARCH_ABI})
    set(LIBDIR ${BINDIR})
    set(PLUGINSDIR ${BINDIR})
    set(DATAROOTDIR assets)
    set(LICENSEDIR ${DATAROOTDIR})
    set(TRANSLATIONSDIR ${DATAROOTDIR}/translations)
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
    set(TRANSLATIONSDIR ${DATAROOTDIR}/webcamoid/translations)
    set(OUTPUT_VLC_PLUGINS_DIR ${LIBDIR}/vlc/plugins)
    set(OUTPUT_GST_PLUGINS_DIR ${LIBDIR}/gstreamer-1.0)
    set(OUTPUT_PIPEWIRE_MODULES_DIR ${LIBDIR}/pipewire)
    set(OUTPUT_PIPEWIRE_SPA_PLUGINS_DIR ${LIBDIR}/pipewire-spa)
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

# Retrieve useful variables related to Qt installation.

find_program(GIT_BIN git)

if (GIT_BIN)
    execute_process(COMMAND ${GIT_BIN} rev-parse HEAD
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                    OUTPUT_VARIABLE GIT_COMMIT_HASH
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET)

    if (GIT_COMMIT_HASH)
        add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")
    endif ()
endif ()

set(ANDROID_JAVA_VERSION 1.6 CACHE STRING "Mimimum Java version to use in Android")
set(ANDROID_JAR_DIRECTORY ${ANDROID_SDK_ROOT}/platforms/android-${ANDROID_NATIVE_API_LEVEL} CACHE INTERNAL "")

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
set(PIPEWIRE_MODULES_PATH "${OUTPUT_PIPEWIRE_MODULES_DIR}" CACHE PATH "PipeWire modules search path")
set(PIPEWIRE_SPA_PLUGINS_PATH "${OUTPUT_PIPEWIRE_SPA_PLUGINS_DIR}" CACHE PATH "PipeWire SPA plugins search path")

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
set(QML_IMPORT_PATH "${CMAKE_SOURCE_DIR}/libAvKys/Lib/share/qml" CACHE STRING "additional libraries" FORCE)

# Try detecting the target architecture.

# NOTE for other developers: TARGET_ARCH is intended to be used as a reference
# for the deploy tool, so don't rush on adding new architectures unless you
# want to create a binary distributable for that architecture.
# Webcamoid build is not affected in anyway by the value of TARGET_ARCH, if the
# build fails its something else and totally unrelated to that variable.

if (WIN32)
    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles("
    #include <windows.h>

    #ifndef _M_X64
        #error Not WIN64
    #endif

    int main()
    {
        return 0;
    }" IS_WIN64_TARGET)

    check_cxx_source_compiles("
    #include <windows.h>

    #ifndef _M_IX86
        #error Not WIN32
    #endif

    int main()
    {
        return 0;
    }" IS_WIN32_TARGET)

    check_cxx_source_compiles("
    #include <windows.h>

    #ifndef _M_ARM64
        #error Not ARM64
    #endif

    int main()
    {
        return 0;
    }" IS_WIN64_ARM_TARGET)

    check_cxx_source_compiles("
    #include <windows.h>

    #ifndef _M_ARM
        #error Not ARM
    #endif

    int main()
    {
        return 0;
    }" IS_WIN32_ARM_TARGET)

    if (IS_WIN64_TARGET OR IS_WIN64_ARM_TARGET)
        set(QTIFW_TARGET_DIR "\@ApplicationsDirX64\@/Webcamoid")
    else ()
        set(QTIFW_TARGET_DIR "\@ApplicationsDirX86\@/Webcamoid")
    endif()

    if (IS_WIN64_TARGET)
        set(TARGET_ARCH win64)
    elseif (IS_WIN64_ARM_TARGET)
        set(TARGET_ARCH win64_arm)
    elseif (IS_WIN32_TARGET)
        set(TARGET_ARCH win32)
    elseif (IS_WIN32_ARM_TARGET)
        set(TARGET_ARCH win32_arm)
    else ()
        set(TARGET_ARCH unknown)
    endif()
elseif (UNIX)
    if (ANDROID)
        set(TARGET_ARCH ${CMAKE_ANDROID_ARCH_ABI})
    else ()
        include(CheckCXXSourceCompiles)
        check_cxx_source_compiles("
        #ifndef __x86_64__
            #error Not x64
        #endif

        int main()
        {
            return 0;
        }" IS_X86_64_TARGET)

        check_cxx_source_compiles("
        #ifndef __i386__
            #error Not x86
        #endif

        int main()
        {
            return 0;
        }" IS_I386_TARGET)

        check_cxx_source_compiles("
        #ifndef __aarch64__
            #error Not ARM64
        #endif

        int main()
        {
            return 0;
        }" IS_ARM64_TARGET)

        check_cxx_source_compiles("
        #ifndef __arm__
            #error Not ARM
        #endif

        int main()
        {
            return 0;
        }" IS_ARM_TARGET)

        check_cxx_source_compiles("
        #ifndef __riscv
            #error Not RISC-V
        #endif

        int main()
        {
            return 0;
        }" IS_RISCV_TARGET)

        if (IS_X86_64_TARGET)
            set(TARGET_ARCH x64)
        elseif (IS_I386_TARGET)
            set(TARGET_ARCH x86)
        elseif (IS_ARM64_TARGET)
            set(TARGET_ARCH arm64)
        elseif (IS_ARM_TARGET)
            set(TARGET_ARCH arm32)
        elseif (IS_RISCV_TARGET)
            set(TARGET_ARCH riscv)
        else ()
            set(TARGET_ARCH unknown)
        endif ()
    endif ()
endif ()
