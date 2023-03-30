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
    sv
    tr
    uk
    vi
    zh_CN
    zh_TW)

# Update translations.

set(UPDATE_TRANSLATIONS ON CACHE BOOL "Update translations when building")

if (UPDATE_TRANSLATIONS)
    find_program(LUPDATE_TOOL lupdate)

    if (LUPDATE_TOOL)
        foreach (LANG IN LISTS LANGUAGES)
            add_custom_target(translation_${LANG}_ts ALL
                              ${LUPDATE_TOOL} -no-obsolete
                                              ${CMAKE_SOURCE_DIR}/libAvKys
                                              ${CMAKE_SOURCE_DIR}/StandAlone
                                              -target-language ${LANG}
                                              -ts ${CMAKE_SOURCE_DIR}/StandAlone/share/ts/${LANG}.ts
                              VERBATIM)
        endforeach ()
    endif ()
endif ()

# Compile translations files.
find_program(LRELEASE_TOOL lrelease)

if (LRELEASE_TOOL)
    # target for all languages
    add_custom_target(translations_qm)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/build/${TRANSLATIONSDIR})

    foreach (LANG IN LISTS LANGUAGES)
        add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/build/${TRANSLATIONSDIR}/${LANG}.qm
            COMMAND ${LRELEASE_TOOL} -removeidentical
                                     -compress ${CMAKE_SOURCE_DIR}/StandAlone/share/ts/${LANG}.ts
                                     -qm ${CMAKE_BINARY_DIR}/build/${TRANSLATIONSDIR}/${LANG}.qm
            DEPENDS ${CMAKE_SOURCE_DIR}/StandAlone/share/ts/${LANG}.ts
            VERBATIM
        )
        add_custom_target(translation_${LANG}_qm ALL
            DEPENDS ${CMAKE_BINARY_DIR}/build/${TRANSLATIONSDIR}/${LANG}.qm
            VERBATIM
        )
        add_dependencies(translations_qm translation_${LANG}_qm)
        install(FILES ${CMAKE_BINARY_DIR}/build/${TRANSLATIONSDIR}/${LANG}.qm
                DESTINATION ${TRANSLATIONSDIR})
    endforeach ()
endif ()
