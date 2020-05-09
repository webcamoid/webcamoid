# Webcamoid, webcam capture application.
# Copyright (C) 2015  Gonzalo Exequiel Pedone
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

TRANSLATIONS_PRI = $${PWD}/../translations.pri

exists(translations.qrc) {
    TRANSLATIONS = $$files(share/ts/*.ts)
    RESOURCES += translations.qrc
}

COMMONS_APPNAME = Webcamoid
COMMONS_TARGET = $$lower($${COMMONS_APPNAME})

exists(commons.pri) {
    include(commons.pri)
} else {
    exists(../commons.pri) {
        include(../commons.pri)
    } else {
        error("commons.pri file not found.")
    }
}

!isEmpty(BUILDDOCS): !isEqual(BUILDDOCS, 0) {
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

unix: !android: !macx | !isEmpty(NOAPPBUNDLE) {
    MANPAGESOURCES = share/man/$${COMMONS_TARGET}.1
    MANPAGEOUT = $${OUT_PWD}/$${MANPAGESOURCES}.gz

    buildmanpage.input = MANPAGESOURCES
    buildmanpage.output = $${MANPAGEOUT}
    buildmanpage.commands = gzip -c9 ${QMAKE_FILE_IN} > $${MANPAGEOUT}
    buildmanpage.clean = dummy_file
    buildmanpage.CONFIG += no_link
    QMAKE_EXTRA_COMPILERS += buildmanpage
    PRE_TARGETDEPS += compiler_buildmanpage_make_all
}

CONFIG += qt link_prl
macx: CONFIG -= app_bundle
!isEmpty(STATIC_BUILD):!isEqual(STATIC_BUILD, 0): CONFIG += static

DAILY_BUILD = $$(DAILY_BUILD)
!isEmpty(DAILY_BUILD): DEFINES += DAILY_BUILD

HEADERS = \
    src/mediatools.h \
    src/videodisplay.h \
    src/iconsprovider.h \
    src/audiolayer.h \
    src/videoeffects.h \
    src/pluginconfigs.h \
    src/clioptions.h \
    src/recording.h \
    src/updates.h \
    src/videolayer.h

INCLUDEPATH += \
    ../libAvKys/Lib/src

LIBS += -L$${OUT_PWD}/../libAvKys/Lib/$${BIN_DIR} -l$$qtLibraryTarget(avkys)
win32: LIBS += -lole32

OTHER_FILES = \
    $$files(share/qml/*.qml) \
    $$files(share/themes/WebcamoidTheme/*.qml) \
    $$files(share/themes/WebcamoidTheme/Private/*.qml)

unix: OTHER_FILES += $${MANPAGESOURCES}
macx: OTHER_FILES += Info.plist

QT += \
    opengl \
    qml \
    quick \
    quickcontrols2 \
    svg \
    widgets

RESOURCES += \
    Webcamoid.qrc \
    DefaultTheme.qrc \
    qml.qrc \
    icons.qrc

SOURCES = \
    src/main.cpp \
    src/mediatools.cpp \
    src/videodisplay.cpp \
    src/iconsprovider.cpp \
    src/audiolayer.cpp \
    src/videoeffects.cpp \
    src/pluginconfigs.cpp \
    src/clioptions.cpp \
    src/recording.cpp \
    src/updates.cpp \
    src/videolayer.cpp

lupdate_only {
    SOURCES += $$files(share/qml/*.qml)
    SOURCES += $$files(share/themes/WebcamoidTheme/*.qml)
    SOURCES += $$files(share/themes/WebcamoidTheme/Private/*.qml)
}

QML_IMPORT_PATH += $$PWD/../libAvKys/Lib/share/qml

DESTDIR = $${OUT_PWD}/$${BIN_DIR}

TARGET = $${COMMONS_TARGET}

macx: ICON = share/themes/WebcamoidTheme/icons/webcamoid.icns
!unix: RC_ICONS = share/themes/WebcamoidTheme/icons/hicolor/256x256/webcamoid.ico

TEMPLATE = app

CODECFORTR = UTF-8
CODECFORSRC = UTF-8

INSTALLS += target
target.path = $${BINDIR}

unix: !android: !macx | !isEmpty(NOAPPBUNDLE) {
    INSTALLS += manpage
    manpage.files = $${OUT_PWD}/share/man/$${COMMONS_TARGET}.1.gz
    manpage.path = $${MANDIR}/man1
    manpage.CONFIG += no_check_exist
}

win32 {
    INSTALLS += appIcon
    appIcon.files = share/themes/WebcamoidTheme/icons/hicolor/256x256/webcamoid.ico
    appIcon.path = $${PREFIX}
} else: macx: isEmpty(NOAPPBUNDLE) {
    INSTALLS += appIcon
    appIcon.files = share/themes/WebcamoidTheme/icons/webcamoid.icns
    appIcon.path = $${DATAROOTDIR}
} else: unix: !android: !macx {
    INSTALLS += \
        appIcon8x8 \
        appIcon16x16 \
        appIcon22x22 \
        appIcon32x32 \
        appIcon48x48 \
        appIcon64x64 \
        appIcon128x128 \
        appIcon256x256 \
        appIconScalable

    appIcon8x8.files = share/themes/WebcamoidTheme/icons/hicolor/8x8/webcamoid.png
    appIcon8x8.path = $${DATAROOTDIR}/icons/hicolor/8x8/apps

    appIcon16x16.files = share/themes/WebcamoidTheme/icons/hicolor/16x16/webcamoid.png
    appIcon16x16.path = $${DATAROOTDIR}/icons/hicolor/16x16/apps

    appIcon22x22.files = share/themes/WebcamoidTheme/icons/hicolor/22x22/webcamoid.png
    appIcon22x22.path = $${DATAROOTDIR}/icons/hicolor/22x22/apps

    appIcon32x32.files = share/themes/WebcamoidTheme/icons/hicolor/32x32/webcamoid.png
    appIcon32x32.path = $${DATAROOTDIR}/icons/hicolor/32x32/apps

    appIcon48x48.files = share/themes/WebcamoidTheme/icons/hicolor/48x48/webcamoid.png
    appIcon48x48.path = $${DATAROOTDIR}/icons/hicolor/48x48/apps

    appIcon64x64.files = share/themes/WebcamoidTheme/icons/hicolor/64x64/webcamoid.png
    appIcon64x64.path = $${DATAROOTDIR}/icons/hicolor/64x64/apps

    appIcon128x128.files = share/themes/WebcamoidTheme/icons/hicolor/128x128/webcamoid.png
    appIcon128x128.path = $${DATAROOTDIR}/icons/hicolor/128x128/apps

    appIcon256x256.files = share/themes/WebcamoidTheme/icons/hicolor/256x256/webcamoid.png
    appIcon256x256.path = $${DATAROOTDIR}/icons/hicolor/256x256/apps

    appIconScalable.files = share/themes/WebcamoidTheme/icons/hicolor/scalable/webcamoid.svg
    appIconScalable.path = $${DATAROOTDIR}/icons/hicolor/scalable/apps
}

!isEmpty(BUILDDOCS): !isEqual(BUILDDOCS, 0) {
    INSTALLS += docs
    docs.files = share/docs_auto/html
    docs.path = $${HTMLDIR}
    docs.CONFIG += no_check_exist
}

!macx | !isEmpty(NOAPPBUNDLE) {
    INSTALLS += license
    license.files = ../COPYING
    license.path = $${LICENSEDIR}
}

unix: !android: !macx {
    INSTALLS += desktop
    desktop.files = ../$${COMMONS_TARGET}.desktop
    desktop.path = $${DATAROOTDIR}/applications
}

macx: isEmpty(NOAPPBUNDLE) {
    INSTALLS += infoPlist
    infoPlist.files = Info.plist
    infoPlist.path = $${EXECPREFIX}
}

android {
    QT += concurrent xml androidextras

    DISTFILES += \
        share/android/AndroidManifest.xml \
        share/android/res/drawable-hdpi/icon.png \
        share/android/res/drawable-ldpi/icon.png \
        share/android/res/drawable-mdpi/icon.png \
        share/android/res/drawable-xhdpi/icon.png \
        share/android/res/drawable-xxhdpi/icon.png \
        share/android/res/drawable-xxxhdpi/icon.png \
        share/android/res/values/libs.xml

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/share/android

    INSTALLS += \
        androidFiles

    androidFiles.files = share/android/*
    androidFiles.path = $${EXECPREFIX}
}
