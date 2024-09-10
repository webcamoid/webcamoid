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

project(Webcamoid_jarutils)

include(../../libAvKys/cmake/ProjectCommons.cmake)

set(SOURCES_PREFIX src/org/webcamoid/webcamoidutils)
set(SOURCES ${SOURCES_PREFIX}/WebcamoidUtils.java)
set(DEPENDENCY_SOURCES ${SOURCES_PREFIX}/AdManager.java)
set(COMPILE_SOURCES ${SOURCES})
set(ALL_SOURCES ${SOURCES} ${DEPENDENCY_SOURCES})
set(INCLUDE_JARS ${ANDROID_JAR_DIRECTORY}/android.jar)

set(MAVEN_REPOSITORIES
    "https://maven.google.com")
string(JOIN "," MAVEN_REPOSITORIES ${MAVEN_REPOSITORIES})

set(MAVEN_DEPENDENCIES
    "com.google.android.gms:play-services-ads-lite:${GOOGLE_PLAY_SERVICES_ADS_VERSION}:aar")

if (ANDROID)
    include(UseJava)
    find_package(Java ${ANDROID_JAVA_VERSION})

    if (Java_FOUND)
        if (ENABLE_ANDROID_ADS AND MVN_BIN AND (ANDROID_TARGET_SDK_VERSION GREATER_EQUAL 31))
            list(APPEND COMPILE_SOURCES ${DEPENDENCY_SOURCES})

            add_custom_target(StandAlone_java_deps)
            set(STANDALONE_JAVA_DEPS StandAlone_java_deps CACHE INTERNAL "")

            foreach (DEPENDENCY IN LISTS MAVEN_DEPENDENCIES)
                string(REPLACE ":" ";" DEPENDENCY_PARTS "${DEPENDENCY}")
                list(GET DEPENDENCY_PARTS 0 DEPENDENCY_GROUPID)
                list(GET DEPENDENCY_PARTS 1 DEPENDENCY_ARTIFACT)
                list(GET DEPENDENCY_PARTS 2 DEPENDENCY_VERSION)
                list(GET DEPENDENCY_PARTS 3 DEPENDENCY_EXT)
                set(DEPENDENCY_TGNAME "${DEPENDENCY_GROUPID}_${DEPENDENCY_ARTIFACT}_${DEPENDENCY_EXT}")
                string(REPLACE "." "_" DEPENDENCY_TGNAME "${DEPENDENCY_TGNAME}")
                string(REPLACE "-" "_" DEPENDENCY_TGNAME "${DEPENDENCY_TGNAME}")
                string(REPLACE "." "/" GROUPID_PATH "${DEPENDENCY_GROUPID}")
                set(DEPFILE_PATH "${MAVEN_LOCAL_REPOSITORY}/${GROUPID_PATH}/${DEPENDENCY_ARTIFACT}/${DEPENDENCY_VERSION}/${DEPENDENCY_ARTIFACT}-${DEPENDENCY_VERSION}.${DEPENDENCY_EXT}")
                get_filename_component(DEPFILE_DIR "${DEPFILE_PATH}" DIRECTORY)
                add_custom_command(
                    OUTPUT "${DEPFILE_PATH}"
                    COMMAND ${MVN_BIN}
                            -Dmaven.repo.local=${MAVEN_LOCAL_REPOSITORY}
                            dependency:get
                            -Dartifact=${DEPENDENCY}
                            -DremoteRepositories=${MAVEN_REPOSITORIES}
                            -Dtransitive=false
                    VERBATIM
                )

                if ("${DEPENDENCY_EXT}" STREQUAL "aar")
                    add_custom_command(
                        OUTPUT "${DEPFILE_DIR}/classes.jar"
                        COMMAND ${CMAKE_COMMAND} -E tar xzf "${DEPFILE_PATH}"
                        WORKING_DIRECTORY "${DEPFILE_DIR}"
                        DEPENDS "${DEPFILE_PATH}"
                        VERBATIM
                    )
                    add_custom_target(StandAlone_java_deps_${DEPENDENCY_TGNAME} ALL
                        DEPENDS "${DEPFILE_DIR}/classes.jar"
                        VERBATIM
                    )
                    list(APPEND INCLUDE_JARS "${DEPFILE_DIR}/classes.jar")
                    add_dependencies(StandAlone_java_deps StandAlone_java_deps_${DEPENDENCY_TGNAME})
                else ()
                    add_custom_target(StandAlone_java_deps_${DEPENDENCY_TGNAME} ALL
                        DEPENDS "${DEPFILE_DIR}/${DEPENDENCY_ARTIFACT}-${DEPENDENCY_VERSION}.${DEPENDENCY_EXT}"
                        VERBATIM
                    )
                    list(APPEND INCLUDE_JARS "${DEPFILE_DIR}/${DEPENDENCY_ARTIFACT}-${DEPENDENCY_VERSION}.${DEPENDENCY_EXT}")
                    add_dependencies(StandAlone_java_deps StandAlone_java_deps_${DEPENDENCY_TGNAME})
                endif ()
            endforeach ()
        endif ()

        set(CMAKE_JAVA_COMPILE_FLAGS "-source" "${ANDROID_JAVA_VERSION}" "-target" "${ANDROID_JAVA_VERSION}")
        add_jar(Webcamoid_jarutils
                SOURCES ${COMPILE_SOURCES}
                INCLUDE_JARS ${INCLUDE_JARS}
                OUTPUT_NAME Webcamoid_jarutils
                OUTPUT_DIR ${CMAKE_BINARY_DIR}/${BUILDDIR}/${JARDIR}
                DEPENDS ${STANDALONE_JAVA_DEPS})
        install_jar(Webcamoid_jarutils DESTINATION ${JARDIR})
    endif ()
endif ()

add_custom_target(Webcamoid_jarutils_sources SOURCES ${ALL_SOURCES})
