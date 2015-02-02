# Webcamoid, webcam capture application.
# Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
#
# Webcamod is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Webcamod is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
#
# Email     : hipersayan DOT x AT gmail DOT com
# Web-Site 1: http://github.com/hipersayanX/Webcamoid
# Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796

exists(commons.pri) {
    include(commons.pri)
} else {
    error("commons.pri file not found.")
}

!isEmpty(BUILDDOCS):!isEqual(BUILDDOCS, 0) {
    DOCSOURCES += $${COMMONS_APPNAME}.qdocconf

    builddocs.input = DOCSOURCES
    builddocs.output = share/docs_auto/html/$${COMMONS_TARGET}.index
    builddocs.commands = $${QDOCTOOL} ${QMAKE_FILE_IN}
    builddocs.variable_out = DOCSOUTPUT
    builddocs.name = Docs ${QMAKE_FILE_IN}
    builddocs.CONFIG += target_predeps

    QMAKE_EXTRA_COMPILERS += builddocs
    PRE_TARGETDEPS += compiler_builddocs_make_all
}

compiletr.input = TRANSLATIONS
compiletr.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
compiletr.commands = $$QMAKE_LRELEASE -removeidentical -compress ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
compiletr.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += compiletr
PRE_TARGETDEPS += compiler_compiletr_make_all

CONFIG += qt

HEADERS = \
    include/mediatools.h \
    include/videodisplay.h \
    include/videoframe.h \
    include/recordingformat.h

INCLUDEPATH += \
    include \
    Qb/include

!win32: LIBS += -L./Qb -lQb
win32: LIBS += -L./Qb -lQb$${VER_MAJ}

OTHER_FILES = \
    .gitignore \
    README.md \
    share/effects.xml

QT += qml quick opengl widgets xml

RESOURCES += \
    Webcamoid.qrc \
    icons.qrc \
    qml.qrc \
    translations.qrc

SOURCES = \
    src/main.cpp \
    src/mediatools.cpp \
    src/videodisplay.cpp \
    src/videoframe.cpp \
    src/recordingformat.cpp

