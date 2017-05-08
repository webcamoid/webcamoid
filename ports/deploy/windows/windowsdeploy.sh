#!/bin/sh

APPNAME=webcamoid
INSTALLERICONSIZE=128

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

detectarch() {
    arch=$(file -b "${ROOTDIR}/StandAlone/${APPNAME}.exe" | grep x86-64)

    if [ -z "$arch" ]; then
        echo win32
    else
        echo win64
    fi
}

ARCH=$(detectarch)

detectsysdir() {
    case "${ARCH}" in
        win32) echo /usr/i686-w64-mingw32 ;;
        *) echo /usr/x86_64-w64-mingw32 ;;
    esac
}

SYSDIR=$(detectsysdir)

readversion() {
    wineRootDir=Z:${ROOTDIR//\//\\}
    mkdir -p "${ROOTDIR}/build"

    cat << EOF > "${ROOTDIR}/build/version.bat"
@echo off
SET PATH=${wineRootDir}\StandAlone;${wineRootDir}\libAvKys\Lib;Z:\usr\i686-w64-mingw32\bin;%PATH%
@echo on
webcamoid --version > ${wineRootDir}\build\version.txt
EOF

    wineconsole "${ROOTDIR}/build/version.bat" 2>/dev/null
    cat "${ROOTDIR}/build/version.txt" | awk '{print $2}' | tr -d '[:space:]'
    rm "${ROOTDIR}/build/version".{bat,txt}
}

VERSION=$(readversion)
QMAKE=${SYSDIR}/lib/qt/bin/qmake

prepare() {
    if [ -e "${ROOTDIR}/build/bundle-data/${APPNAME}" ]; then
        return
    fi

    pushd ${ROOTDIR}
        make INSTALL_ROOT="${ROOTDIR}/build/bundle-data" install
    popd

    cp -vf ${SYSDIR}/bin/libEGL.dll "${ROOTDIR}/build/bundle-data/${APPNAME}/bin/"
    cp -vf ${SYSDIR}/bin/libGLESv2.dll "${ROOTDIR}/build/bundle-data/${APPNAME}/bin/"
    cp -vf ${SYSDIR}/bin/D3DCompiler_*.dll "${ROOTDIR}/build/bundle-data/${APPNAME}/bin/"
}

listdependencies() {
    bundleData=${ROOTDIR}/build/bundle-data
    scanpaths=(${APPNAME}/bin
               ${APPNAME}/lib)

    for scanpath in ${scanpaths[@]}; do
        find ${bundleData}/${scanpath} \( -iname '*.dll' -or -iname '*.exe' \) | \
        while read scanlib; do
            ${SYSDIR}/bin/objdump -x $scanlib | grep 'DLL Name' | awk '{print $3}'
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
    binpath=${bundleData}/${APPNAME}/bin

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

    listdependencies | sort | uniq | \
    while read mod; do
        if [[ "$mod" != Qt5* ]]; then
            continue
        fi

        modname=$(echo $mod | awk -F. '{print $1}')

        for module in "${pluginsMap[@]}"; do
            if [[ "$module" != "$modname "* ]]; then
                continue
            fi

            for plugin in ${module#* }; do
                pluginspath=$syspluginspath/$plugin

                if [ ! -e $pluginspath ]; then
                    continue
                fi

                mkdir -p "$binpath/$plugin"
                cp -rf ${pluginspath}/* "$binpath/$plugin/"
            done
        done
    done
}

qtdeps() {
    qmldeps
    pluginsdeps
}

solvedeps() {
    path=$1
    echo Installing missing dependencies

    bundleData=${ROOTDIR}/build/bundle-data

    find "${path}" \( -iname '*.dll' -or -iname '*.exe' \) | \
    while read libpath; do
        libpath=${libpath/${path}\//}
        fname=$(basename $libpath)
        echo Solving $libpath

        where=${path}/$libpath

        ${SYSDIR}/bin/objdump -x $where | grep 'DLL Name' | awk '{print $3}' | \
        while read lib; do
            oldpath=${SYSDIR}/bin/${lib}

            if [ ! -e "$oldpath" ]; then
                continue
            fi

            echo '    dep ' $lib

            dest=${bundleData}/${APPNAME}/bin

            if [ ! -e ${dest}/$lib ]; then
                echo '        copying' "$oldpath"
                cp -f $oldpath ${dest}/
            fi
        done
    done
}

solveall() {
    bundleData=${ROOTDIR}/build/bundle-data
    paths=(${APPNAME}/bin
           ${APPNAME}/lib)

    for path in ${paths[@]}; do
        solvedeps ${bundleData}/$path
    done
}

createlauncher() {
    cat << EOF > "${ROOTDIR}/build/bundle-data/${APPNAME}/${APPNAME}.bat"
@echo off

rem set QT_QUICK_BACKEND=software
start /b "" "%~dp0bin\webcamoid" -q "%~dp0lib\qt\qml" -p "%~dp0lib\avkys" -c "%~dp0share\config"
EOF
}

createportable() {
    rm -vf "${ROOTDIR}/ports/deploy/windows/${APPNAME}-portable-${VERSION}-${ARCH}.zip"

    pushd "${ROOTDIR}/build/bundle-data"
        zip -r9 "${ROOTDIR}/ports/deploy/windows/${APPNAME}-portable-${VERSION}-${ARCH}.zip" ${APPNAME}
    popd
}

detectqtifw() {
    ls ~/.wine/drive_c/Qt/QtIFW2.0.3/bin/binarycreator.exe 2>/dev/null | sort -V | tail -n 1
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

    # Create layout
    configdir=${ROOTDIR}/build/bundle-data/installer/config
    packagedir=${ROOTDIR}/build/bundle-data/installer/packages/com.${APPNAME}prj.${APPNAME}
    mkdir -p "$configdir"
    mkdir -p "$packagedir"/{data,meta}
    cp -vf \
        "${ROOTDIR}/StandAlone/share/icons/hicolor/${INSTALLERICONSIZE}x${INSTALLERICONSIZE}/${APPNAME}.ico" \
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
    <Version>${VERSION}</Version>
    <Title>Webcamoid, The ultimate webcam suite!</Title>
    <Publisher>Webcamoid</Publisher>
    <ProductUrl>https://webcamoid.github.io/</ProductUrl>
    <InstallerWindowIcon>webcamoid</InstallerWindowIcon>
    <InstallerApplicationIcon>webcamoid</InstallerApplicationIcon>
    <Logo>webcamoid</Logo>
    <TitleColor>#3F1F7F</TitleColor>
    <RunProgram>@TargetDir@/bin/webcamoid.exe</RunProgram>
    <RunProgramDescription>Launch Webcamoid now!</RunProgramDescription>
    <StartMenuDir>Webcamoid</StartMenuDir>
    <MaintenanceToolName>WebcamoidMaintenanceTool</MaintenanceToolName>
    <AllowNonAsciiCharacters>true</AllowNonAsciiCharacters>
    <TargetDir>@ApplicationsDir@/${APPNAME}</TargetDir>
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

    // Create shortcuts.
    var installDir = ["@TargetDir@", "@StartMenuDir@", "@DesktopDir@"];

    for (var dir in installDir)
        component.addOperation("CreateShortcut",
                                "@TargetDir@\\bin\\webcamoid.exe",
                                installDir[dir] + "\\webcamoid.lnk");
}
EOF

    cat << EOF > "$packagedir/meta/package.xml"
<?xml version="1.0"?>
<Package>
    <DisplayName>Webcamoid</DisplayName>
    <Description>The ultimate webcam suite</Description>
    <Version>${VERSION}</Version>
    <ReleaseDate>$(date "+%Y-%m-%d")</ReleaseDate>
    <Name>com.${APPNAME}prj.${APPNAME}</Name>
    <Licenses>
        <License name="GNU General Public License v3.0" file="COPYING.txt" />
    </Licenses>
    <Script>installscript.qs</Script>
    <UpdateText>
$(readchangelog "${ROOTDIR}/ChangeLog" ${VERSION} | sed '$ d')
    </UpdateText>
    <Default>true</Default>
    <ForcedInstallation>true</ForcedInstallation>
    <Essential>false</Essential>
</Package>
EOF

    # Remove old file
    rm -vf "${ROOTDIR}/ports/deploy/windows/${APPNAME}-${VERSION}-${ARCH}.exe"

    wine "${bincreator}" \
         -c "$configdir/config.xml" \
         -p "${ROOTDIR}/build/bundle-data/installer/packages" \
         ${ROOTDIR}/ports/deploy/windows/${APPNAME}-${VERSION}-${ARCH}.exe
}

package() {
    createportable
    createinstaller
}

cleanup() {
    find "${ROOTDIR}/build/bundle-data/${APPNAME}" \
        \( -iname '*.a' -or -iname '*.prl' \) -delete
}

prepare
qtdeps
solveall
createlauncher
package
cleanup
