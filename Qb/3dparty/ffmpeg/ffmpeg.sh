#!/bin/sh
#
# Webcamoid, webcam capture application.
# Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
#
# Webcamod is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Webcamod is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
#
# Email     : hipersayan DOT x AT gmail DOT com
# Web-Site 1: http://github.com/hipersayanX/Webcamoid
# Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796

packageName=$1
packageVersion=$2
packageInstallPrefix=$3
extraOptions=$4
buildSuffix=$5
packagePrivateFolder=${packageName}_priv
packageFolder="$packageName-$packageVersion"
fileExt='tar.bz2'
packageFile="$packageFolder.$fileExt"
initialPath=$PWD
priFile=../${packageName}_auto.pri

if [ -f "$priFile" ];then
    exit 0
fi

mkdir -p $packagePrivateFolder
cd $packagePrivateFolder

if [ ! -d "$packageFolder" ];then
    if [ ! -f "$packageFile" ];then
        wget --retry-connrefused -c http://$packageName.org/releases/$packageFile

        if [ $? != '0' ];then
            exit 1
        fi
    fi

    tar -xjvf "$packageFile"

    if [ $? != '0' ];then
        exit 2
    fi
fi

cd "$packageFolder"

./configure --prefix="${packageInstallPrefix}" \
            --disable-debug \
            --enable-gpl \
            --enable-version3 \
            --build-suffix=${buildSuffix} \
            --enable-runtime-cpudetect \
            --disable-programs \
            --disable-ffmpeg \
            --disable-ffplay \
            --disable-ffprobe \
            --disable-ffserver \
            --disable-doc \
            --disable-htmlpages \
            --disable-manpages \
            --disable-podpages \
            --disable-txtpages \
            --enable-shared \
            --disable-static \
            --enable-avresample \
            ${extraOptions}

if [ $? != '0' ];then
    exit 3
fi

make -j $(grep -c "^processor" /proc/cpuinfo)

if [ $? != '0' ];then
    exit 4
fi

make install

if [ $? != '0' ];then
    exit 5
fi

cd ${initialPath}

if [ ! -f "$priFile" ];then
    packageVarName=$(echo ${packageName} | tr '[:lower:]' '[:upper:]')

    echo ${packageVarName}HEADERSPATH=\"${packageInstallPrefix}/include\" >> $priFile
    echo ${packageVarName}LIBSPATH=\"${packageInstallPrefix}/lib\" >> $priFile
    echo ${packageVarName}BUILDSUFFIX=${buildSuffix} >> $priFile
fi
