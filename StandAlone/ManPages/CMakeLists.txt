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

project(ManPages)

include(../../libAvKys/cmake/ProjectCommons.cmake)

set(MANPAGE StandAlone/share/man/webcamoid.1)
configure_file(src/webcamoid.1.in src/webcamoid.1)
find_program(GZIP_TOOL gzip)

if (GZIP_TOOL)
    add_custom_target(manpages ALL
                      ${GZIP_TOOL} -c9 webcamoid.1 > webcamoid.1.gz
                      DEPENDS src/webcamoid.1
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src
                      BYPRODUCTS src/webcamoid.1.gz
                      VERBATIM
                      SOURCES src/webcamoid.1.in)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/src/webcamoid.1.gz DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif ()
