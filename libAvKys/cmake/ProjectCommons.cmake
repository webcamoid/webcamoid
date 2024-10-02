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

set(QT_VERSION_MAJOR 6 CACHE STRING "Qt version to compile with")
set(QT_MINIMUM_VERSION 6.2 CACHE INTERNAL "")

if (APPLE)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)
else ()
    set(CMAKE_CXX_STANDARD 11)
endif ()

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VER_MAJ 9)
set(VER_MIN 2)
set(VER_PAT 0)
set(VERSION ${VER_MAJ}.${VER_MIN}.${VER_PAT})

set(DAILY_BUILD OFF CACHE BOOL "Mark this as a daily build")
set(STATIC_BUILD OFF CACHE BOOL "Build Webcamoid statically")
set(STATIC_LIBGCC OFF CACHE BOOL "Build with static GCC libraries")
set(ORGANIZATION_IDENTIFIER "io.github.webcamoid" CACHE STRING "Organization identifier")
set(APP_IDENTIFIER "${ORGANIZATION_IDENTIFIER}.Webcamoid" CACHE STRING "Application identifier")
set(WITH_FLATPAK_VCAM ON CACHE BOOL "Enable support for the virtual camera in Flatpak")
set(ANDROID_OPENSSL_SUFFIX "_3" CACHE STRING "Set OpenSSL libraries suffix")
set(ENABLE_ANDROID_DEBUGGING OFF CACHE BOOL "Enable debugging logs in Android")
set(ENABLE_IPO OFF CACHE BOOL "Enable interprocedural optimization")
set(NOALSA OFF CACHE BOOL "Disable ALSA support")
set(NODSHOW OFF CACHE BOOL "Disable DirectShow support")
set(NOFFMPEG OFF CACHE BOOL "Disable FFmpeg support")
set(NOFFMPEGSCREENCAP OFF CACHE BOOL "Disable FFmpeg screen capture support")
set(NOGSTREAMER OFF CACHE BOOL "Disable GStreamer support")
set(NOJACK OFF CACHE BOOL "Disable JACK support")
set(NOLIBAVDEVICE OFF CACHE BOOL "Disable libavdevice support in FFmpeg")
set(NOLIBUSB OFF CACHE BOOL "Disable libusb  support")
set(NOLIBUVC OFF CACHE BOOL "Disable libuvc  support")
set(NOMEDIAFOUNDATION OFF CACHE BOOL "Disable Microsoft Media Foundation support")
set(NONDKMEDIA OFF CACHE BOOL "Disable Android NDK Media support")
set(NOPIPEWIRE OFF CACHE BOOL "Disable Pipewire support")
set(NOPORTAUDIO OFF CACHE BOOL "Disable PortAudio support")
set(NOPULSEAUDIO OFF CACHE BOOL "Disable PulseAudio support")
set(NOQTCAMERA OFF CACHE BOOL "Disable video capture using QCamera support")
set(NOQTSCREENCAPTURE OFF CACHE BOOL "Disable screen capture using QScreenCapture")
set(NOSCREENCAPTURE OFF CACHE BOOL "Disable screen capture")
set(NOSDL OFF CACHE BOOL "Disable SDL support")
set(NOV4L2 OFF CACHE BOOL "Disable V4L2 support")
set(NOV4LUTILS OFF CACHE BOOL "Disable V4l-utils support")
set(NOVIDEOEFFECTS OFF CACHE BOOL "No build video effects")
set(NOVLC OFF CACHE BOOL "Disable VLC support")
set(NOWASAPI OFF CACHE BOOL "Disable WASAPI support")
set(NOXLIBSCREENCAP OFF CACHE BOOL "Disable screen capture using Xlib")

# Ads configurations

set(ENABLE_ANDROID_ADS OFF CACHE BOOL "Enable Android AdMob")

# Set testing ads units as defaults

set(ANDROID_AD_APPID "ca-app-pub-3940256099942544~3347511713" CACHE STRING "Android application ID")
set(ANDROID_AD_UNIT_ID_APP_OPEN "ca-app-pub-3940256099942544/9257395921" CACHE STRING "Android app open unit ID")
set(ANDROID_AD_UNIT_ID_BANNER "ca-app-pub-3940256099942544/6300978111" CACHE STRING "Android banner unit ID")
set(ANDROID_AD_UNIT_ID_ADAPTIVE_BANNER "ca-app-pub-3940256099942544/9214589741" CACHE STRING "Android adaptive banner unit ID")
set(ANDROID_AD_UNIT_ID_ADAPTIVE_INTERSTITIAL "ca-app-pub-3940256099942544/1033173712" CACHE STRING "Android adaptive interstitial unit ID")
set(ANDROID_AD_UNIT_ID_ADAPTIVE_REWARDED "ca-app-pub-3940256099942544/5224354917" CACHE STRING "Android adaptive rewarded unit ID")
set(ANDROID_AD_UNIT_ID_ADAPTIVE_REWARDED_INTERSTITIAL "ca-app-pub-3940256099942544/5354046379" CACHE STRING "Android adaptive rewarded interstitial unit ID")

