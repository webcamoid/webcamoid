#!/bin/bash

p7zVersion=920
ffmpegVersion=2.2.3
opencvVersion=2.4.9
FGET='wget -c --retry-connrefused --no-check-certificate'

function get7Z()
{
    packageName=7za
    fileExt=zip
    packageFile=${packageName}${p7zVersion}.${fileExt}

    if [ ! -f "${packageName}.exe" ]
    then
        ${FGET} http://downloads.sourceforge.net/sevenzip/${packageFile}
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

function buildOpenCV()
{
    packageName=opencv
    packageFolder=${packageName}-${opencvVersion}
    fileExt=zip
    packageFile=${packageFolder}.${fileExt}

    curPath="${PWD}"

    if [ -f "${packageName}.compiled" ]
    then
        return
    fi

    if [ ! -d "${packageFolder}" ]
    then
        ${FGET} http://downloads.sourceforge.net/${packageName}library/${packageFile}
        unzip -u ${packageFile}
    fi

    mkdir -p "${packageFolder}/build"
    cd "${packageFolder}/build"

    cmake \
        -G "MSYS Makefiles" \
        -D CMAKE_INSTALL_PREFIX="${curPath}/win32" \
        -D CMAKE_C_COMPILER="C:/MinGW/bin/gcc.exe" \
        -D CMAKE_CXX_COMPILER="C:/MinGW/bin/g++.exe" \
        -D CMAKE_BUILD_TYPE=Release \
        -D BUILD_PERF_TESTS=OFF \
        -D BUILD_TESTS=OFF \
        ..

    mingw32-make
    mingw32-make install
    cp -Rvf ${curPath}/win32/x86/mingw/bin ${curPath}/win32
    cp -Rvf ${curPath}/win32/x86/mingw/lib ${curPath}/win32

    if [ $? == '0' ]
    then
        touch ../../${packageName}.compiled
    fi
}

function buildWebcamoid()
{
    cd ..

    /qttools/qmake Webcamoid.pro \
        FFMPEGINCLUDES="${PWD}/build/win32/include" \
        FFMPEGLIBS="-L${PWD}/build/win32/lib" \
        OPENCVINCLUDES="${PWD}/build/win32/include" \
        OPENCVLIBS="-L${PWD}/build/win32/lib" \
        OPENCVLIBS+=-lopencv_calib3d249 \
        OPENCVLIBS+=-lopencv_contrib249 \
        OPENCVLIBS+=-lopencv_core249 \
        OPENCVLIBS+=-lopencv_features2d249 \
        OPENCVLIBS+=-lopencv_flann249 \
        OPENCVLIBS+=-lopencv_gpu249 \
        OPENCVLIBS+=-lopencv_highgui249 \
        OPENCVLIBS+=-lopencv_imgproc249 \
        OPENCVLIBS+=-lopencv_legacy249 \
        OPENCVLIBS+=-lopencv_ml249 \
        OPENCVLIBS+=-lopencv_objdetect249 \
        OPENCVLIBS+=-lopencv_photo249 \
        OPENCVLIBS+=-lopencv_stitching249 \
        OPENCVLIBS+=-lopencv_superres249 \
        OPENCVLIBS+=-lopencv_ts249 \
        OPENCVLIBS+=-lopencv_video249 \
        OPENCVLIBS+=-lopencv_videostab249

    mingw32-make
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
    buildOpenCV \
    buildWebcamoid
