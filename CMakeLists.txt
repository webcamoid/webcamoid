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
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
endif ()

add_subdirectory(libAvKys)
add_subdirectory(StandAlone)

# Try detecting Qmake executable.

set(QT_QMAKE_EXECUTABLE qmake CACHE FILEPATH "Qt qmake path")
find_program(QT_QMAKE_EXECUTABLE_BIN "${QT_QMAKE_EXECUTABLE}")

# Retrieve useful variables related to Qt installation.

if (QT_QMAKE_EXECUTABLE_BIN)
    execute_process(COMMAND ${QT_QMAKE_EXECUTABLE_BIN} -query QT_INSTALL_PREFIX
                    OUTPUT_VARIABLE QT_INSTALL_PREFIX
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${QT_QMAKE_EXECUTABLE_BIN} -query QT_INSTALL_LIBS
                    OUTPUT_VARIABLE QT_INSTALL_LIBS
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${QT_QMAKE_EXECUTABLE_BIN} -query QT_INSTALL_BINS
                    OUTPUT_VARIABLE QT_INSTALL_BINS
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${QT_QMAKE_EXECUTABLE_BIN} -query QT_INSTALL_PLUGINS
                    OUTPUT_VARIABLE QT_INSTALL_PLUGINS
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${QT_QMAKE_EXECUTABLE_BIN} -query QT_INSTALL_QML
                    OUTPUT_VARIABLE QT_INSTALL_QML
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif ()

set(QT_INSTALL_SRC "${QT_INSTALL_PREFIX}/src" CACHE PATH "Qt sources templates directory")

# Set platform related variables to be used in package_info.conf.

if (ANDROID)
    set(TARGET_PLATFORM android)
    set(BUILD_INFO_FILE ${DATAROOTDIR}/build-info.txt)
    set(MAIN_EXECUTABLE ${BINDIR}/libWebcamoid_${CMAKE_ANDROID_ARCH_ABI}.so)
    set(APP_LIBDIR ${LIBDIR})
    set(OUTPUT_FORMATS "AndroidAPK")
    set(OUTPUT_QML_DIR ${DATAROOTDIR}/android_rcc_bundle/qml)
    set(OUTPUT_QT_PLUGINS_DIR ${DATAROOTDIR}/android_rcc_bundle/plugins)
elseif (APPLE)
    set(TARGET_PLATFORM mac)
    set(BUILD_INFO_FILE ${DATAROOTDIR}/build-info.txt)
    set(MAIN_EXECUTABLE ${BINDIR}/Webcamoid)
    set(APP_LIBDIR ${LIBDIR})
    set(QTIFW_TARGET_DIR "\@ApplicationsDir\@/Webcamoid")
    set(OUTPUT_FORMATS "CompressedTarXz, MacPkg")
    set(INSTALLER_ICON "StandAlone/share/themes/WebcamoidTheme/icons/webcamoid.icns")
    set(RUN_PROGRAM "\@TargetDir\@/${MAIN_EXECUTABLE}")
    set(OUTPUT_QML_DIR ${DATAROOTDIR}/qml)
    set(OUTPUT_QT_PLUGINS_DIR ${EXECPREFIX}/Plugins)
    set(QT_CONF_FILE ${DATAROOTDIR}/qt.conf)
elseif (WIN32)
    set(TARGET_PLATFORM windows)
    set(BUILD_INFO_FILE ${DATAROOTDIR}/build-info.txt)
    set(MAIN_EXECUTABLE ${BINDIR}/webcamoid.exe)
    set(APP_LIBDIR ${LIBDIR})
    set(OUTPUT_FORMATS "CompressedZip, Nsis")
    set(INSTALLER_ICON "StandAlone/share/themes/WebcamoidTheme/icons/hicolor/256x256/webcamoid.ico")
    set(RUN_PROGRAM "${MAIN_EXECUTABLE}")
    set(PROGRAM_ARGS "-c \"%%~dp0${DATAROOTDIR}\\config\"")
    set(OUTPUT_QML_DIR ${LIBDIR}/qt/qml)
    set(OUTPUT_QT_PLUGINS_DIR ${LIBDIR}/qt/plugins)
    set(QT_CONF_FILE ${BINDIR}/qt.conf)
    set(EXTRA_LIBS_LIST
        libeay32.dll
        ssleay32.dll
        libEGL.dll
        libGLESv2.dll
        D3DCompiler_43.dll
        D3DCompiler_46.dll
        D3DCompiler_47.dll)
    list(JOIN EXTRA_LIBS_LIST ", " EXTRA_LIBS)
else ()
    set(TARGET_PLATFORM posix)
    set(BUILD_INFO_FILE ${DATAROOTDIR}/build-info.txt)
    set(MAIN_EXECUTABLE ${BINDIR}/webcamoid)
    set(APP_LIBDIR ${LIBDIR})
    set(QTIFW_TARGET_DIR "\@HomeDir\@/Webcamoid")
    set(OUTPUT_FORMATS "AppImage, CompressedTarXz, QtIFW, DebPackage, RpmPackage")
    set(INSTALLER_ICON "StandAlone/share/themes/WebcamoidTheme/icons/hicolor/128x128/webcamoid.png")
    set(RUN_PROGRAM "\@TargetDir\@/webcamoid.sh")
    set(OUTPUT_QML_DIR ${LIBDIR}/qt/qml)
    set(OUTPUT_QT_PLUGINS_DIR ${LIBDIR}/qt/plugins)
    set(QT_CONF_FILE ${BINDIR}/qt.conf)
endif ()

# Try detecting the target platform.

if (UNIX AND NOT APPLE AND NOT ANDROID)
    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles("
    #ifndef __linux__
        #error Not Linux
    #endif

    int main()
    {
        return 0;
    }" IS_LINUX)

    check_cxx_source_compiles("
    #ifndef __FreeBSD__
        #error Not FreeBSD
    #endif

    int main()
    {
        return 0;
    }" IS_FREEBSD)

    if (IS_LINUX)
        set(PKG_TARGET_PLATFORM linux)
    elseif (IS_FREEBSD)
        set(PKG_TARGET_PLATFORM freebsd)
    else ()
        set(PKG_TARGET_PLATFORM ${TARGET_PLATFORM})
    endif ()
else ()
    set(PKG_TARGET_PLATFORM ${TARGET_PLATFORM})
endif ()

find_package(PkgConfig)

# Try detecting VLC plugins install path.

pkg_check_modules(VLC_PLUGINS vlc-plugin)

if (VLC_PLUGINS_FOUND)
    pkg_get_variable(VLC_PLUGINS_DIR vlc-plugin pluginsdir)
elseif (APPLE AND NOT VLC_PLUGINS_FOUND)
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
        set(VLCAPP_PLUGINS_DIR "${VLCAPP_DIR}/Contents/MacOS/plugins")

        if (EXISTS ${VLCAPP_PLUGINS_DIR})
            message("--   Found VLC in '${VLCAPP_DIR}'")
            set(VLC_PLUGINS_DIR "${VLCAPP_PLUGINS_DIR}")
        else ()
            message("--   No 'VLC' found")
        endif ()
    else ()
        message("--   No 'VLC' found")
    endif ()
endif ()

# Try detecting GStreamer plugins install path.

pkg_check_modules(GST_PLUGINS gstreamer-1.0)

if (GST_PLUGINS_FOUND)
    pkg_get_variable(GST_PLUGINS_DIR gstreamer-1.0 pluginsdir)
endif ()

# Try detecting PipeWire plugins install path.

pkg_check_modules(PIPEWIRE_MODULES libpipewire-0.3)
pkg_check_modules(PIPEWIRE_SPA_PLUGINS libspa-0.2)

if (PIPEWIRE_MODULES_FOUND)
    pkg_get_variable(PIPEWIRE_MODULES_DIR libpipewire-0.3 moduledir)
endif ()

if (PIPEWIRE_SPA_PLUGINS_FOUND)
    pkg_get_variable(PIPEWIRE_SPA_PLUGINS_DIR libspa-0.2 plugindir)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(PACKAGE_IS_DEBUG true)
endif()

set(DEB_INSTALL_ROOT_PREFIX "usr" CACHE STRING "Install prefix relative to the system")
set(DEB_INSTALL_PREFIX "usr/share/webcamoid" CACHE STRING "Install prefix for DEB packages data files")
set(RPM_INSTALL_PREFIX "${DEB_INSTALL_PREFIX}" CACHE STRING "Install prefix for RPM packages data files")

if (TARGET_ARCH STREQUAL "x64")
    set(DEB_TARGET_ARCH_DEFAULT amd64)
    set(RPM_TARGET_ARCH_DEFAULT x86_64)
elseif (TARGET_ARCH STREQUAL "x86")
    set(DEB_TARGET_ARCH_DEFAULT i386)
    set(RPM_TARGET_ARCH_DEFAULT i686)
elseif (TARGET_ARCH STREQUAL "arm64")
    set(DEB_TARGET_ARCH_DEFAULT arm64)
    set(RPM_TARGET_ARCH_DEFAULT aarch64)
elseif (TARGET_ARCH STREQUAL "arm32")
    set(DEB_TARGET_ARCH_DEFAULT armhf)
    set(RPM_TARGET_ARCH_DEFAULT armv7hl)
elseif (TARGET_ARCH STREQUAL "riscv")
    set(DEB_TARGET_ARCH_DEFAULT riscv64)
    set(RPM_TARGET_ARCH_DEFAULT riscv64)
else ()
    set(DEB_TARGET_ARCH_DEFAULT any)
    set(RPM_TARGET_ARCH_DEFAULT noarch)
endif ()

set(DEB_TARGET_ARCH "${DEB_TARGET_ARCH_DEFAULT}" CACHE STRING "Architecture of the DEB package")
set(RPM_TARGET_ARCH "${RPM_TARGET_ARCH_DEFAULT}" CACHE STRING "Architecture of the RPM package")
set(DEB_LINKS "")

if (UNIX AND NOT APPLE AND NOT ANDROID)
    set(DEB_LINKS_PATHS
        ${BINDIR}/webcamoid
        ${DATAROOTDIR}/applications/webcamoid.desktop
        ${DATAROOTDIR}/metainfo/${APP_IDENTIFIER}.metainfo.xml
        ${CMAKE_INSTALL_MANDIR}/man1/webcamoid.1.gz
        ${LICENSEDIR}/COPYING)

    set(ICONS_SIZE
        "8x8"
        "16x16"
        "22x22"
        "32x32"
        "48x48"
        "64x64"
        "128x128"
        "256x256"
        "scalable")

    foreach (SIZE IN LISTS ICONS_SIZE)
        list(APPEND DEB_LINKS_PATHS ${DATAROOTDIR}/icons/hicolor/${SIZE}/apps/webcamoid.png)
    endforeach ()

    set(DEB_LINKS_PATHS_TEMP "dummy")

    foreach (LINK IN LISTS DEB_LINKS_PATHS)
        get_filename_component(ROOT_LINK_DIR "/${DEB_INSTALL_ROOT_PREFIX}/${LINK}" DIRECTORY)
        file(RELATIVE_PATH REL_PATH "${ROOT_LINK_DIR}" "/${DEB_INSTALL_PREFIX}/${LINK}")
        list(APPEND DEB_LINKS_PATHS_TEMP "${DEB_INSTALL_ROOT_PREFIX}/${LINK}:${REL_PATH}")
    endforeach ()

    list(POP_FRONT DEB_LINKS_PATHS_TEMP)
    list(JOIN DEB_LINKS_PATHS_TEMP ", " DEB_LINKS)
endif ()

set(RPM_LINKS "${DEB_LINKS}")
set(DEB_COPYRIGHT_FILE "${LICENSEDIR}/COPYING" CACHE STRING "Copyright file install path relative to DEB_INSTALL_PREFIX")

configure_file(package_info.conf.in package_info.conf)

if (ANDROID)
    configure_file(package_info_android.conf.in package_info_android.conf)
    configure_file(COPYING
                   "${CMAKE_BINARY_DIR}/${BUILDDIR}/${LICENSEDIR}/COPYING"
                   COPYONLY)
else ()
    if (WIN32 AND CMAKE_HOST_UNIX)
        configure_file(package_info_windows.conf.in package_info_windows.conf)
    endif ()

    install(FILES COPYING DESTINATION ${LICENSEDIR})
endif ()

if (UNIX AND NOT ANDROID AND NOT APPLE)
    install(FILES webcamoid.desktop DESTINATION ${DATAROOTDIR}/applications)
    install(FILES ${APP_IDENTIFIER}.metainfo.xml DESTINATION ${DATAROOTDIR}/metainfo)
endif ()

# uninstall target
#
# taken from https://gitlab.kitware.com/cmake/community/-/wikis/FAQ#can-i-do-make-uninstall-with-cmake
# usage: "cmake --build path/to/build/directory --target uninstall"

if (NOT TARGET uninstall)
    set(REMOVE_ROOT OFF CACHE BOOL "Remove root install directory when uninstalling")

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
                   IMMEDIATE @ONLY)

    add_custom_target(uninstall
                      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

    # Remove folders owned by the project.

    install(CODE "list(APPEND CMAKE_INSTALL_MANIFEST_FILES \"\${CMAKE_INSTALL_PREFIX}/${AKPLUGINSDIR}\")")
    install(CODE "list(APPEND CMAKE_INSTALL_MANIFEST_FILES \"\${CMAKE_INSTALL_PREFIX}/${LICENSEDIR}\")")
    install(CODE "list(APPEND CMAKE_INSTALL_MANIFEST_FILES \"\${CMAKE_INSTALL_PREFIX}/${DATAROOTDIR}/webcamoid\")")
endif ()
