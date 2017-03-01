#!/bin/sh

APPNAME=webcamoid
APPVERSION=8.0.0
OPTPATH=/usr/local/opt

function scriptdir {
    dir=$(dirname $PWD/$1)
    pushd $dir 1>/dev/null
    echo $PWD
    popd 1>/dev/null
}

CURPATH=$(scriptdir $0)

function deploy {
    echo Deploying app

    ${OPTPATH}/qt5/bin/macdeployqt \
        ${CURPATH}/../../StandAlone/${APPNAME}.app \
        -always-overwrite \
        -appstore-compliant \
        -qmldir=${CURPATH}/../.. \
        -libpath=${CURPATH}/../../libAvKys/Lib
}

function installplugins {
    echo Installing plugins

    pushd ${CURPATH}/../../libAvkys
        contents=${PWD}/../StandAlone/${APPNAME}.app/Contents
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
    contents=${CURPATH}/../../StandAlone/${APPNAME}.app/Contents

    find ${path} \
        -name '*.dylib' -or -name '*.framework' -or -name ${APPNAME} | \
    while read libpath; do
        libpath=${libpath/${path}\//}
        fname=$(basename $libpath)
        echo Solving $libpath

        if [[ $libpath == *.dylib || $libpath == ${APPNAME} ]]; then
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
            depbasename=$(basename $oldpath)
            deplibname=$(echo $depbasename | awk -F. '{print $1}')
            deplibdir=$(dirname $oldpath)

            echo '    dep ' $oldpath

            if [[ "$deplibdir" == '.' || "$deplibdir" == @* ]]; then
                continue
            fi

            if [[ "$depbasename" == *.dylib ]]; then
                fname=$deplibname*.dylib
                tfname=$deplibname.dylib
            else
                fname=$deplibname.framework
                tfname=$fname
                deplibdir=${deplibdir/\/$tfname/:}
                deplibdir=$(echo $deplibdir | awk -F: '{print $1}')
            fi

            dest=${contents}/Frameworks

            if [ ! -e ${dest}/$tfname ]; then
                echo '        copying' "${deplibdir}/$fname"
                cp -Raf ${deplibdir}/$fname ${dest}/

                if [ -d ${dest}/$tfname ]; then
                    find ${dest}/$fname -path '*/Headers/*' -delete
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
        done
    done
}

function solveall {
    contents=${CURPATH}/../../StandAlone/${APPNAME}.app/Contents
    paths=(Plugins
           Frameworks)

    for path in ${paths[@]}; do
        solvedeps ${contents}/$path
    done
}

function fixlibs {
    path=$1
    echo Fixing dependencies paths
    contents=${CURPATH}/../../StandAlone/${APPNAME}.app/Contents

    find ${path} \
        -name '*.dylib' -or -name '*.framework' -or -name ${APPNAME} | \
    while read libpath; do
        libpath=${libpath/${path}\//}
        fname=$(basename $libpath)
        libname=$(echo $fname | awk -F. '{print $1}')
        echo Fixing $libpath

        if [[ $libpath == *.dylib || $libpath == ${APPNAME} ]]; then
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
            depbasename=$(basename $oldpath)
            deplibname=$(echo $depbasename | awk -F. '{print $1}')
            relpath=@executable_path/../Frameworks
            echo '    dep ' $oldpath

            if [[ $libname == $deplibname ]]; then
                olddirpath=$(dirname $oldpath)

                if [[ $olddirpath != '.' && $olddirpath != $relpath/* ]]; then
                    tname=$depbasename

                    if [[ "$tname" != *.dylib ]]; then
                        tname=$tname.framework
                    fi

                    if [ -e $contents/Frameworks/$tname ]; then
                        if [[ "$tname" == *.dylib ]]; then
                            newpath=$relpath/$tname
                        else
                            rel=${oldpath/$tname\//:}
                            rel=$(echo $rel | awk -F: '{print $2}')
                            newpath=$relpath/$tname/$rel
                        fi
                    else
                        newpath=$depbasename
                    fi

                    echo '          change id to' $newpath

                    install_name_tool -id \
                        $newpath \
                        $where
                fi
            else
                if [[ "$depbasename" == *.dylib ]]; then
                    newpath=$relpath/$depbasename
                else
                    rel=${oldpath/$depbasename\.framework\//:}
                    rel=$(echo $rel | awk -F: '{print $2}')
                    newpath=$relpath/$depbasename.framework/$rel
                fi

                if [[ $newpath == $oldpath ]]; then
                    continue
                fi

                echo '          change path to' $newpath

                install_name_tool -change \
                    $oldpath \
                    $newpath \
                    $where
            fi
        done
    done
}

function fixall {
    contents=${CURPATH}/../../StandAlone/${APPNAME}.app/Contents
    paths=(MacOS
           Frameworks
           Plugins
           Resources/qml/AkQml)

    for path in ${paths[@]}; do
        fixlibs ${contents}/$path
    done
}

function package {
    cp -rvf \
        ${CURPATH}/../../StandAlone/${APPNAME}.app/* \
        ${CURPATH}/../../ports/installer/packages/com.webcamoidprj.webcamoid/data/${APPNAME}.app

    ~/Qt/QtIFW*/bin/binarycreator \
        -c ${CURPATH}/../../ports/installer/config/config.xml \
        -p ${CURPATH}/../../ports/installer/packages \
        ${CURPATH}/../../ports/deploy/${APPNAME}-${APPVERSION}.dmg
}

deploy
installplugins
solveall
fixall
package
