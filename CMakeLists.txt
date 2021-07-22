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

# NOTE for other developers: TARGET_ARCH is intended to be used as a reference
# for the deploy tool, so don't rush on adding new architectures unless you
# want to create a binary distributable for that architecture.
# Webcamoid build is not affected in anyway by the value of TARGET_ARCH, if the
# build fails its something else and totally unrelated to that variable.

set(QT_QMAKE_EXECUTABLE qmake CACHE STRING "Qt's qmake executable")
find_program(QT_QMAKE_EXECUTABLE_BIN "${QT_QMAKE_EXECUTABLE}")

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

    if (QT_INSTALL_PREFIX)
        set(QT_INSTALL_SRC ${QT_INSTALL_PREFIX}/src)
    endif ()
endif ()

if (ANDROID)
    set(TARGET_PLATFORM android)
    set(BUILD_INFO_FILE ${DATAROOTDIR}/build-info.txt)

    if (${QT_VERSION_MAJOR} GREATER_EQUAL 6)
        set(MAIN_EXECUTABLE ${BINDIR}/Webcamoid_${CMAKE_ANDROID_ARCH_ABI})
    else ()
        set(MAIN_EXECUTABLE ${BINDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}Webcamoid_${CMAKE_ANDROID_ARCH_ABI}${CMAKE_SHARED_LIBRARY_SUFFIX})
    endif ()

    set(OUTPUT_FORMATS "AndroidAPK")
    set(TARGET_ARCH ${CMAKE_ANDROID_ARCH_ABI})
    set(OUTPUT_QML_DIR ${DATAROOTDIR}/android_rcc_bundle/qml)
    set(OUTPUT_QT_PLUGINS_DIR ${DATAROOTDIR}/android_rcc_bundle/plugins)
elseif (APPLE)
    set(TARGET_PLATFORM mac)
    set(BUILD_INFO_FILE ${DATAROOTDIR}/build-info.txt)
    set(MAIN_EXECUTABLE ${BINDIR}/Webcamoid)
    set(QTIFW_TARGET_DIR "\@ApplicationsDir\@/Webcamoid")
    set(OUTPUT_FORMATS "Dmg, QtIFW")
    set(INSTALLER_ICON StandAlone/share/themes/WebcamoidTheme/icons/webcamoid.icns)
    set(RUN_PROGRAM "\@TargetDir\@/${MAIN_EXECUTABLE}")
    set(OUTPUT_QML_DIR ${DATAROOTDIR}/qml)
    set(OUTPUT_QT_PLUGINS_DIR ${EXECPREFIX}/Plugins)
    set(QT_CONF_FILE ${DATAROOTDIR}/qt.conf)
elseif (WIN32)
    set(TARGET_PLATFORM windows)
    set(BUILD_INFO_FILE ${DATAROOTDIR}/build-info.txt)
    set(MAIN_EXECUTABLE ${BINDIR}/webcamoid.exe)

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

    set(OUTPUT_FORMATS "CompressedZip, Nsis")
    set(INSTALLER_ICON StandAlone/share/themes/WebcamoidTheme/icons/hicolor/256x256/webcamoid.ico)
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
    set(QTIFW_TARGET_DIR "\@HomeDir\@/Webcamoid")
    set(OUTPUT_FORMATS "AppImage, CompressedTarXz, QtIFW")
    set(INSTALLER_ICON StandAlone/share/themes/WebcamoidTheme/icons/hicolor/128x128/webcamoid.png)
    set(RUN_PROGRAM "\@TargetDir\@/webcamoid.sh")
    set(OUTPUT_QML_DIR ${LIBDIR}/qt/qml)
    set(OUTPUT_QT_PLUGINS_DIR ${LIBDIR}/qt/plugins)
    set(QT_CONF_FILE ${BINDIR}/qt.conf)
endif ()

if (UNIX AND NOT ANDROID)
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
        set(TARGET_ARCH x86_64)
    elseif (IS_I386_TARGET)
        set(TARGET_ARCH i386)
    elseif (IS_ARM64_TARGET)
        set(TARGET_ARCH arm64)
    elseif (IS_ARM_TARGET)
        set(TARGET_ARCH arm)
    elseif (IS_RISCV_TARGET)
        set(TARGET_ARCH riscv)
    else ()
        set(TARGET_ARCH unknown)
    endif ()
endif ()

configure_file(package_info.conf.in package_info.conf)

if (ANDROID)
    configure_file(package_info_android.conf.in package_info_android.conf)
    configure_file(COPYING
                   "${CMAKE_BINARY_DIR}/android-build/${LICENSEDIR}/COPYING"
                   COPYONLY)
else ()
    if (WIN32 AND CMAKE_HOST_UNIX)
        configure_file(package_info_windows.conf.in package_info_windows.conf)
    endif ()

    install(FILES COPYING DESTINATION ${LICENSEDIR})
endif ()

if (UNIX AND NOT ANDROID AND NOT MACX)
    install(FILES webcamoid.desktop DESTINATION ${DATAROOTDIR}/applications)
endif ()
