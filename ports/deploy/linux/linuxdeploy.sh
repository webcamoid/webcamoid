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
    pushd ${ROOTDIR}
        make INSTALL_ROOT="${ROOTDIR}/build/bundle-data" install
        mv -f "${ROOTDIR}/build/bundle-data/usr" "${ROOTDIR}/build/bundle-data/${APPNAME}"
    popd
}

function solvedeps {
    path=$1
    echo Installing missing dependencies

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

function rootdir {
    dir=\$(dirname \$PWD/\$1)
    pushd \$dir 1>/dev/null
    echo \$PWD
    popd 1>/dev/null
}

ROOTDIR=\$(rootdir \$0)
export LD_LIBRARY_PATH=\${ROOTDIR}/lib:\$LD_LIBRARY_PATH
\${ROOTDIR}/bin/webcamoid "\$@"
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
solveall
createlauncher
package