set(GOOGLE_PLAY_SERVICES_ADS_VERSION "23.3.0" CACHE STRING "com.google.android.gms:play-services-ads-lite version")

find_program(MVN_BIN mvn)

if (ENABLE_ANDROID_ADS)
    if (MVN_BIN AND (ANDROID_TARGET_SDK_VERSION GREATER_EQUAL 31))
        set(ANDROID_IMPLEMENTATIONS "com.google.android.gms:play-services-ads-lite:${GOOGLE_PLAY_SERVICES_ADS_VERSION}" CACHE INTERNAL "")
    else ()
        if (NOT MVN_BIN)
            message("Warning: You must install maven for enabling ads.")
        endif ()

        if (ANDROID_TARGET_SDK_VERSION LESS 31)
            message("Warning: The target API must be 31 or higher for enabling ads.")
        endif ()
    endif ()
endif ()

if (APPLE)
    set(BUILDDIR build)
    set(EXECPREFIX Webcamoid.app/Contents)
    set(BINDIR ${EXECPREFIX}/MacOS)
    set(LIBDIR ${EXECPREFIX}/Frameworks)
    set(PLUGINSDIR ${EXECPREFIX}/Plugins)
    set(AKPLUGINSDIR ${PLUGINSDIR}/avkys)
    set(DATAROOTDIR ${EXECPREFIX}/Resources)
    set(QMLDIR ${DATAROOTDIR}/qt/qml)
    set(LICENSEDIR ${DATAROOTDIR})
    set(TRANSLATIONSDIR ${DATAROOTDIR}/translations)
    set(OUTPUT_VLC_PLUGINS_DIR ${EXECPREFIX}/Plugins/vlc)
    set(OUTPUT_GST_PLUGINS_DIR ${EXECPREFIX}/Plugins/gstreamer-1.0)
    set(JARDIR ${DATAROOTDIR}/java)
elseif (ANDROID)
    set(BUILDDIR android-build)
    set(EXECPREFIX "")
    set(BINDIR libs/${CMAKE_ANDROID_ARCH_ABI})
    set(LIBDIR ${BINDIR})
    set(PLUGINSDIR ${BINDIR})
    set(AKPLUGINSDIR ${PLUGINSDIR})
    set(DATAROOTDIR assets)
    set(QMLDIR ${DATAROOTDIR}/qt/qml)
    set(LICENSEDIR ${DATAROOTDIR})
    set(TRANSLATIONSDIR ${DATAROOTDIR}/translations)
    set(OUTPUT_VLC_PLUGINS_DIR ${LIBDIR})
    set(OUTPUT_GST_PLUGINS_DIR ${LIBDIR})
    set(JARDIR libs)
else ()
    include(GNUInstallDirs)
    set(BUILDDIR build)
    set(EXECPREFIX "")
    set(BINDIR ${CMAKE_INSTALL_BINDIR})
    set(LIBDIR ${CMAKE_INSTALL_LIBDIR})
    set(PLUGINSDIR ${CMAKE_INSTALL_LIBDIR}/qt/plugins
        CACHE FILEPATH "Plugins install directory")
    set(AKPLUGINSDIR ${PLUGINSDIR}/avkys)
    set(QMLDIR ${CMAKE_INSTALL_LIBDIR}/qt/qml
        CACHE FILEPATH "Qml install directory")
    set(DATAROOTDIR ${CMAKE_INSTALL_DATAROOTDIR})
    set(LICENSEDIR ${DATAROOTDIR}/licenses/webcamoid)
    set(TRANSLATIONSDIR ${DATAROOTDIR}/webcamoid/translations)
    set(OUTPUT_VLC_PLUGINS_DIR ${LIBDIR}/vlc/plugins)
    set(OUTPUT_GST_PLUGINS_DIR ${LIBDIR}/gstreamer-1.0)
    set(OUTPUT_PIPEWIRE_MODULES_DIR ${LIBDIR}/pipewire)
    set(OUTPUT_PIPEWIRE_SPA_PLUGINS_DIR ${LIBDIR}/pipewire-spa)
    set(JARDIR ${DATAROOTDIR}/java)
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

