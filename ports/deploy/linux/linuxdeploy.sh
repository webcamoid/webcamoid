#!/bin/sh

APPNAME=webcamoid
INSTALLERICONSIZE=128

detectqmake() {
    if qmake-qt5 -v 1>/dev/null 2>/dev/null; then
        echo qmake-qt5
    elif qmake -qt=5 -v 1>/dev/null 2>/dev/null; then
        echo qmake -qt=5
    elif qmake -v 1>/dev/null 2>/dev/null; then
        echo qmake
    fi
}

QMAKE=$(detectqmake)

rootdir() {
    if [[ "\$1" == /* ]]; then
        dir=$(dirname $1)/../../..
    else
        dir=$(dirname $PWD/$1)/../../..
    fi

    pushd $dir 1>/dev/null
        echo $PWD
    popd 1>/dev/null
}

ROOTDIR=$(rootdir $0)

prepare() {
    if [ -e "${ROOTDIR}/build/bundle-data/${APPNAME}" ]; then
        return
    fi

    sysqmlpath=$(${QMAKE} -query QT_INSTALL_QML)
    insqmldir=${ROOTDIR}/build/bundle-data/${sysqmlpath#/*}
    dstqmldir=${ROOTDIR}/build/bundle-data/usr/lib/qt/qml

    pushd ${ROOTDIR}
        make INSTALL_ROOT="${ROOTDIR}/build/bundle-data" install

        if [ "$insqmldir" != "$dstqmldir" ]; then
            mkdir -p "$dstqmldir"
            cp -rf "$insqmldir"/* "$dstqmldir/"
        fi

        mv -f "${ROOTDIR}/build/bundle-data/usr" "${ROOTDIR}/build/bundle-data/${APPNAME}"
    popd
}

listqtmodules() {
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

scanimports() {
    find $1 -iname '*.qml' -type f -exec grep "^import \w\{1,\}" {} \;
    find $1 -iname 'qmldir' -type f -exec grep '^depends ' {} \;
}

qmldeps() {
    libpath=${ROOTDIR}/build/bundle-data/${APPNAME}/lib
    sysqmlpath=$(${QMAKE} -query QT_INSTALL_QML)
    qmlsearchdir=(StandAlone/share/qml
                  libAvKys/Plugins
                  build/bundle-data/${APPNAME}/lib/qt/qml)

    for searchpath in "${qmlsearchdir[@]}"; do
        scanimports "${ROOTDIR}/$searchpath" | sort | uniq | \
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

pluginsdeps() {
    bundleData=${ROOTDIR}/build/bundle-data
    libpath=${bundleData}/${APPNAME}/lib

    pluginsMap=('Qt53DRenderer sceneparsers'
                'Qt5Declarative qml1tooling'
                'Qt5EglFSDeviceIntegration egldeviceintegrations'
                'Qt5Gui accessible generic iconengines imageformats platforms platforminputcontexts'
                'Qt5Location geoservices'
                'Qt5Multimedia audio mediaservice playlistformats'
                'Qt5Network bearer'
                'Qt5Positioning position'
                'Qt5PrintSupport printsupport'
                'Qt5QmlTooling qmltooling'
                'Qt5Quick scenegraph qmltooling'
                'Qt5Sensors sensors sensorgestures'
                'Qt5SerialBus canbus'
                'Qt5Sql sqldrivers'
                'Qt5TextToSpeech texttospeech'
                'Qt5WebEngine qtwebengine'
                'Qt5WebEngineCore qtwebengine'
                'Qt5WebEngineWidgets qtwebengine'
                'Qt5XcbQpa xcbglintegrations')

    syspluginspath=$(${QMAKE} -query QT_INSTALL_PLUGINS)

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

qtdeps() {
    qmldeps
    pluginsdeps
}

readdeps() {
    cat "${ROOTDIR}/ports/deploy/linux/$1" | cut -d \# -f 1 | sed '/^$/d' | \
    while read line; do
        echo $line | awk '{print $1}'
    done
}

isexcluded() {
    lib=$1
    exclude=($2)

    for e in ${exclude[@]}; do
        [[ "$lib" =~ $e ]] && return 0
    done

    return 1
}

packinfo() {
    if which pacman 1>/dev/null 2>/dev/null; then
        info=($(LC_ALL=C pacman -Qo $1 | tr ' ' $'\n' | tail -n 2))
        echo ${info[0]} ${info[1]}
    elif which dpkg 1>/dev/null 2>/dev/null; then
        package=$(dpkg -S $1 | awk -F: '{print $1}')
        version=$(dpkg -s $package | grep '^Version:' | awk '{print $2}')
        echo $package $version
    elif which rpm 1>/dev/null 2>/dev/null; then
        rpm -qf $1
    fi
}

solvedeps() {
    path=$1
    echo Installing missing dependencies

    exclude=($(readdeps deps.txt | sort | uniq))
    user=$(whoami)
    group=$(ls -ld ~ | awk '{print $4}')
    bundleData=${ROOTDIR}/build/bundle-data
    depsinfo=${bundleData}/${APPNAME}/share/depsinfo.txt

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

            signature=$(echo "$lib" | cut -d '=' -f 1 | cut -d '(' -f 1 | xargs)

            if isexcluded "$signature" "${exclude[*]}"; then
                continue
            fi

            depbasename=$(basename $oldpath)
            echo '    dep ' $signature

            dest=${bundleData}/${APPNAME}/lib

            if [ ! -e ${dest}/$depbasename ]; then
                echo '        copying' "$oldpath"
                cp -f $oldpath ${dest}/
                chmod +x ${dest}/$depbasename
                echo $(packinfo $oldpath) >> "$depsinfo"
            fi
        done
    done
}

solveall() {
    bundleData=${ROOTDIR}/build/bundle-data
    sysinfo=${bundleData}/${APPNAME}/share/sysinfo.txt
    depsinfo=${bundleData}/${APPNAME}/share/depsinfo.txt
    paths=(${APPNAME}/bin
           ${APPNAME}/lib)

    cat /etc/*-release > "$sysinfo"

    for path in ${paths[@]}; do
        solvedeps ${bundleData}/$path
    done

    cat "$depsinfo" | sort | uniq > "$depsinfo.tmp"
    cat "$depsinfo.tmp" > "$depsinfo"
    rm -f "$depsinfo.tmp"
}

createlauncher() {
    launcher=$1

    cat << EOF > "$launcher"
#!/bin/sh

rootdir() {
    if [[ "\$1" == /* ]]; then
        dirname \$1
    else
        dir=\$(dirname \$PWD/\$1)
        cwd=\$PWD
        cd \$dir 1>/dev/null
            echo \$PWD
        cd \$cwd 1>/dev/null
    fi
}

ROOTDIR=\$(rootdir \$0)
export PATH="\${ROOTDIR}"/bin:\$PATH
export LD_LIBRARY_PATH="\${ROOTDIR}"/lib:\$LD_LIBRARY_PATH
export QT_DIR="\${ROOTDIR}"/lib/qt
export QT_QPA_PLATFORM_PLUGIN_PATH=\${QT_DIR}/platforms
export QT_PLUGIN_PATH=\${QT_DIR}/plugins
export QML_IMPORT_PATH=\${QT_DIR}/qml
export QML2_IMPORT_PATH=\${QT_DIR}/qml
#export QT_DEBUG_PLUGINS=1
${APPNAME} "\$@"
EOF

    chmod +x "$launcher"
}

createportable() {
    pushd "${ROOTDIR}/build/bundle-data/${APPNAME}"
        version=$(./${APPNAME}.sh --version 2>/dev/null | awk '{print $2}')
    popd

    arch=$(uname -m)

    rm -vf "${ROOTDIR}/ports/deploy/linux/${APPNAME}-portable-${version}-${arch}.tar.xz"
    createlauncher "${ROOTDIR}/build/bundle-data/${APPNAME}/${APPNAME}.sh"

    pushd "${ROOTDIR}/build/bundle-data"
        tar -cJf "${ROOTDIR}/ports/deploy/linux/${APPNAME}-portable-${version}-${arch}.tar.xz" ${APPNAME}
    popd
}

detectqtifw() {
    if which binarycreator 1>/dev/null 2>/dev/null; then
        which binarycreator
    else
        ls ~/Qt/QtIFW*/bin/binarycreator 2>/dev/null | sort -V | tail -n 1
    fi
}

readchangelog() {
    version=$2
    start=0

    cat $1 | \
    while read line; do
        if [ "$line" == "Webcamoid $version:" ]; then
            start=1

            continue
        fi

        if [[ "$start" == 1 ]]; then
            # Skip first blank line.
            start=2
        elif [[ "$start" == 2 ]]; then
            [[ "$line" == Webcamoid\ *: ]] && break

            echo $line
        fi
    done
}

createinstaller() {
    bincreator=$(detectqtifw)

    if [ -z "$bincreator" ]; then
        return
    fi

    pushd "${ROOTDIR}/build/bundle-data/${APPNAME}"
        version=$(./${APPNAME}.sh --version 2>/dev/null | awk '{print $2}')
    popd

    arch=$(uname -m)

    # Create layout
    configdir=${ROOTDIR}/build/bundle-data/installer/config
    packagedir=${ROOTDIR}/build/bundle-data/installer/packages/com.${APPNAME}prj.${APPNAME}
    mkdir -p "$configdir"
    mkdir -p "$packagedir"/{data,meta}
    cp -vf \
        "${ROOTDIR}/build/bundle-data/${APPNAME}/$appdir/share/icons/hicolor/${INSTALLERICONSIZE}x${INSTALLERICONSIZE}/apps/${APPNAME}.png" \
        "$configdir/"
    cp -vf \
        "${ROOTDIR}/COPYING" \
        "$packagedir/meta/COPYING.txt"
    cp -rf \
        "${ROOTDIR}/build/bundle-data/${APPNAME}/"* \
        "$packagedir/data/"

    cat << EOF > "$configdir/config.xml"
<?xml version="1.0" encoding="UTF-8"?>
<Installer>
    <Name>Webcamoid</Name>
    <Version>$version</Version>
    <Title>Webcamoid, The ultimate webcam suite!</Title>
    <Publisher>Webcamoid</Publisher>
    <ProductUrl>https://webcamoid.github.io/</ProductUrl>
    <InstallerWindowIcon>webcamoid</InstallerWindowIcon>
    <InstallerApplicationIcon>webcamoid</InstallerApplicationIcon>
    <Logo>webcamoid</Logo>
    <TitleColor>#3F1F7F</TitleColor>
    <RunProgram>@TargetDir@/webcamoid.sh</RunProgram>
    <RunProgramDescription>Launch Webcamoid now!</RunProgramDescription>
    <StartMenuDir>Webcamoid</StartMenuDir>
    <MaintenanceToolName>WebcamoidMaintenanceTool</MaintenanceToolName>
    <AllowNonAsciiCharacters>true</AllowNonAsciiCharacters>
    <TargetDir>@HomeDir@/${APPNAME}</TargetDir>
</Installer>
EOF

    cat << EOF > "$packagedir/meta/installscript.qs"
function Component()
{
}

Component.prototype.beginInstallation = function()
{
    component.beginInstallation();
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    component.addOperation("InstallIcons", "@TargetDir@/share/icons" );
    component.addOperation("CreateDesktopEntry",
                            "Webcamoid.desktop",
                            "Name=Webcamoid\n"
                            + "GenericName=Webcam Capture Software\n"
                            + "GenericName[ca]=Programari de Captura de Càmera web\n"
                            + "GenericName[de]=Webcam-Capture-Software\n"
                            + "GenericName[el]=κάμερα συλλαμβάνει το λογισμικό\n"
                            + "GenericName[es]=Programa para Captura de la Webcam\n"
                            + "GenericName[fr]=Logiciel de Capture Webcam\n"
                            + "GenericName[gl]=Programa de Captura de Webcam\n"
                            + "GenericName[it]=Webcam Capture Software\n"
                            + "GenericName[ja]=ウェブカメラのキャプチャソフトウェア\n"
                            + "GenericName[ko]=웹캠 캡처 소프트웨어\n"
                            + "GenericName[pt]=Software de Captura de Webcam\n"
                            + "GenericName[ru]=Веб-камера захвата программного обеспечения\n"
                            + "GenericName[zh_CN]=摄像头捕捉软件\n"
                            + "GenericName[zh_TW]=攝像頭捕捉軟件\n"
                            + "Comment=Take photos and record videos with your webcam\n"
                            + "Comment[ca]=Fer fotos i gravar vídeos amb la seva webcam\n"
                            + "Comment[de]=Maak foto's en video's opnemen met uw webcam\n"
                            + "Comment[el]=Τραβήξτε φωτογραφίες και εγγραφή βίντεο με την κάμερα σας\n"
                            + "Comment[es]=Tome fotos y grabe videos con su camara web\n"
                            + "Comment[fr]=Prenez des photos et enregistrer des vidéos avec votre webcam\n"
                            + "Comment[gl]=Facer fotos e gravar vídeos coa súa cámara web\n"
                            + "Comment[it]=Scatta foto e registrare video con la tua webcam\n"
                            + "Comment[ja]=ウェブカメラで写真や記録ビデオを撮影\n"
                            + "Comment[ko]=웹캠으로 사진과 기록 비디오를 촬영\n"
                            + "Comment[pt]=Tirar fotos e gravar vídeos com sua webcam\n"
                            + "Comment[ru]=Возьмите фотографии и записывать видео с веб-камеры\n"
                            + "Comment[zh_CN]=拍摄照片和录制视频与您的摄像头\n"
                            + "Comment[zh_TW]=拍攝照片和錄製視頻與您的攝像頭\n"
                            + "Keywords=photo;video;webcam;\n"
                            + "Exec=" + installer.value("RunProgram") + "\n"
                            + "Icon=webcamoid\n"
                            + "Terminal=false\n"
                            + "Type=Application\n"
                            + "Categories=AudioVideo;Player;Qt;\n"
                            + "StartupNotify=true\n");
}
EOF

    cat << EOF > "$packagedir/meta/package.xml"
<?xml version="1.0"?>
<Package>
    <DisplayName>Webcamoid</DisplayName>
    <Description>The ultimate webcam suite</Description>
    <Version>$version</Version>
    <ReleaseDate>$(date "+%Y-%m-%d")</ReleaseDate>
    <Name>com.${APPNAME}prj.${APPNAME}</Name>
    <Licenses>
        <License name="GNU General Public License v3.0" file="COPYING.txt" />
    </Licenses>
    <Script>installscript.qs</Script>
    <UpdateText>
$(readchangelog "${ROOTDIR}/ChangeLog" $version | sed '$ d')
    </UpdateText>
    <Default>true</Default>
    <ForcedInstallation>true</ForcedInstallation>
    <Essential>false</Essential>
</Package>
EOF

    # Remove old file
    rm -vf "${ROOTDIR}/ports/deploy/linux/${APPNAME}-${version}-${arch}.run"

    ${bincreator} \
         -c "$configdir/config.xml" \
         -p "${ROOTDIR}/build/bundle-data/installer/packages" \
         ${ROOTDIR}/ports/deploy/linux/${APPNAME}-${version}-${arch}.run
}

createappimage() {
    url="https://github.com/probonopd/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
    dstdir=~/.local/bin

    if [ ! -e $dstdir/appimagetool ]; then
        mkdir -p $dstdir
        wget -c -O $dstdir/appimagetool $url
        chmod a+x $dstdir/appimagetool
    fi

    pushd "${ROOTDIR}/build/bundle-data/${APPNAME}"
        version=$(./${APPNAME}.sh --version 2>/dev/null | awk '{print $2}')
    popd

    arch=$(uname -m)

    appdir=${ROOTDIR}/build/bundle-data/${APPNAME}-${version}-${arch}.AppDir
    mkdir -p "$appdir"
    cp -rf "${ROOTDIR}/build/bundle-data/${APPNAME}"/{bin,lib,share} "$appdir"/
    createlauncher "$appdir/AppRun"

    cat << EOF > "$appdir/${APPNAME}.desktop"
[Desktop Entry]
Name=${APPNAME}
Exec=${APPNAME}
Icon=${APPNAME}
EOF

    cp -vf "$appdir/share/icons/hicolor/256x256/apps/${APPNAME}.png" "$appdir/"

    pushd "${ROOTDIR}/ports/deploy/linux"
        $dstdir/appimagetool "$appdir"
    popd
}

package() {
    createportable
    createinstaller
    createappimage
}

prepare
qtdeps
solveall
package
