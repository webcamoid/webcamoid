#!/bin/sh

QTVER=$(ls /usr/local/Cellar/qt5 | tail -n 1)
QT5PATH=/usr/local/Cellar/qt5/${QTVER}

${QT5PATH}/bin/macdeployqt \
    ../../StandAlone/webcamoid.app \
    -always-overwrite \
    -appstore-compliant \
    -qmldir=../../libAvKys \
    -libpath=../../libAvKys/Lib

# Fix QtCore
install_name_tool -id \
    @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore \
    ../../StandAlone/webcamoid.app/Contents/Frameworks/QtCore.framework/Versions/5/QtCore

install_name_tool -change \
    ${QT5PATH}/lib/QtCore.framework/Versions/5/QtCore \
    @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore \
    ../../StandAlone/webcamoid.app/Contents/MacOs/webcamoid

# Fix QtGui
install_name_tool -id \
    @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui \
    ../../StandAlone/webcamoid.app/Contents/Frameworks/QtGui.framework/Versions/5/QtGui

install_name_tool -change \
    ${QT5PATH}/lib/QtGui.framework/Versions/5/QtGui \
    @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui \
    ../../StandAlone/webcamoid.app/Contents/MacOs/webcamoid