# Enable the virtual camera support in Flatpak.
# Requires --talk-name=org.freedesktop.Flatpak to be set.

if (WITH_FLATPAK_VCAM)
    add_definitions(-DWITH_FLATPAK_VCAM)
endif ()

# Retrieve useful variables related to Qt installation.

set(GIT_COMMIT_HASH "" CACHE STRING "Set the alternative commit hash if not detected by git")

find_program(GIT_BIN git)

if (GIT_BIN)
    execute_process(COMMAND ${GIT_BIN} rev-parse HEAD
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                    OUTPUT_VARIABLE GIT_COMMIT_HASH_RESULT
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET)

endif ()

if ("${GIT_COMMIT_HASH_RESULT}" STREQUAL "")
    set(GIT_COMMIT_HASH_RESULT "${GIT_COMMIT_HASH}")
endif ()

if (NOT "${GIT_COMMIT_HASH_RESULT}" STREQUAL "")
    add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH_RESULT}")
endif ()

if (ANDROID)
    file(GLOB_RECURSE JAR_FILES ${ANDROID_SDK_ROOT}/platforms/android-*/android.jar)

    if (JAR_FILES)
        list(SORT JAR_FILES COMPARE NATURAL ORDER DESCENDING)
        list(GET JAR_FILES 0 DEFAULT_JAR_FILE)
        string(REGEX MATCH "[0-9]+" DEFAULT_ANDROID_TARGET_PLATFORM "${DEFAULT_JAR_FILE}")
    else ()
        if (ANDROIDLIB)
            get_filename_component(ANDROID_SYSROOT_LIB_API "${ANDROIDLIB}" DIRECTORY)
            get_filename_component(ANDROID_SYSROOT_LIB "${ANDROID_SYSROOT_LIB_API}" DIRECTORY)
            file(GLOB ANDROID_SYSROOT_LIB_APIS "${ANDROID_SYSROOT_LIB}/[0-9]*")

            if (ANDROID_SYSROOT_LIB_APIS)
                list(SORT ANDROID_SYSROOT_LIB_APIS COMPARE NATURAL ORDER DESCENDING)
                list(GET ANDROID_SYSROOT_LIB_APIS 0 NDK_LATEST_API)
                get_filename_component(DEFAULT_ANDROID_TARGET_PLATFORM "${NDK_LATEST_API}" NAME)

                if (NOT DEFAULT_ANDROID_TARGET_PLATFORM)
                    set(DEFAULT_ANDROID_TARGET_PLATFORM "${ANDROID_PLATFORM}")
                endif ()
            else ()
                set(DEFAULT_ANDROID_TARGET_PLATFORM "${ANDROID_PLATFORM}")
            endif ()
        else ()
            set(DEFAULT_ANDROID_TARGET_PLATFORM "${ANDROID_PLATFORM}")
        endif ()
    endif ()
endif ()

set(ANDROID_TARGET_SDK_VERSION "${DEFAULT_ANDROID_TARGET_PLATFORM}" CACHE STRING "Android target API")
set(ANDROID_JAVA_VERSION 1.7 CACHE STRING "Mimimum Java version to use in Android")
set(ANDROID_JAR_DIRECTORY ${ANDROID_SDK_ROOT}/platforms/android-${ANDROID_TARGET_SDK_VERSION} CACHE INTERNAL "")
set(MAVEN_LOCAL_REPOSITORY "${CMAKE_BINARY_DIR}/maven/repository" CACHE INTERNAL "")

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

# Set PkgConfig working directoies when multi ABI build is enabled.

if (ANDROID)
    string(REPLACE "-" "_" PKG_CONFIG_ABI "${ANDROID_ABI}")

    if (DEFINED ENV{PKG_CONFIG_SYSROOT_DIR_${PKG_CONFIG_ABI}})
        set(ENV{PKG_CONFIG_SYSROOT_DIR} "$ENV{PKG_CONFIG_SYSROOT_DIR_${PKG_CONFIG_ABI}}")
    endif ()

    if (DEFINED ENV{PKG_CONFIG_LIBDIR_${PKG_CONFIG_ABI}})
        set(ENV{PKG_CONFIG_LIBDIR} "$ENV{PKG_CONFIG_LIBDIR_${PKG_CONFIG_ABI}}")
    endif ()
endif ()

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

if (STATIC_BUILD AND NOT MSVC)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif ()

if ((STATIC_LIBGCC OR STATIC_BUILD) AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif ()

if (ENABLE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IPO_IS_SUPPORTED OUTPUT IPO_SUPPORTED_OUTPUT)
endif ()
