#!/bin/sh

APPNAME=webcamoid
OPTPATH=/usr/local/opt

rootdir() {
    dir=$(dirname $PWD/$1)/../../..
    pushd $dir 1>/dev/null
    echo $PWD
    popd 1>/dev/null
}

ROOTDIR=$(rootdir $0)

deploy() {
    echo Deploying app

    ${OPTPATH}/qt5/bin/macdeployqt \
        ${ROOTDIR}/StandAlone/${APPNAME}.app \
        -always-overwrite \
        -appstore-compliant \
        -qmldir=${ROOTDIR} \
        -libpath=${ROOTDIR}/libAvKys/Lib
}

installplugins() {
    echo Installing plugins

    pushd ${ROOTDIR}/libAvkys
        contents=${ROOTDIR}/StandAlone/${APPNAME}.app/Contents
        bundledata=${ROOTDIR}/build/bundle-data

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

solvedeps() {
    path=$1
    echo Installing missing dependencies

    user=$(whoami)
    group=$(groups $user | awk '{print $1}')
    contents=${ROOTDIR}/StandAlone/${APPNAME}.app/Contents

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

solveall() {
    contents=${ROOTDIR}/StandAlone/${APPNAME}.app/Contents
    paths=(Plugins
           Frameworks)

    for path in ${paths[@]}; do
        solvedeps ${contents}/$path
    done
}

fixlibs() {
    path=$1
    echo Fixing dependencies paths
    contents=${ROOTDIR}/StandAlone/${APPNAME}.app/Contents

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

fixall() {
    contents=${ROOTDIR}/StandAlone/${APPNAME}.app/Contents
    paths=(MacOS
           Frameworks
           Plugins
           Resources/qml/AkQml)

    for path in ${paths[@]}; do
        fixlibs ${contents}/$path
    done
}

createportable() {
    # The DMG creation script is a modification of:
    #
    # https://asmaloney.com/2013/07/howto/packaging-a-mac-os-x-application-using-a-dmg/

    echo Creating dmg
    staggingdir=${ROOTDIR}/build/${APPNAME}

    mkdir -p ${staggingdir}/${APPNAME}.app
    cp -rf \
        ${ROOTDIR}/StandAlone/${APPNAME}.app/* \
        ${staggingdir}/${APPNAME}.app

    program="${staggingdir}/${APPNAME}.app/Contents/MacOS/${APPNAME}"
    version=$("${program}" --version | awk '{print $2}')
    dsize=$(du -sh "${staggingdir}" | sed 's/\([0-9\.]*\)M\(.*\)/\1/')
    dsize=$(echo "${dsize} + 1.0" | bc | awk '{print int($1+0.5)}')
    tmpdmg=${ROOTDIR}/build/${APPNAME}_tmp.dmg
    volname="${APPNAME} ${version}"

    hdiutil create \
        -srcfolder "$staggingdir" \
        -volname "$volname" \
        -fs HFS+ \
        -fsargs "-c c=64,a=16,e=16" \
        -format UDRW \
        -size ${dsize}M \
        "${tmpdmg}"

    device=$(hdiutil attach -readwrite -noverify "${tmpdmg}" | \
             egrep '^/dev/' | sed 1q | awk '{print $1}')

    sleep 2

    cp -vf \
        ${ROOTDIR}/StandAlone/share/icons/webcamoid.icns \
        /Volumes/"${volname}"/.VolumeIcon.icns
    SetFile -c icnC /Volumes/"${volname}"/.VolumeIcon.icns
    SetFile -a C /Volumes/"${volname}"

    pushd /Volumes/"${volname}"
        ln -s /Applications
    popd

    sync

    hdiutil detach "${device}"
    hdiutil convert \
        "$tmpdmg" \
        -format UDZO \
        -imagekey zlib-level=9 \
        -o "${ROOTDIR}/ports/deploy/mac/${APPNAME}-portable-${version}.dmg"
    rm -rf "$tmpdmg"
    rm -rf "$staggingdir"
}

createintaller() {
    installerDir=${ROOTDIR}/ports/installer
    dataDir=${installerDir}/packages/com.webcamoidprj.webcamoid/data
    mkdir -p ${dataDir}/${APPNAME}.app
    cp -rf \
        ${ROOTDIR}/StandAlone/${APPNAME}.app/* \
        ${dataDir}/${APPNAME}.app

    program="${dataDir}/${APPNAME}.app/Contents/MacOS/${APPNAME}"
    version=$("${program}" --version | awk '{print $2}')

    ~/Qt/QtIFW*/bin/binarycreator \
         -c ${installerDir}/config/config.xml \
         -p ${installerDir}/packages \
         ${ROOTDIR}/ports/deploy/mac/${APPNAME}-${version}.dmg

    rm -rf ${ROOTDIR}/ports/deploy/mac/*.app
    rm -rf "$dataDir"
}

package() {
    createportable
    createintaller
}

deploy
installplugins
solveall
fixall
package
