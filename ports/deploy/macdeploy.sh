#!/bin/sh

OPTPATH=/usr/local/opt
CELLARPATH=/usr/local/Cellar
QTVER=$(ls ${CELLARPATH}/qt5 | tail -n 1)
QT5PATH=${CELLARPATH}/qt5/${QTVER}

function scriptdir {
    dir=$(dirname $PWD/$0)
    pushd $dir 1>/dev/null
    echo $PWD
    popd 1>/dev/null
}

function deploy {
    echo Deploying app
    curpath=$(scriptdir)

    ${OPTPATH}/qt5/bin/macdeployqt \
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
    path=$1
    echo Installing missing dependencies

    user=$(whoami)
    group=$(groups $user | awk '{print $1}')
    curpath=$(scriptdir)
    contents=${curpath}/../../StandAlone/webcamoid.app/Contents

    find ${path} \
        -name '*.dylib' -or -name '*.framework' -or -name 'webcamoid' | \
    while read libpath; do
        libpath=${libpath/${path}\//}
        fname=$(basename $libpath)
        echo Solving $libpath

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
                  || "$lib" == /System/Library/Frameworks/*
                  || "$lib" == *$fname* ]]; then
                continue
            fi

            oldpath=${lib%(*}

            echo '    dep ' $oldpath

            if [[ "$lib" == ${CELLARPATH}/*
                  || "$lib" == ${OPTPATH}/* ]]; then
                fname=$(basename $oldpath)

                if [[ "$fname" == *.dylib ]]; then
                    libname=$(echo $fname | awk -F. '{print $1}')
                    fname=$libname*.dylib
                    tfname=$libname.dylib
                else
                    fname=$fname.framework
                    tfname=$fname
                fi

                pkg=${lib/${CELLARPATH}\//}
                pkg=${pkg/${OPTPATH}\//}
                pkg=$(echo $pkg | awk -F/ '{print $1}')
                dest=${contents}/Frameworks

                if [ ! -e ${dest}/$tfname ]; then
                    echo '        copying' "${OPTPATH}/$pkg/lib/$fname"
                    cp -Raf ${OPTPATH}/$pkg/lib/$fname ${dest}/

                    if [ -d ${dest}/$tfname ]; then
                        find ${dest}/$fname \
                            \( -name 'Headers' -or -name '*.prl' \) -delete
                    fi

                    chown -R $user:$group ${dest}/$fname

                    if [ -f ${dest}/$tfname ]; then
                        chmod 644 ${dest}/$fname
                    else
                        find ${dest}/$fname -type d -exec chmod 755 {} \;
                        find ${dest}/$fname -type f -exec chmod 644 {} \;
                    fi
                fi
            fi
        done
    done
}

function solveall {
    curpath=$(scriptdir)
    contents=${curpath}/../../StandAlone/webcamoid.app/Contents
    paths=(Plugins
           Frameworks)

    for path in ${paths[@]}; do
        solvedeps ${contents}/$path
    done
}

function fixlibs {
    path=$1
    echo Fixing dependencies paths

    find ${path} \
        -name '*.dylib' -or -name '*.framework' -or -name 'webcamoid' | \
    while read libpath; do
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
solveall
fixall
