#!/bin/sh

APPNAME=webcamoid

function rootdir {
    dir=$(dirname $PWD/$1)/../../..
    pushd $dir 1>/dev/null
    echo $PWD
    popd 1>/dev/null
}

ROOTDIR=$(rootdir $0)

function prepare {
    if [ -e "${ROOTDIR}/build/bundle-data/${APPNAME}" ]; then
        return
    fi

    pushd ${ROOTDIR}
        make INSTALL_ROOT="${ROOTDIR}/build/bundle-data" install
        mv -f "${ROOTDIR}/build/bundle-data/usr" "${ROOTDIR}/build/bundle-data/${APPNAME}"
    popd
}

function listqtmodules {
    bundleData=${ROOTDIR}/build/bundle-data
    scanpaths=(${APPNAME}/bin
               ${APPNAME}/lib)

    for scanpath in ${scanpaths[@]}; do
        find ${bundleData}/${scanpath} -name 'lib*.so*' -or -name ${APPNAME} | \
        while read scanlib; do
            ldd $scanlib | \
            while read lib; do
                lib=$(echo $lib | awk '{print $1}')
                lib=$(basename $lib)
                lib=$(echo $lib | awk -F. '{print $1}')
                lib=${lib:3}

                if [[ "$lib" == Qt5* ]]; then
                    echo $lib
                fi
            done
        done
    done
}

