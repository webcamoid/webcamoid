# Webcamoid, camera capture application.
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

project(DesktopCapture_androidscreenjar)

include(../../../../../cmake/ProjectCommons.cmake)

set(SOURCES_PREFIX src/org/webcamoid/plugins/DesktopCapture/submodules/androidscreen)
set(SOURCES
    ${SOURCES_PREFIX}/AkAndroidScreenCallbacks.java
    ${SOURCES_PREFIX}/ScreenCaptureService.java)

# Maven settings

set(MAVEN_REPOSITORIES
    "https://maven.google.com")
set(MAVEN_DEPENDENCIES
    "androidx.core:core:${ANDROIDX_CORE_VERSION}:aar"
    "androidx.annotation:annotation:${ANDROIDX_ANNOTATION_VERSION}:jar")

if (ANDROID AND NOT NOSCREENCAPTURE)
    include(UseJava)
    find_package(Java ${ANDROID_JAVA_VERSION})

    if (Java_FOUND)
        resolve_maven_dependencies(DesktopCapture_androidscreenjar_deps
                                   REPOSITORIES ${MAVEN_REPOSITORIES}
                                   DEPENDENCIES ${MAVEN_DEPENDENCIES}
                                   OUT_JARS MAVEN_DEPS)
        set(CMAKE_JAVA_COMPILE_FLAGS
            "-source" "${ANDROID_JAVA_VERSION}"
            "-target" "${ANDROID_JAVA_VERSION}")
        add_jar(DesktopCapture_androidscreenjar
                SOURCES ${SOURCES}
                INCLUDE_JARS
                ${ANDROID_JAR_DIRECTORY}/android.jar
                ${MAVEN_DEPS}
                OUTPUT_NAME AkDesktopCapture_androidscreen
                OUTPUT_DIR ${CMAKE_BINARY_DIR}/${BUILDDIR}/${JARDIR}
                DEPENDS DesktopCapture_androidscreenjar_deps)
        install_jar(DesktopCapture_androidscreenjar DESTINATION ${JARDIR})
    endif ()
endif ()

add_custom_target(DesktopCapture_androidscreenjar_sources SOURCES ${SOURCES})
