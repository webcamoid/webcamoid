#!/bin/sh

p7zVersion=920
ffmpegVersion=2.2.3
FGET='wget -c --retry-connrefused --no-check-certificate'

function get7Z()
{
    packageName=7za
    fileExt=zip
    packageFile=$packageName$p7zVersion.$fileExt

    if [ ! -f "$packageName.exe" ]
    then
        $FGET http://downloads.sourceforge.net/sevenzip/$packageFile
        unzip -u $packageFile
    fi
}

function getFFmpeg()
{
    packageName=ffmpeg
    packageFolder=$packageName-$ffmpegVersion-win32-dev
    fileExt=7z
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ ! -d "$packageFolder" ]
    then
        $FGET http://$packageName.zeranoe.com/builds/win32/dev/$packageFile
        7za x -y $packageFile
        cd $packageFolder
        cp -Rvf * ../win32
    fi
}

function compileWebcamoid()
{
    cd ..

    /qttools/qmake Webcamoid.pro \
        FFMPEGINCLUDES="$PWD/build/win32/include" \
        FFMPEGLIBS="-L$PWD/build/win32/lib"

    mingw32-make
}

function build()
{
    mkdir -p ../../build
    cd ../../build
    mainPath="$PWD"
    export PATH="$mainPath:$PATH"
    mkdir -p win32

    for cmd in "$@"
    do
        cd "$mainPath"
        $cmd

        if [ $? != '0' ]
        then
            break
        fi
    done
}

build \
    get7Z \
    getFFmpeg \
    compileWebcamoid