function pluginsdeps {
    bundleData=${ROOTDIR}/build/bundle-data
    libpath=${bundleData}/${APPNAME}/lib

    pluginsMap=('Qt5Core platforms'
                'Qt5Gui accessible iconengines imageformats platforms platforminputcontexts'
                'Qt5Network bearer'
                'Qt5Sql sqldrivers'
                'Qt5Multimedia audio mediaservice playlistformats'
                'Qt5PrintSupport printsupport'
                'Qt5Quick scenegraph qmltooling'
                'Qt5QmlTooling qmltooling'
                'Qt5Declarative qml1tooling'
                'Qt5Positioning position'
                'Qt5Location geoservices'
                'Qt5Sensors sensors sensorgestures'
                'Qt5WebEngine qtwebengine'
                'Qt5WebEngineCore qtwebengine'
                'Qt5WebEngineWidgets qtwebengine'
                'Qt53DRenderer sceneparsers'
                'Qt5TextToSpeech texttospeech'
                'Qt5SerialBus canbus')

    syspluginspath=$(qmake-qt5 -query QT_INSTALL_PLUGINS)

    listqtmodules | sort | uniq | \
    while read mod; do
        for module in "${pluginsMap[@]}"; do
            if [[ "$module" != "$mod "* ]]; then
                continue
            fi

            for plugin in ${module#* }; do
                pluginspath=$syspluginspath/$plugin

                if [ ! -e $pluginspath ]; then
                    continue
                fi

                mkdir -p "$libpath/qt/plugins/$plugin"
                cp -rf ${pluginspath}/* "$libpath/qt/plugins/$plugin"
            done
        done
    done
}

function qmldeps {
    libpath=${ROOTDIR}/build/bundle-data/${APPNAME}/lib
    sysqmlpath=$(qmake-qt5 -query QT_INSTALL_QML)
    qmlsearchdir=(StandAlone/share/qml
                  libAvKys/Plugins)

    for searchpath in "${qmlsearchdir[@]}"; do
        find ${ROOTDIR}/$searchpath -iname '*.qml' -type f -exec grep "^import \w\{1,\}" {} \; | sort | uniq | \
        while read qmlmodule; do
            modulename=$(echo $qmlmodule | awk '{print $2}')
            moduleversion=$(echo $qmlmodule | awk '{print $3}')
            modulepath=$(echo $modulename | tr . / )
            majorversion=$(echo $moduleversion | awk -F. '{print $1}')

            if (( $majorversion > 1 )); then
                modulepath=$modulepath.$majorversion
            fi

            if [[ -e $sysqmlpath/$modulepath
                  && ! -e $libpath/qt/qml/$modulepath ]]; then
                mkdir -p $libpath/qt/qml/$modulepath
                cp -rf "$sysqmlpath/$modulepath"/* "$libpath/qt/qml/$modulepath"
            fi
        done
    done
}

function qtdeps {
    qmldeps
    pluginsdeps
}

function glibdeps {
    which pacman 1>/dev/null 2>/dev/null &&
    (
        pacman -Ql glibc | grep '\w\{1,\}\-[0-9]\{1,\}\.[0-9]\{1,\}\.so' | awk '{print $2}'

        return
    )
    which dpkg-query 1>/dev/null 2>/dev/null &&
    (
        dpkg-query -L libc6  | grep '\w\{1,\}\-[0-9]\{1,\}\.[0-9]\{1,\}\.so' | awk '{print $1}' | sort

        return
    )
    which rpm 1>/dev/null 2>/dev/null &&
    (
        rpm -ql glibc  | grep '\w\{1,\}\-[0-9]\{1,\}\.[0-9]\{1,\}\.so' | awk '{print $1}' | sort

        return
    )
}

function isexcluded {
    lib=$(readlink -f $1)
    exclude=($2)

    for e in ${exclude[@]}; do
        if [[ "$lib" == "$e" ]]; then
            echo 1

            return
        fi
    done

    echo 0
}

function solvedeps {
    path=$1
    echo Installing missing dependencies

    exclude=($(glibdeps))
    user=$(whoami)
    group=$(ls -ld ~ | awk '{print $4}')
    bundleData=${ROOTDIR}/build/bundle-data

    find ${path} -name 'lib*.so*' -or -name ${APPNAME} | \
    while read libpath; do
        libpath=${libpath/${path}\//}
        fname=$(basename $libpath)
        echo Solving $libpath

        where=${path}/$libpath

        ldd $where | \
        while read lib; do
            if [[ "$lib" == *'=>'* ]]; then
                oldpath=$(echo ${lib/=>/} | awk '{print $2}')
            else
                oldpath=$(echo ${lib} | awk '{print $1}')
            fi

            if [ ! -e "$oldpath" ]; then
                continue
            fi

            e=$(isexcluded "$oldpath" "${exclude[*]}")

            if [[ "$e" == 1 ]]; then
                continue
            fi

            depbasename=$(basename $oldpath)
            echo '    dep ' $oldpath

            dest=${bundleData}/${APPNAME}/lib

            if [ ! -e ${dest}/$depbasename ]; then
                echo '        copying' "$oldpath"
                cp -f $oldpath ${dest}/
                chmod +x ${dest}/$depbasename
            fi
        done
    done
}

function solveall {
    bundleData=${ROOTDIR}/build/bundle-data
    paths=(${APPNAME}/bin
           ${APPNAME}/lib)

    for path in ${paths[@]}; do
        solvedeps ${bundleData}/$path
    done
}

function createlauncher {
    launcher=${ROOTDIR}/build/bundle-data/${APPNAME}/${APPNAME}.sh

    cat << EOF > "$launcher"
#!/bin/sh

rootdir () {
    dir=\$(dirname \$PWD/\$1)
    cwd=\$PWD
    cd \$dir 1>/dev/null
        echo \$PWD
    cd \$cwd 1>/dev/null
}

ROOTDIR=\$(rootdir \$0)
export LD_LIBRARY_PATH=\${ROOTDIR}/lib:\$LD_LIBRARY_PATH
export QT_DIR=\${ROOTDIR}/lib/qt
export QT_QPA_PLATFORM_PLUGIN_PATH=\${QT_DIR}/platforms
export QT_PLUGIN_PATH=\${QT_DIR}/plugins
export QML_IMPORT_PATH=\${QT_DIR}/qml
export QML2_IMPORT_PATH=\${QT_DIR}/qml
\${ROOTDIR}/bin/${APPNAME} "\$@"
EOF

    chmod +x "$launcher"
}

function createportable {
    pushd "${ROOTDIR}/build/bundle-data"
        version=$(./${APPNAME}/${APPNAME}.sh --version 2>/dev/null | awk '{print $2}')
        arch=$(uname -m)
        tar -cJf "${ROOTDIR}/ports/deploy/linux/${APPNAME}-portable-${version}-${arch}.tar.xz" ${APPNAME}
    popd
}

function createintaller {
    installerDir=${ROOTDIR}/ports/installer
    dataDir=${installerDir}/packages/com.webcamoidprj.webcamoid/data

    mkdir -p "${dataDir}"
    cp -rf \
        ${ROOTDIR}/build/bundle-data/${APPNAME}/* \
        ${dataDir}

    pushd "${ROOTDIR}/build/bundle-data/${APPNAME}"
        version=$(./${APPNAME}.sh --version 2>/dev/null | awk '{print $2}')
    popd

    arch=$(uname -m)

    binarycreator \
         -c ${installerDir}/config/config.xml \
         -p ${installerDir}/packages \
         ${ROOTDIR}/ports/deploy/linux/${APPNAME}-${version}-${arch}.run

    rm -rf ${dataDir}
}

function package {
    createportable
    createintaller
}

prepare
qtdeps
solveall
createlauncher
package