lupdate_only {
    SOURCES = share/qml/*.qml
}

TRANSLATIONS = $$files(share/ts/*.ts)

DESTDIR = $${PWD}

TARGET = $${COMMONS_TARGET}

TEMPLATE = app

CODECFORTR = UTF-8
CODECFORSRC = UTF-8

# http://www.loc.gov/standards/iso639-2/php/code_list.php

CODECFORTR = UTF-8
CODECFORSRC = UTF-8

INSTALLS += target
unix: INSTALLS += desktop
!unix: INSTALLS += \
    dllDeps \
    pluginsPlatform

unix:target.path = $${BINDIR}
!unix:target.path = $${PREFIX}

unix {
    desktop.files = $${COMMONS_TARGET}.desktop
    desktop.path = $${DATAROOTDIR}/applications/kde4
}
!unix {
    dllDeps.files = \
        \ # Qt
        $$[QT_INSTALL_BINS]/Qt5Core.dll \
        $$[QT_INSTALL_BINS]/Qt5Gui.dll \
        $$[QT_INSTALL_BINS]/Qt5Multimedia.dll \
        $$[QT_INSTALL_BINS]/Qt5Network.dll \
        $$[QT_INSTALL_BINS]/Qt5OpenGL.dll \
        $$[QT_INSTALL_BINS]/Qt5Qml.dll \
        $$[QT_INSTALL_BINS]/Qt5Quick.dll \
        $$[QT_INSTALL_BINS]/Qt5Widgets.dll \
        \ # FFmpeg
        $$[QT_INSTALL_BINS]/avcodec-*.dll \
        $$[QT_INSTALL_BINS]/avdevice-*.dll \
        $$[QT_INSTALL_BINS]/avfilter-*.dll \
        $$[QT_INSTALL_BINS]/avformat-*.dll \
        $$[QT_INSTALL_BINS]/avresample-*.dll \
        $$[QT_INSTALL_BINS]/avutil-*.dll \
        $$[QT_INSTALL_BINS]/postproc-*.dll \
        $$[QT_INSTALL_BINS]/swresample-*.dll \
        $$[QT_INSTALL_BINS]/swscale-*.dll \
        \ # SDL
        $$[QT_INSTALL_BINS]/SDL.dll \
        \ # Codecs
        $$[QT_INSTALL_BINS]/libass-*.dll \
        $$[QT_INSTALL_BINS]/libbluray-*.dll \
        $$[QT_INSTALL_BINS]/libbz2-*.dll \
        $$[QT_INSTALL_BINS]/libgnutls-*.dll \
        $$[QT_INSTALL_BINS]/libgsm.dll.1.* \
        $$[QT_INSTALL_BINS]/libiconv-*.dll \
        $$[QT_INSTALL_BINS]/libjpeg-*.dll \
        $$[QT_INSTALL_BINS]/libmodplug-*.dll \
        $$[QT_INSTALL_BINS]/libmp3lame-*.dll \
        $$[QT_INSTALL_BINS]/libogg-*.dll \
        $$[QT_INSTALL_BINS]/libopencore-amrnb-*.dll \
        $$[QT_INSTALL_BINS]/libopencore-amrwb-*.dll \
        $$[QT_INSTALL_BINS]/libopenjpeg-*.dll \
        $$[QT_INSTALL_BINS]/libopus-*.dll \
        $$[QT_INSTALL_BINS]/librtmp-*.dll \
        $$[QT_INSTALL_BINS]/libschroedinger-*.dll \
        $$[QT_INSTALL_BINS]/libspeex-*.dll \
        $$[QT_INSTALL_BINS]/libtheoradec-*.dll \
        $$[QT_INSTALL_BINS]/libtheoraenc-*.dll \
        $$[QT_INSTALL_BINS]/libtiff-*.dll \
        $$[QT_INSTALL_BINS]/libvorbis-*.dll \
        $$[QT_INSTALL_BINS]/libvorbisenc-*.dll \
        $$[QT_INSTALL_BINS]/libvpx.dll.1.3.0 \
        $$[QT_INSTALL_BINS]/libx264-*.dll \
        $$[QT_INSTALL_BINS]/libx265.dll \
        $$[QT_INSTALL_BINS]/xvidcore.dll \
        \ # OpenCV
        $$[QT_INSTALL_BINS]/libopencv_core*.dll \
        $$[QT_INSTALL_BINS]/libopencv_highgui*.dll \
        $$[QT_INSTALL_BINS]/libopencv_imgproc*.dll \
        $$[QT_INSTALL_BINS]/libopencv_objdetect*.dll \
        \ # System
        $$[QT_INSTALL_BINS]/libeay32.dll \
        $$[QT_INSTALL_BINS]/libEGL.dll \
        $$[QT_INSTALL_BINS]/libenca-*.dll \
        $$[QT_INSTALL_BINS]/libexpat-*.dll \
        $$[QT_INSTALL_BINS]/libffi-*.dll \
        $$[QT_INSTALL_BINS]/libfontconfig-*.dll \
        $$[QT_INSTALL_BINS]/libfreetype-*.dll \
        $$[QT_INSTALL_BINS]/libfribidi-*.dll \
        $$[QT_INSTALL_BINS]/libgcc_s_*.dll \
        $$[QT_INSTALL_BINS]/libGLESv*.dll \
        $$[QT_INSTALL_BINS]/libglib-*.dll \
        $$[QT_INSTALL_BINS]/libgmp-*.dll \
        $$[QT_INSTALL_BINS]/libHalf-*.dll \
        $$[QT_INSTALL_BINS]/libharfbuzz-0.dll \
        $$[QT_INSTALL_BINS]/libhogweed-*.dll \
        $$[QT_INSTALL_BINS]/libIex-*.dll \
        $$[QT_INSTALL_BINS]/libIlmImf-Imf_*.dll \
        $$[QT_INSTALL_BINS]/libIlmThread-*.dll \
        $$[QT_INSTALL_BINS]/libintl-*.dll \
        $$[QT_INSTALL_BINS]/libjasper-*.dll \
        $$[QT_INSTALL_BINS]/libnettle-*.dll \
        $$[QT_INSTALL_BINS]/liborc-0.*.dll \
        $$[QT_INSTALL_BINS]/libp11-kit-*.dll \
        $$[QT_INSTALL_BINS]/libpcre16-0.dll \
        $$[QT_INSTALL_BINS]/libpng*.dll \
        $$[QT_INSTALL_BINS]/libstdc++-*.dll \
        $$[QT_INSTALL_BINS]/libtasn1-*.dll \
        $$[QT_INSTALL_BINS]/libwinpthread-*.dll \
        $$[QT_INSTALL_BINS]/libxml2-*.dll \
        $$[QT_INSTALL_BINS]/ssleay32.dll \
        $$[QT_INSTALL_BINS]/zlib*.dll

    dllDeps.path = $${PREFIX}

    pluginsPlatform.files = $$[QT_INSTALL_PLUGINS]/platforms/qwindows.dll
    pluginsPlatform.path = $${PREFIX}/platforms
}

!isEmpty(BUILDDOCS):!isEqual(BUILDDOCS, 0) {
    INSTALLS += docs

    docs.files = share/docs_auto/html
    docs.path = $${HTMLDIR}
    docs.CONFIG += no_check_exist
}
