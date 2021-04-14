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

project(Translations)

include(../../libAvKys/cmake/ProjectCommons.cmake)

set(LANGUAGES
    ca
    de
    el
    es
    et
    fr
    gl
    he
    hu
    it
    ja
    kab
    ko
    nb_NO
    nl
    oc
    pl
    pt_BR
    pt
    ru
    si
    tr
    uk
    vi
    zh_CN
    zh_TW)

foreach (LANG IN LISTS LANGUAGES)
    configure_file(${CMAKE_SOURCE_DIR}/StandAlone/share/ts/${LANG}.ts
                   ${CMAKE_BINARY_DIR}/StandAlone/share/ts/${LANG}.ts
                   COPYONLY)
endforeach ()

# Update translations.
find_program(LUPDATE_TOOL lupdate)

if (LUPDATE_TOOL)
    foreach (LANG IN LISTS LANGUAGES)
        add_custom_target(tranlation_${LANG}_ts ALL
                          ${LUPDATE_TOOL} -no-obsolete
                                          ${CMAKE_SOURCE_DIR}/libAvKys
                                          ${CMAKE_SOURCE_DIR}/StandAlone
                                          -target-language ${LANG}
                                          -ts ${CMAKE_SOURCE_DIR}/StandAlone/share/ts/${LANG}.ts
                          VERBATIM)
    endforeach ()
endif ()

# Compile translations files.
find_program(LRELEASE_TOOL lrelease)

if (LRELEASE_TOOL)
    foreach (LANG IN LISTS LANGUAGES)
        add_custom_target(tranlation_${LANG}_qm ALL
                          ${LRELEASE_TOOL} -removeidentical
                                           -compress ${CMAKE_SOURCE_DIR}/StandAlone/share/ts/${LANG}.ts
                                           -qm ${CMAKE_SOURCE_DIR}/StandAlone/share/ts/${LANG}.qm
                          DEPENDS tranlation_${LANG}_ts
                          BYPRODUCTS StandAlone/share/ts/${LANG}.qm
                          VERBATIM)
    endforeach ()
endif ()

# Update qrc file.
set(TRANSLATIONS_QRC ${CMAKE_SOURCE_DIR}/StandAlone/translations.qrc)

file(WRITE ${TRANSLATIONS_QRC} "<RCC>\n")
string(REPEAT " " 4 INDENT_RESOURCE)
file(APPEND ${TRANSLATIONS_QRC} "${INDENT_RESOURCE}<qresource prefix=\"/Webcamoid\">\n")
set(TRANSLATION_RESOURCES "")
string(REPEAT " " 8 INDENT_FILE)

foreach (LANG IN LISTS LANGUAGES)
    file(APPEND ${TRANSLATIONS_QRC} "${INDENT_FILE}<file>share/ts/${LANG}.qm</file>\n")
endforeach ()

file(APPEND ${TRANSLATIONS_QRC} "${INDENT_RESOURCE}</qresource>\n")
file(APPEND ${TRANSLATIONS_QRC} "</RCC>\n")
