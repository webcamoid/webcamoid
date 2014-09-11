#!/bin/sh

ffmpegVersion=2.3.2
yasmVersion=1.2.0
fribidiVersion=0.19.6
expatVersion=2.1.0
fontconfigVersion=2.11.1
libassVersion=0.11.2
libblurayVersion=0.6.0
libgsmVersion=1.0.13
lameVersion=3.99.5
opencoreVersion=0.1.3
openjpegVersion=1.5.1
opusVersion=1.1
rtmpdumpVersion=2.3
orcVersion=0.4.18
schroedingerVersion=1.0.11
liboggVersion=1.3.2
speexVersion=1.2rc1
theoraVersion=1.1.1
libvorbisVersion=1.3.4
vpxVersion=1.2.0
xvidcoreVersion=1.3.3
FGET='wget -c --retry-connrefused --no-check-certificate'

function getYasm()
{
    packageName=yasm
    fileExt=exe
    packageFile=$packageName-$yasmVersion-win32.$fileExt

    if [ ! -f "$packageName.$fileExt" ]
    then
        $FGET http://www.tortall.net/projects/$packageName/releases/$packageFile
        ln -s $packageFile $packageName.$fileExt
    fi
}

function getFreeType()
{
    packageName=freetype
    fileExt=zip
    packageFile=$packageName-bin.$fileExt

    if [ ! -f "$packageName.$fileExt" ]
    then
        $FGET -O $packageFile http://gnuwin32.sourceforge.net/downlinks/$packageName-bin-$fileExt.php
        unzip -u -d win32 $packageFile
    fi
}

