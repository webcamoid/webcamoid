#!/bin/sh

CURPATH=$(dirname $0)
QTVER=$(ls /usr/local/Cellar/qt5 | tail -n 1)
QT5PATH=/usr/local/Cellar/qt5/${QTVER}
QT5OPT=/usr/local/opt/qt5

function deploy {
    echo Deploying app

    ${QT5PATH}/bin/macdeployqt \
        ${CURPATH}/../../StandAlone/webcamoid.app \
        -always-overwrite \
        -appstore-compliant \
        -qmldir=${CURPATH}/../.. \
        -libpath=${CURPATH}/../../libAvKys/Lib
}

function installplugins {
    pushd ${CURPATH}/../../libAvkys
        make INSTALL_ROOT="${PWD}/bundle-data" install
        mkdir -p ${PWD}/../StandAlone/webcamoid.app/Contents/Resources/qml/AkQml
        cp -rvf ${PWD}/bundle-data/usr/lib/qt/qml/AkQml/* \
                ${PWD}/../StandAlone/webcamoid.app/Contents/Resources/qml/AkQml
        mkdir -p ${PWD}/../StandAlone/webcamoid.app/Contents/Plugins/avkys
        cp -rvf ${PWD}/bundle-data/usr/lib/avkys/* \
                ${PWD}/../StandAlone/webcamoid.app/Contents/Plugins/avkys
        rm -rvf ${PWD}/bundle-data/
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
            lib=$(echo $lib | awk '{print $1}')

            if [[ "$lib" == *:*
                  || "$lib" == /usr/lib*
                  || "$lib" == /System/Library/Frameworks/* ]]; then
                continue
            fi

            oldpath=${lib%(*}
            relpath=@executable_path/../Frameworks
            changeid=0
            echo '    dep ' $oldpath

            if [[ "$lib" == libavkys.*.dylib ]]; then
                newpath=@executable_path/../Frameworks/$lib
                echo '          change path to' $newpath
            elif [[ "$lib" == $QT5PATH\/lib* ]]; then
                newpath=${oldpath/$QT5PATH\/lib/$relpath}
                echo '          change path to' $newpath
            elif [[ "$lib" == $QT5OPT\/*$fname* ]]; then
                newpath=$(basename $oldpath)
                changeid=1
                echo '          change id to' $newpath
            elif [[ "$lib" == $QT5OPT* ]]; then
                newpath=${oldpath/$QT5OPT\/lib/$relpath}
                echo '          change path to' $newpath
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
fixlibs ${CURPATH}/../../StandAlone/webcamoid.app/Contents/Resources/qml/AkQml
