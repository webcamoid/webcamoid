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

project(StandAloneSrc LANGUAGES CXX)

include(../../libAvKys/cmake/ProjectCommons.cmake)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Concurrent
    OpenGL
    Qml
    Quick
    QuickControls2
    Svg
    Widgets)

if (ANDROID)
    list(APPEND QT_COMPONENTS
         Xml)
endif()

find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
set(PROJECT_SOURCES
    audiolayer.cpp
    audiolayer.h
    clioptions.cpp
    clioptions.h
    downloadmanager.cpp
    downloadmanager.h
    iconsprovider.cpp
    iconsprovider.h
    main.cpp
    mediatools.cpp
    mediatools.h
    pluginconfigs.cpp
    pluginconfigs.h
    recording.cpp
    recording.h
    updates.cpp
    updates.h
    videodisplay.cpp
    videodisplay.h
    videoeffects.cpp
    videoeffects.h
    videolayer.cpp
    videolayer.h
    ../DefaultTheme.qrc
    ../icons.qrc
    ../Webcamoid.qrc)

if (APPLE)
    list(APPEND PROJECT_SOURCES ../WebcamoidInfo.plist
                                ../share/themes/WebcamoidTheme/icons/webcamoid.icns)
    set_source_files_properties(../share/themes/WebcamoidTheme/icons/webcamoid.icns PROPERTIES
                                MACOSX_PACKAGE_LOCATION Resources)
elseif (WIN32)
    list(APPEND PROJECT_SOURCES ../Webcamoid.rc)
endif ()

qt_add_executable(StandAlone ${PROJECT_SOURCES})

if (APPLE)
    string(TIMESTAMP YEAR "%Y")
    set_target_properties(StandAlone PROPERTIES
                          OUTPUT_NAME Webcamoid
                          MACOSX_BUNDLE TRUE
                          MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../WebcamoidInfo.plist
                          MACOSX_BUNDLE_BUNDLE_NAME Webcamoid
                          MACOSX_BUNDLE_BUNDLE_VERSION ${VERSION}
                          MACOSX_BUNDLE_COPYRIGHT "Copyright (C)  2011-${YEAR}  Gonzalo Exequiel Pedone"
                          MACOSX_BUNDLE_GUI_IDENTIFIER "${APP_IDENTIFIER}"
                          MACOSX_BUNDLE_ICON_FILE webcamoid.icns
                          MACOSX_BUNDLE_INFO_STRING "Webcamoid, The ultimate webcam suite!"
                          MACOSX_BUNDLE_LONG_VERSION_STRING "Webcamoid ${VERSION}"
                          MACOSX_BUNDLE_SHORT_VERSION_STRING ${VERSION})
elseif (ANDROID)
    set_target_properties(StandAlone PROPERTIES
                          OUTPUT_NAME Webcamoid)
else ()
    set_target_properties(StandAlone PROPERTIES
                          OUTPUT_NAME webcamoid)
endif ()

if (WIN32)
    set_target_properties(StandAlone PROPERTIES
                          WIN32_EXECUTABLE TRUE)
endif ()

if (APPLE)
    set_target_properties(StandAlone PROPERTIES
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR})
else ()
    set_target_properties(StandAlone PROPERTIES
                          LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR}
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILDDIR}/${BINDIR})
endif ()