function compileFribidi()
{
    packageName=fribidi
    packageFolder=$packageName-$fribidiVersion
    fileExt=tar.bz2
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET http://$packageName.org/download/$packageFile
        tar -xjvf $packageFile
    fi

    cd $packageFolder

    ./configure --prefix="$curPath/win32" --disable-shared

    make
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileFontConfig()
{
    packageName=fontconfig
    packageFolder=$packageName-$fontconfigVersion
    fileExt=tar.bz2
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET http://www.freedesktop.org/software/$packageName/release/$packageFile
        tar -xjvf $packageFile
    fi

    cd $packageFolder

    export FREETYPE_CFLAGS="-I$curPath/win32/include"
    export FREETYPE_LIBS=-"-L$curPath/win32/lib"

    ./configure --prefix="$curPath/win32" --disable-shared

    make
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileLibass()
{
    packageName=libass
    packageFolder=$packageName-$libassVersion
    fileExt=tar.gz
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET -O $packageFile https://github.com/$packageName/$packageName/releases/download/$libassVersion/$packageFile
        tar -xzvf $packageFile
    fi

    cd $packageFolder

    export FREETYPE_CFLAGS="-I$curPath/win32/include"
    export FREETYPE_LIBS=-"-L$curPath/win32/lib"
    export FRIBIDI_CFLAGS="-I$curPath/win32/include"
    export FRIBIDI_LIBS=-"-L$curPath/win32/lib"
    export FONTCONFIG_CFLAGS="-I$curPath/win32/include"
    export FONTCONFIG_LIBS=-"-L$curPath/win32/lib"

    ./configure --prefix="$curPath/win32" --disable-shared

    make
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileLibbluray()
{
    packageName=libbluray
    packageFolder=$packageName-$libblurayVersion
    fileExt=tar.bz2
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET ftp://ftp.videolan.org/pub/videolan/$packageName/$libblurayVersion/$packageFile
        tar -xjvf $packageFile
    fi

    cd $packageFolder

    ./configure --prefix="$curPath/win32" --disable-shared \
        --disable-werror \
        --disable-extra-warnings \
        --disable-examples \
        --disable-doxygen-doc \
        --disable-doxygen-dot \
        --disable-doxygen-html \
        --disable-doxygen-ps \
        --disable-doxygen-pdf \
        --without-libxml2 \
        --without-freetype

    make
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileLibgsm()
{
    packageName=gsm
    packageFolder=$packageName-$libgsmVersion
    fileExt=tar.gz
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET http://www.quut.com/$packageName/$packageFile
        tar -xzvf $packageFile
    fi

    cd gsm-*

    make
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileLame()
{
    packageName=lame
    packageFolder=$packageName-$lameVersion
    fileExt=tar.gz
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET http://sourceforge.net/projects/$packageName/files/$packageName/3.99/$packageFile
        tar -xzvf $packageFile
    fi

    cd $packageFolder

    ./configure --prefix="$curPath/win32" --disable-shared

    make
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileOpencore()
{
    packageName=opencore-amr
    packageFolder=$packageName-$opencoreVersion
    fileExt=tar.gz
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET http://sourceforge.net/projects/$packageName/files/$packageName/$packageFile
        tar -xzvf $packageFile
    fi

    cd $packageFolder

    ./configure --prefix="$curPath/win32" --disable-shared

    make
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileOpenjpeg()
{
    packageName=openjpeg
    packageFolder=$packageName-$openjpegVersion
    fileExt=tar.gz
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET https://$packageName.googlecode.com/files/$packageFile
        tar -xzvf $packageFile
    fi

    cd $packageFolder

    ./configure --prefix="$curPath/win32" --disable-shared \
        --disable-doc

    make
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileOpus()
{
    packageName=opus
    packageFolder=$packageName-$opusVersion
    fileExt=tar.gz
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET http://downloads.xiph.org/releases/$packageName/$packageFile
        tar -xzvf $packageFile
    fi

    cd $packageFolder

    ./configure --prefix="$curPath/win32" --disable-shared

    make
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileRtmpdump()
{
    packageName=rtmpdump
    packageFolder=$packageName-$rtmpdumpVersion
    fileExt=tgz
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET http://$packageName.mplayerhq.hu/download/$packageFile
        tar -xzvf $packageFile
    fi

    cd $packageFolder

    make
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileOrc()
{
    packageName=orc
    packageFolder=$packageName-$orcVersion
    fileExt=tar.gz
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET http://code.entropywave.com/download/$packageName/$packageFile
        tar -xzvf $packageFile
    fi

    cd $packageFolder

    ./configure --prefix="$curPath/win32" --disable-shared

    make
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileSchroedinger()
{
    packageName=schroedinger
    packageFolder=$packageName-$schroedingerVersion
    fileExt=tar.gz
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET http://diracvideo.org/download/$packageName/$packageFile
        tar -xzvf $packageFile
    fi

    cd $packageFolder

    ./configure --prefix="$curPath/win32" --disable-shared

    make
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileOgg()
{
    packageName=libogg
    packageFolder=$packageName-$liboggVersion
    fileExt=zip
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET http://downloads.xiph.org/releases/ogg/$packageFile
        unzip -u $packageFile
    fi

    cd $packageFolder

    ./configure --prefix="$curPath/win32" --disable-shared

    make
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileSpeex()
{
    packageName=speex
    packageFolder=$packageName-$speexVersion
    fileExt=tar.gz
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET http://downloads.xiph.org/releases/$packageName/$packageFile
        tar -xzvf $packageFile
    fi

    cd $packageFolder

    ./configure --prefix="$curPath/win32" --disable-shared \
        --with-ogg-libraries="$curPath/win32/lib" \
        --with-ogg-includes="$curPath/win32/include" \
        --disable-oggtest

    make CFLAGS="-I$curPath/win32/include -L$curPath/win32/lib"
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileTheora()
{
    packageName=libtheora
    packageFolder=$packageName-$theoraVersion
    fileExt=zip
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET http://downloads.xiph.org/releases/theora/$packageFile
        unzip -u $packageFile
    fi

    cd $packageFolder

    ./configure --prefix="$curPath/win32" --disable-shared \
        --with-ogg-libraries="$curPath/win32/lib" \
        --with-ogg-includes="$curPath/win32/include" \
        --disable-encode \
        --disable-oggtest \
        --disable-vorbistest \
        --disable-sdltest \
        --disable-examples \
        --without-vorbis

    make CFLAGS="-I$curPath/win32/include -L$curPath/win32/lib"
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileLibvorbis()
{
    packageName=libvorbis
    packageFolder=$packageName-$libvorbisVersion
    fileExt=zip
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET http://downloads.xiph.org/releases/vorbis/$packageFile
        unzip -u $packageFile
    fi

    cd $packageFolder

    ./configure --prefix="$curPath/win32" --disable-shared \
        --with-ogg-libraries="$curPath/win32/lib" \
        --with-ogg-includes="$curPath/win32/include" \
        --disable-docs \
        --disable-examples \
        --disable-oggtest

    make CFLAGS="-I$curPath/win32/include -L$curPath/win32/lib"
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileVpx()
{
    packageName=libvpx
    packageFolder=$packageName-v$vpxVersion
    fileExt=zip
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET https://webm.googlecode.com/files/$packageFile
        unzip -u $packageFile
    fi

    cd $packageFolder

    ./configure --prefix="$curPath/win32" --disable-shared \
        --disable-unit-tests \
        --disable-examples \
        --disable-docs

    make
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileLibx264()
{
    packageName=x264
    packageFolder=last_$packageName
    fileExt=tar.bz2
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -f "$packageFile" ]
    then
        $FGET ftp://ftp.videolan.org/pub/$packageName/snapshots/$packageFile
        tar -xjvf $packageFile
    fi

    cd $packageName-snapshot-*

    ./configure --prefix="$curPath/win32" --enable-static \
        --disable-avs \
        --disable-swscale \
        --disable-lavf \
        --disable-ffms \
        --disable-gpac \
        --disable-lsmash

    make
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
}

function compileXvid()
{
    packageName=xvidcore
    packageFolder=$packageName-$xvidcoreVersion
    fileExt=tar.bz2
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageName" ]
    then
        $FGET http://downloads.xvid.org/downloads/$packageFile
        tar -xjvf $packageFile
    fi

    cd $packageName/build/generic

    ./configure --prefix="$curPath/win32"

    make
    make install

    if [ $? == '0' ]
    then
        touch ../../../$packageName.compiled
    fi
}

function compileFFmpeg()
{
    packageName=ffmpeg
    buildSuffix=Qb
    packageFolder=$packageName-$ffmpegVersion
    fileExt=tar.bz2
    packageFile=$packageFolder.$fileExt

    curPath="$PWD"

    if [ -f "$packageName.compiled" ]
    then
        return
    fi

    if [ ! -d "$packageFolder" ]
    then
        $FGET http://$packageName.org/releases/$packageFile
        tar -xjvf $packageFile
    fi

    cd $packageFolder

    ./configure \
        --prefix="$curPath/win32" \
        --extra-cflags="-I$curPath/win32/include" \
        --extra-ldflags="-L$curPath/win32/lib" \
        --disable-debug \
        --enable-gpl \
        --enable-version3 \
        --build-suffix=$buildSuffix \
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
        --enable-static \
        --disable-shared \
        --enable-avresample \
        --enable-dxva2 \
        --disable-fontconfig \
        --disable-libass \
        --disable-libbluray \
        --disable-libfreetype \
        --disable-libgsm \
        --disable-libmodplug \
        --enable-libmp3lame \
        --enable-libopencore_amrnb \
        --enable-libopencore_amrwb \
        --disable-libopenjpeg \
        --disable-libopus \
        --disable-librtmp \
        --disable-libschroedinger \
        --enable-libspeex \
        --enable-libtheora \
        --enable-libvorbis \
        --enable-libvpx \
        --enable-libx264 \
        --enable-libxvid \
        --enable-postproc \
        --enable-vdpau

    make
    make install

    if [ $? == '0' ]
    then
        touch ../$packageName.compiled
    fi
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
    getYasm \
    compileLame \
    compileOpencore \
    compileOpus \
    compileOrc \
    compileOgg \
    compileSpeex \
    compileTheora \
    compileLibvorbis \
    compileVpx \
    compileLibx264 \
    compileXvid \
    compileFFmpeg
#    getFreeType \
#    compileFribidi \
#    compileFontConfig \
#    compileLibass \
#    compileLibbluray \
#    compileLibgsm \
#    compileOpenjpeg \
#    compileRtmpdump \
#    compileSchroedinger \
