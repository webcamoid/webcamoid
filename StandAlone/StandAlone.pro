# Webcamoid, webcam capture application.
# Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

TRANSLATIONS = $$files(share/ts/*.ts)

exists(commons.pri) {
    include(commons.pri)
} else {
    exists(../commons.pri) {
        include(../commons.pri)
    } else {
        error("commons.pri file not found.")
    }
}

!isEmpty(BUILDDOCS):!isEqual(BUILDDOCS, 0) {
    DOCSOURCES = ../$${COMMONS_APPNAME}.qdocconf

    builddocs.input = DOCSOURCES
    builddocs.output = share/docs_auto/html/$${COMMONS_TARGET}.index
    builddocs.commands = $${QDOCTOOL} ${QMAKE_FILE_IN}
    builddocs.variable_out = DOCSOUTPUT
    builddocs.name = Docs ${QMAKE_FILE_IN}
    builddocs.CONFIG += target_predeps

    QMAKE_EXTRA_COMPILERS += builddocs
    PRE_TARGETDEPS += compiler_builddocs_make_all
}

unix {
    MANPAGESOURCES = share/man/man1/$${COMMONS_TARGET}.1

    buildmanpage.input = MANPAGESOURCES
    buildmanpage.output = ${QMAKE_FILE_IN}.gz
    buildmanpage.commands = gzip -fk9 ${QMAKE_FILE_IN}
    buildmanpage.CONFIG += no_link

    QMAKE_EXTRA_COMPILERS += buildmanpage
    PRE_TARGETDEPS += compiler_buildmanpage_make_all
}

CONFIG += qt
!isEmpty(STATIC_BUILD):!isEqual(STATIC_BUILD, 0): CONFIG += static

HEADERS = \
    src/mediatools.h \
    src/videodisplay.h \
    src/videoframe.h \
    src/iconsprovider.h \
    src/audiolayer.h \
    src/videoeffects.h \
    src/mediasource.h \
    src/pluginconfigs.h \
    src/clioptions.h \
    src/recording.h

INCLUDEPATH += \
    ../libAvKys/Lib/src

LIBS += -L../libAvKys/Lib -lavkys
win32: LIBS += -lole32

OTHER_FILES = \
    share/effects.xml
unix: OTHER_FILES += $${MANPAGESOURCES}

QT += qml quick opengl widgets svg

RESOURCES += \
    Webcamoid.qrc \
    qml.qrc \
    translations.qrc \
    share/icons/icons.qrc

SOURCES = \
    src/main.cpp \
    src/mediatools.cpp \
    src/videodisplay.cpp \
    src/videoframe.cpp \
    src/iconsprovider.cpp \
    src/audiolayer.cpp \
    src/videoeffects.cpp \
    src/mediasource.cpp \
    src/pluginconfigs.cpp \
    src/clioptions.cpp \
    src/recording.cpp

lupdate_only {
    SOURCES += $$files(share/qml/*.qml)
}

DESTDIR = $${PWD}

TARGET = $${COMMONS_TARGET}

macx: ICON = share/icons/webcamoid.icns
!unix: RC_ICONS = share/icons/hicolor/256x256/webcamoid.ico

TEMPLATE = app

# http://www.loc.gov/standards/iso639-2/php/code_list.php

CODECFORTR = UTF-8
CODECFORSRC = UTF-8

INSTALLS += target

target.path = $${BINDIR}

!unix {
    INSTALLS += \
        dllDeps \
        pluginsImageFormats \
        pluginsScenegraph \
        pluginsPlatform \
        pluginsQml \
        appIcon

    DLLFILES = \
        \ # Qt
        $$[QT_INSTALL_BINS]/Qt5Core.dll \
        $$[QT_INSTALL_BINS]/Qt5Gui.dll \
        $$[QT_INSTALL_BINS]/Qt5Network.dll \
        $$[QT_INSTALL_BINS]/Qt5OpenGL.dll \
        $$[QT_INSTALL_BINS]/Qt5Qml.dll \
        $$[QT_INSTALL_BINS]/Qt5Quick.dll \
        $$[QT_INSTALL_BINS]/Qt5Svg.dll \
        $$[QT_INSTALL_BINS]/Qt5Widgets.dll \
        \ # System
        $$[QT_INSTALL_BINS]/libEGL.dll \
        $$[QT_INSTALL_BINS]/libGLESv2.dll \
        $$[QT_INSTALL_BINS]/libbz2-*.dll \
        $$[QT_INSTALL_BINS]/libfreetype-*.dll \
        $$[QT_INSTALL_BINS]/libgcc_s_seh-*.dll \
        $$[QT_INSTALL_BINS]/libgcc_s_sjlj-*.dll \
        $$[QT_INSTALL_BINS]/libglib-*.dll \
        $$[QT_INSTALL_BINS]/libharfbuzz-?.dll \
        $$[QT_INSTALL_BINS]/libiconv-*.dll \
        $$[QT_INSTALL_BINS]/libintl-*.dll \
        $$[QT_INSTALL_BINS]/libjpeg-*.dll \
        $$[QT_INSTALL_BINS]/libpcre-*.dll \
        $$[QT_INSTALL_BINS]/libpcre16-*.dll \
        $$[QT_INSTALL_BINS]/libpng16-*.dll \
        $$[QT_INSTALL_BINS]/libstdc++-*.dll \
        $$[QT_INSTALL_BINS]/libwinpthread-*.dll \
        $$[QT_INSTALL_BINS]/zlib1.dll

    isEmpty(FFMPEGLIBS) {
        DLLFILES += \
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
            $$[QT_INSTALL_BINS]/libdcadec.dll \
            $$[QT_INSTALL_BINS]/libeay32.dll \
            $$[QT_INSTALL_BINS]/libexpat-*.dll \
            $$[QT_INSTALL_BINS]/libffi-*.dll \
            $$[QT_INSTALL_BINS]/libfontconfig-*.dll \
            $$[QT_INSTALL_BINS]/libfribidi-*.dll \
            $$[QT_INSTALL_BINS]/libgmp-*.dll \
            $$[QT_INSTALL_BINS]/libgnutls-*.dll \
            $$[QT_INSTALL_BINS]/libgomp-?.dll \
            $$[QT_INSTALL_BINS]/libgsm.dll.*.*.* \
            $$[QT_INSTALL_BINS]/libhogweed-*.dll \
            $$[QT_INSTALL_BINS]/libidn-*.dll \
            $$[QT_INSTALL_BINS]/liblzma-*.dll \
            $$[QT_INSTALL_BINS]/libmodplug-*.dll \
            $$[QT_INSTALL_BINS]/libmp3lame-*.dll \
            $$[QT_INSTALL_BINS]/libnettle-*.dll \
            $$[QT_INSTALL_BINS]/libogg-*.dll \
            $$[QT_INSTALL_BINS]/libopencore-amrnb-*.dll \
            $$[QT_INSTALL_BINS]/libopencore-amrwb-*.dll \
            $$[QT_INSTALL_BINS]/libopenjpeg-*.dll \
            $$[QT_INSTALL_BINS]/libopus-*.dll \
            $$[QT_INSTALL_BINS]/liborc-?.*.dll \
            $$[QT_INSTALL_BINS]/libp11-kit-*.dll \
            $$[QT_INSTALL_BINS]/libschroedinger-*.dll \
            $$[QT_INSTALL_BINS]/libsoxr.dll \
            $$[QT_INSTALL_BINS]/libspeex-*.dll \
            $$[QT_INSTALL_BINS]/libssh.dll \
            $$[QT_INSTALL_BINS]/libtasn1-*.dll \
            $$[QT_INSTALL_BINS]/libtheoradec-*.dll \
            $$[QT_INSTALL_BINS]/libtheoraenc-*.dll \
            $$[QT_INSTALL_BINS]/libvidstab.dll \
            $$[QT_INSTALL_BINS]/libvorbis-*.dll \
            $$[QT_INSTALL_BINS]/libvorbisenc-*.dll \
            $$[QT_INSTALL_BINS]/libvpx.dll.*.*.* \
            $$[QT_INSTALL_BINS]/libwebp-*.dll \
            $$[QT_INSTALL_BINS]/libwebpmux-*.dll \
            $$[QT_INSTALL_BINS]/libx264-*.dll \
            $$[QT_INSTALL_BINS]/libx265.dll \
            $$[QT_INSTALL_BINS]/libxml2-*.dll \
            $$[QT_INSTALL_BINS]/xvidcore.dll
    }

    dllDeps.files = $${DLLFILES}
    dllDeps.path = $${BINDIR}

    pluginsPlatform.files = $$[QT_INSTALL_PLUGINS]/platforms/qwindows.dll
    pluginsPlatform.path = $${BINDIR}/platforms

    pluginsImageFormats.files = $$[QT_INSTALL_PLUGINS]/imageformats/*
    pluginsImageFormats.path = $${BINDIR}/imageformats

    pluginsScenegraph.files = $$[QT_INSTALL_PLUGINS]/scenegraph/*
    pluginsScenegraph.path = $${BINDIR}/scenegraph

    pluginsQml.files = $$[QT_INSTALL_QML]/*
    pluginsQml.path = $${LIBDIR}/qt/qml

    appIcon.files = share/icons/hicolor/256x256/webcamoid.ico
    appIcon.path = $${PREFIX}
}

unix:!macx {
    INSTALLS += \
        manpage \
        appIcon8x8 \
        appIcon16x16 \
        appIcon22x22 \
        appIcon32x32 \
        appIcon48x48 \
        appIcon64x64 \
        appIcon128x128 \
        appIcon256x256 \
        appIconScalable

    manpage.files = share/man/man1/*.1.gz
    manpage.path = $${MANDIR}/man1
    manpage.CONFIG += no_check_exist

    appIcon8x8.files = share/icons/hicolor/8x8/webcamoid.png
    appIcon8x8.path = $${DATAROOTDIR}/icons/hicolor/8x8/apps

    appIcon16x16.files = share/icons/hicolor/16x16/webcamoid.png
    appIcon16x16.path = $${DATAROOTDIR}/icons/hicolor/16x16/apps

    appIcon22x22.files = share/icons/hicolor/22x22/webcamoid.png
    appIcon22x22.path = $${DATAROOTDIR}/icons/hicolor/22x22/apps

    appIcon32x32.files = share/icons/hicolor/32x32/webcamoid.png
    appIcon32x32.path = $${DATAROOTDIR}/icons/hicolor/32x32/apps

    appIcon48x48.files = share/icons/hicolor/48x48/webcamoid.png
    appIcon48x48.path = $${DATAROOTDIR}/icons/hicolor/48x48/apps

    appIcon64x64.files = share/icons/hicolor/64x64/webcamoid.png
    appIcon64x64.path = $${DATAROOTDIR}/icons/hicolor/64x64/apps

    appIcon128x128.files = share/icons/hicolor/128x128/webcamoid.png
    appIcon128x128.path = $${DATAROOTDIR}/icons/hicolor/128x128/apps

    appIcon256x256.files = share/icons/hicolor/256x256/webcamoid.png
    appIcon256x256.path = $${DATAROOTDIR}/icons/hicolor/256x256/apps

    appIconScalable.files = share/icons/hicolor/scalable/webcamoid.svg
    appIconScalable.path = $${DATAROOTDIR}/icons/hicolor/scalable/apps
}

!isEmpty(BUILDDOCS):!isEqual(BUILDDOCS, 0) {
    INSTALLS += docs

    docs.files = share/docs_auto/html
    docs.path = $${HTMLDIR}
    docs.CONFIG += no_check_exist
}
