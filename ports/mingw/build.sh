#!/bin/bash

p7zVersion=920
ffmpegVersion=3.0.1
FGET='wget -c --retry-connrefused --no-check-certificate'

function get7Z()
{
    packageName=7za
    fileExt=zip
    packageFile=${packageName}${p7zVersion}.${fileExt}

    if [ ! -f "${packageName}.exe" ]
    then
        ${FGET} http://www.7-zip.org/a/${packageFile}
        unzip -u ${packageFile}
    fi
}

function getFFmpeg()
{
    packageName=ffmpeg
    packageFolder=${packageName}-${ffmpegVersion}-win32-dev
    fileExt=7z
    packageFile=${packageFolder}.${fileExt}

    curPath="${PWD}"

    if [ ! -d "${packageFolder}" ]
    then
        ${FGET} http://${packageName}.zeranoe.com/builds/win32/dev/${packageFile}
        7za x -y ${packageFile}
        cd ${packageFolder}
        cp -Rvf * ../win32
    fi
}

function buildWebcamoid()
{
    cd ..

    /qttools/qmake Webcamoid.pro \
        PREFIX="${PWD}/build/webcamoid-7.2.0-win32" \
        FFMPEGINCLUDES="${PWD}/build/win32/include" \
        FFMPEGLIBS=-L"${PWD}/build/win32/lib" \
        FFMPEGLIBS+=-lavcodec \
        FFMPEGLIBS+=-lavdevice \
        FFMPEGLIBS+=-lavformat \
        FFMPEGLIBS+=-lavutil \
        FFMPEGLIBS+=-lswresample \
        FFMPEGLIBS+=-lswscale

    mingw32-make
    mingw32-make install
}

function build()
{
    mkdir -p ../../build
    cd ../../build
    mainPath="${PWD}"
    export PATH="${mainPath}:$PATH"
    mkdir -p win32

    for cmd in "$@"
    do
        cd "${mainPath}"
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
    buildWebcamoid
