#!/bin/sh

CURPATH=$(dirname $0)
QTVER=$(ls /usr/local/Cellar/qt5 | tail -n 1)
QT5PATH=/usr/local/Cellar/qt5/${QTVER}
QT5OPT=/usr/local/opt/qt5

function deploy {
    echo Deploying app...

    ${QT5PATH}/bin/macdeployqt \
        ${CURPATH}/../../StandAlone/webcamoid.app \
        -always-overwrite \
        -appstore-compliant \
        -libpath=${CURPATH}/../../libAvKys/Lib
}

function installplugins {
    pushd ${CURPATH}/../../libAvkys
        make INSTALL_ROOT="${PWD}/bundle-data" install
        cp -Rvf ${PWD}/bundle-data/usr/lib/qt/qml/AkQml \
                ${PWD}/../StandAlone/webcamoid.app/Contents/Resources/qml
        cp -Rvf ${PWD}/bundle-data/usr/lib/avkys \
                ${PWD}/../StandAlone/webcamoid.app/Contents/Plugins
        rm -Rvf ${PWD}/bundle-data
    popd
}

function fixlibs {
    path=$1

    for libpath in $(find ${path} -name '*.dylib' \
                                  -or \
                                  -name '*.framework' \
                                  -or \
                                  -name 'webcamoid'); do
        libpath=${libpath/${path}\//}
        fname=$(basename $libpath)
        echo Fixing $libpath

        if [[ $libpath == *.dylib || $libpath == webcamoid ]]; then
            where=${path}/$libpath
        else
            module=${fname%.framework}
            where=${path}/$libpath/$module
        fi

        otool -L $where | \
        while read lib; do
            if [[ "$lib" == *:*
                  || "$lib" == /usr/lib*
                  || "$lib" == /System/Library/Frameworks/* ]]; then
                continue
            fi

            oldpath=${lib%(*}
            relpath=@executable_path/../Frameworks
            changeid=0
            echo '    dep ' $oldpath

            if [[ "$lib" == $QT5PATH\/lib* ]]; then
                newpath=${oldpath/$QT5PATH\/lib/$relpath}
                echo '          change path to' $newpath
            elif [[ "$lib" == @executable_path/../Frameworks/*$fname* ]]; then
                newpath=${oldpath/@executable_path\/..\/Frameworks/$relpath}
                changeid=1
                echo '          change id to ' $newpath
            elif [[ "$lib" == @executable_path/../Frameworks* ]]; then
                newpath=${oldpath/@executable_path\/..\/Frameworks/$relpath}
                echo '          change path to' $newpath
            elif [[ "$lib" == $QT5OPT* ]]; then
                newpath=$(basename $oldpath)
                changeid=1
                echo '          change id to' $newpath
            else
                continue
            fi

            if [ $changeid -eq 0 ]; then
                install_name_tool -change \
                    $oldpath \
                    $newpath \
                    $where
            else
                install_name_tool -id \
                    $newpath \
                    $where
            fi
        done
    done
}

deploy
installplugins
fixlibs ${CURPATH}/../../StandAlone/webcamoid.app/Contents/MacOS
fixlibs ${CURPATH}/../../StandAlone/webcamoid.app/Contents/Frameworks
fixlibs ${CURPATH}/../../StandAlone/webcamoid.app/Contents/Plugins
