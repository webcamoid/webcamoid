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

project(FaceDetect LANGUAGES CXX)

include(../../cmake/ProjectCommons.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS
    Concurrent
    Gui
    Qml)
find_package(QT NAMES Qt${QT_VERSION_MAJOR} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${QT_MINIMUM_VERSION} COMPONENTS
             ${QT_COMPONENTS}
             REQUIRED)
add_library(FaceDetect SHARED
            src/facedetect.h
            src/facedetectelement.h
            src/haar/haarcascade.h
            src/haar/haardetector.h
            src/haar/haarfeature.h
            src/haar/haarstage.h
            src/haar/haartree.h
            src/facedetect.cpp
            src/facedetectelement.cpp
            src/haar/haarcascade.cpp
            src/haar/haardetector.cpp
            src/haar/haarfeature.cpp
            src/haar/haarstage.cpp
            src/haar/haartree.cpp
            FaceDetect.qrc
            haarcascades.qrc
            masks.qrc
            pspec.json)

if (WIN32)
    set_target_properties(FaceDetect PROPERTIES
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${PLUGINSDIR})
elseif (NOT ANDROID)
    set_target_properties(FaceDetect PROPERTIES
                          LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/${PLUGINSDIR})
endif ()

add_dependencies(FaceDetect avkys)
target_include_directories(FaceDetect
                           PRIVATE ../../Lib/src)
target_compile_definitions(FaceDetect PRIVATE AVKYS_PLUGIN_FACEDETECT)
list(TRANSFORM QT_COMPONENTS PREPEND Qt${QT_VERSION_MAJOR}:: OUTPUT_VARIABLE QT_LIBS)
target_link_libraries(FaceDetect avkys ${QT_LIBS})

if (WIN32)
    install(TARGETS FaceDetect RUNTIME DESTINATION ${PLUGINSDIR})
elseif (NOT ANDROID)
    install(TARGETS FaceDetect DESTINATION ${PLUGINSDIR})
endif ()
