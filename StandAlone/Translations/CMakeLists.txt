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

project(Translations)

include(../../libAvKys/cmake/ProjectCommons.cmake)

set(LANGUAGES
    af
    am
    ar
    as
    az
    be
    bg
    bn
    bs
    ca
    cs
    da
    de
    el
    es
    et
    eu
    fa
    fi
    fil
    fr
    gl
    gu
    he
    hi
    hr
    hu
    hy
    id
    is
    it
    ja
    ka
    kab
    kk
    km
    kn
    ko
    ky
    ms
    my
    nb_NO
    nl
    oc
    ota
    pl
    pt
    pt_BR
    ro
    ru
    si
    sk
    sl
    sq
    sv
    sw
    ta
    th
    tk
    tr
    uk
    ur
    uz
    vi
    zh_CN
    zh_HK
    zh_TW)

# Update translations.

set(UPDATE_TRANSLATIONS ON CACHE BOOL "Update translations when building")

set(LUPDATE_TOOL  "${QT_INSTALL_BINS}/lupdate"  CACHE FILEPATH "Path to lupdate tool")
set(LRELEASE_TOOL "${QT_INSTALL_BINS}/lrelease" CACHE FILEPATH "Path to lrelease tool")

# Macro to resolve a Qt tool in the following order:
#
#   1) User override (if EXISTS)
#   2) QT_INSTALL_BINS
#   3) System PATH
function(resolve_qt_tool OUT_VAR USER_VAR TOOL_NAME)
    # 1) User override (absolute path)
    if (EXISTS "${${USER_VAR}}")
        set(${OUT_VAR} "${${USER_VAR}}" PARENT_SCOPE)

        return ()
    endif ()

    # 2) Look inside Qt installation path
    find_program(_qt_tmp_${TOOL_NAME}
                 NAMES ${TOOL_NAME}
                 HINTS "${QT_INSTALL_BINS}"
                 NO_DEFAULT_PATH )

    if (_qt_tmp_${TOOL_NAME})
        set(${OUT_VAR} "${_qt_tmp_${TOOL_NAME}}" PARENT_SCOPE)

        return ()
    endif ()

    # 3) System PATH
    find_program(_qt_tmp_${TOOL_NAME} ${TOOL_NAME})

    if (_qt_tmp_${TOOL_NAME})
        set(${OUT_VAR} "${_qt_tmp_${TOOL_NAME}}" PARENT_SCOPE)

        return ()
    endif ()

    # Tool not found
    set(${OUT_VAR} "" PARENT_SCOPE)
endfunction()

# Resolve the translation tools

resolve_qt_tool(LUPDATE_TOOL_BIN  LUPDATE_TOOL  lupdate)
resolve_qt_tool(LRELEASE_TOOL_BIN LRELEASE_TOOL lrelease)

# Update translation sources (.ts)

if (UPDATE_TRANSLATIONS AND LUPDATE_TOOL_BIN)
    foreach (LANG IN LISTS LANGUAGES)
        add_custom_target(translation_${LANG}_ts ALL
                          ${LUPDATE_TOOL_BIN}
                          -no-obsolete
                          "${CMAKE_SOURCE_DIR}/libAvKys"
                          "${CMAKE_SOURCE_DIR}/StandAlone"
                          -target-language ${LANG}
                          -ts "${CMAKE_SOURCE_DIR}/StandAlone/share/ts/${LANG}.ts"
                          VERBATIM)
    endforeach ()
endif ()

# Build translation binaries (.qm)

if (LRELEASE_TOOL_BIN)
    add_custom_target(translations_qm ALL)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/${BUILDDIR}/${TRANSLATIONSDIR}")

    foreach (LANG IN LISTS LANGUAGES)
        add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/${BUILDDIR}/${TRANSLATIONSDIR}/${LANG}.qm"
                           COMMAND ${LRELEASE_TOOL_BIN}
                                   -removeidentical
                                   -compress
                                   "${CMAKE_SOURCE_DIR}/StandAlone/share/ts/${LANG}.ts"
                                   -qm "${CMAKE_BINARY_DIR}/${BUILDDIR}/${TRANSLATIONSDIR}/${LANG}.qm"
                           DEPENDS "${CMAKE_SOURCE_DIR}/StandAlone/share/ts/${LANG}.ts"
                           VERBATIM)
        add_custom_target(translation_${LANG}_qm ALL
                          DEPENDS "${CMAKE_BINARY_DIR}/${BUILDDIR}/${TRANSLATIONSDIR}/${LANG}.qm"
                          VERBATIM)
        add_dependencies(translations_qm translation_${LANG}_qm)

        install(FILES "${CMAKE_BINARY_DIR}/${BUILDDIR}/${TRANSLATIONSDIR}/${LANG}.qm"
                DESTINATION ${TRANSLATIONSDIR})
    endforeach ()
endif ()