if (IPO_IS_SUPPORTED)
    set_target_properties(StandAlone PROPERTIES
                          INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

add_dependencies(StandAlone avkys)
target_include_directories(StandAlone
                           PRIVATE ../../libAvKys/Lib/src)
target_compile_definitions(StandAlone
                           PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(StandAlone
                      PUBLIC
                      avkys
                      ${QT_LIBS})

if (WIN32)
    target_link_libraries(StandAlone
                          PUBLIC
                          ole32)
endif ()

if (ANDROID)
    target_link_libraries(StandAlone
                          PUBLIC
                          log
                          PRIVATE
                          Qt${QT_VERSION_MAJOR}::CorePrivate)
endif ()

string(REPLACE "." ";" ORGANIZATION_DOMAIN "${ORGANIZATION_IDENTIFIER}")
list(REVERSE ORGANIZATION_DOMAIN)
string(JOIN "." ORGANIZATION_DOMAIN ${ORGANIZATION_DOMAIN})

add_definitions(-DCOMMONS_TARGET="webcamoid"
                -DCOMMONS_APPNAME="Webcamoid"
                -DCOMMONS_VERSION="${VERSION}"
                -DORGANIZATION_DOMAIN="${ORGANIZATION_DOMAIN}"
                -DBINDIR="${BINDIR}"
                -DDATAROOTDIR="${DATAROOTDIR}"
                -DTRANSLATIONSDIR="${TRANSLATIONSDIR}"
                -DTARGET_ARCH="${TARGET_ARCH}"
                -DANDROID_OPENSSL_SUFFIX="${ANDROID_OPENSSL_SUFFIX}")

if (ANDROID)
    if (ENABLE_ANDROID_DEBUGGING)
        add_definitions(-DENABLE_ANDROID_DEBUGGING)
    endif ()

    if (ENABLE_ANDROID_ADS AND MVN_BIN AND (ANDROID_TARGET_SDK_VERSION GREATER_EQUAL 31))
        add_definitions(-DENABLE_ANDROID_ADS
                        -DANDROID_AD_UNIT_ID_APP_OPEN="${ANDROID_AD_UNIT_ID_APP_OPEN}"
                        -DANDROID_AD_UNIT_ID_BANNER="${ANDROID_AD_UNIT_ID_BANNER}"
                        -DANDROID_AD_UNIT_ID_ADAPTIVE_BANNER="${ANDROID_AD_UNIT_ID_ADAPTIVE_BANNER}"
                        -DANDROID_AD_UNIT_ID_ADAPTIVE_INTERSTITIAL="${ANDROID_AD_UNIT_ID_ADAPTIVE_INTERSTITIAL}"
                        -DANDROID_AD_UNIT_ID_ADAPTIVE_REWARDED="${ANDROID_AD_UNIT_ID_ADAPTIVE_REWARDED}"
                        -DANDROID_AD_UNIT_ID_ADAPTIVE_REWARDED_INTERSTITIAL="${ANDROID_AD_UNIT_ID_ADAPTIVE_REWARDED_INTERSTITIAL}")
    endif ()

    configure_file(android_values_appid.xml
                   "${CMAKE_BINARY_DIR}/${BUILDDIR}/res/values/appid.xml")
endif ()

if (APPLE)
    install(TARGETS StandAlone DESTINATION ${CMAKE_INSTALL_PREFIX})
elseif (NOT ANDROID)
    install(TARGETS StandAlone DESTINATION ${BINDIR})
endif ()

if (WIN32)
    install(FILES ../share/themes/WebcamoidTheme/icons/hicolor/256x256/webcamoid.ico
            DESTINATION ${CMAKE_INSTALL_PREFIX})
elseif (UNIX AND NOT APPLE AND NOT ANDROID)
    file(GLOB_RECURSE ICONS
         ../share/themes/WebcamoidTheme/*/webcamoid.png
         ../share/themes/WebcamoidTheme/*/webcamoid.svg)

    foreach (ICON IN LISTS ICONS)
        file(RELATIVE_PATH ICON_DST ${CMAKE_CURRENT_SOURCE_DIR}/../share/themes/WebcamoidTheme ${ICON})
        get_filename_component(INSTALL_DIR ${ICON_DST} DIRECTORY)
        string(APPEND INSTALL_DIR /apps)
        install(FILES ${ICON} DESTINATION ${DATAROOTDIR}/${INSTALL_DIR})
    endforeach ()
endif ()

if (ANDROID)
    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../share/android")
    file(GLOB_RECURSE ANDROID_PACKAGE_TEMPLATES
         ../share/android/*)

    foreach (FILE IN LISTS ANDROID_PACKAGE_TEMPLATES)
        file(RELATIVE_PATH FILE_DST ${CMAKE_CURRENT_SOURCE_DIR}/../share/android ${FILE})
        configure_file(${FILE} "${CMAKE_BINARY_DIR}/${BUILDDIR}/${FILE_DST}" COPYONLY)
    endforeach ()
endif()
