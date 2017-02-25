#!/bin/sh

QTVER=$(ls /usr/local/Cellar/qt5 | tail -n 1)
QT5PATH=/usr/local/Cellar/qt5/${QTVER}
OPTPATH=/usr/local/opt

function scriptdir {
    dir=$(dirname $PWD/$0)
    pushd $dir 1>/dev/null
    echo $PWD
    popd 1>/dev/null
}

function deploy {
    echo Deploying app
    curpath=$(scriptdir)

    ${QT5PATH}/bin/macdeployqt \
        ${curpath}/../../StandAlone/webcamoid.app \
        -always-overwrite \
        -appstore-compliant \
        -qmldir=${curpath}/../.. \
        -libpath=${curpath}/../../libAvKys/Lib
}

function installplugins {
    echo Installing plugins
    curpath=$(scriptdir)

    pushd ${curpath}/../../libAvkys
        contents=${PWD}/../StandAlone/webcamoid.app/Contents
        bundledata=${PWD}/build/bundle-data

        make INSTALL_ROOT="${bundledata}" install

        mkdir -p ${contents}/Resources/qml/AkQml
        cp -rf ${bundledata}/usr/lib/qt/qml/AkQml/* \
               ${contents}/Resources/qml/AkQml

        mkdir -p ${contents}/Plugins/avkys
        cp -rf ${bundledata}/usr/lib/avkys/* \
               ${contents}/Plugins/avkys

        rm -rf ${bundledata}
    popd
}

function solvedeps {
    echo Installing missing dependencies

    user=$(whoami)
    group=$(groups $user | awk '{print $1}')
    frameworks=(QtConcurrent)
    curpath=$(scriptdir)
    contents=${curpath}/../../StandAlone/webcamoid.app/Contents

    for framework in ${frameworks[@]}; do
        path=${contents}/Frameworks/$framework.framework
        mkdir -p ${path}
        cp -Raf ${QT5PATH}/lib/$framework.framework/* ${path}
        find ${path} \( -name 'Headers' -or -name '*.prl' \) -delete
        chown -R $user:$group ${path}
        find ${path} -type d -exec chmod 755 {} \;
        find ${path} -type f -exec chmod 644 {} \;
    done
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
                dir=$(dirname $lib)
                newpath=${oldpath/$dir/$relpath}
                echo '          change path to' $newpath
            elif [[ "$lib" == $OPTPATH\/*$fname* ]]; then
                newpath=$(basename $oldpath)
                changeid=1
                echo '          change id to' $newpath
            elif [[ "$lib" == $OPTPATH* ]]; then
                dir=$(dirname $lib)
                newpath=${oldpath/$dir/$relpath}
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

function fixall {
    curpath=$(scriptdir)
    contents=${curpath}/../../StandAlone/webcamoid.app/Contents
    paths=(MacOS
           Frameworks
           Plugins
           Resources/qml/AkQml)

    for path in ${paths[@]}; do
        fixlibs ${contents}/$path
    done
}

deploy
installplugins
solvedeps
fixall
