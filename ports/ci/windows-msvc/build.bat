REM Webcamoid, webcam capture application.
REM Copyright (C) 2022  Gonzalo Exequiel Pedone
REM
REM Webcamoid is free software: you can redistribute it and/or modify
REM it under the terms of the GNU General Public License as published by
REM the Free Software Foundation, either version 3 of the License, or
REM (at your option) any later version.
REM
REM Webcamoid is distributed in the hope that it will be useful,
REM but WITHOUT ANY WARRANTY; without even the implied warranty of
REM MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
REM GNU General Public License for more details.
REM
REM You should have received a copy of the GNU General Public License
REM along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
REM
REM Web-Site: http://webcamoid.github.io/

if "%TARGET_ARCH%" == "x86" (
    set FF_ARCH=win32
    set GST_ARCH=x86
    set VC_ARGS=x86
    set VS_ARCH=Win32
    set QTDIR=C:\Qt\%QTVER%\msvc2019
) else (
    set FF_ARCH=win64
    set GST_ARCH=x86_64
    set VC_ARGS=amd64
    set VS_ARCH=x64
    set QTDIR=C:\Qt\%QTVER%\msvc2019_64
)

set TOOLSDIR=C:\Qt\Tools\QtCreator

rem Visual Studio init
set VSPATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build
call "%VSPATH%\vcvarsall" "%VC_ARGS%"

set FFMPEG_DEV_PATH=%CD%\ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-dev
set PATH_ORIG=%PATH%

set INSTALL_PREFIX=%CD%\webcamoid-data-%TARGET_ARCH%
set buildDir=build-%TARGET_ARCH%
mkdir "%buildDir%"

if not "%DAILY_BUILD%" == "" goto DailyBuild

set GSTREAMER_DEV_PATH=C:\gstreamer\1.0\%GST_ARCH%
set PATH=%QTDIR%\bin;%TOOLSDIR%\bin;%CD%\ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-shared\bin;%GSTREAMER_DEV_PATH%\bin;%PATH%

rem Add FFmpeg includes and libraries paths
set CXXFLAGS="-I%FFMPEG_DEV_PATH%\include"
set LDFLAGS="-L%FFMPEG_DEV_PATH%\lib"
set LDFLAGS="%LDFLAGS% -lavcodec"
set LDFLAGS="%LDFLAGS% -lavdevice"
set LDFLAGS="%LDFLAGS% -lavformat"
set LDFLAGS="%LDFLAGS% -lavutil"
set LDFLAGS="%LDFLAGS% -lswresample"
set LDFLAGS="%LDFLAGS% -lswscale"

rem Add GStreamer includes and libraries paths
set CXXFLAGS="-I%GSTREAMER_DEV_PATH%\include"
set CXXFLAGS="%CXXFLAGS% -I%GSTREAMER_DEV_PATH%\include\glib-2.0"
set CXXFLAGS="%CXXFLAGS% -I%GSTREAMER_DEV_PATH%\include\gstreamer-1.0"
set CXXFLAGS="%CXXFLAGS% -I%GSTREAMER_DEV_PATH%\lib\glib-2.0\include"
set LDFLAGS="-L%GSTREAMER_DEV_PATH%\lib"
set LDFLAGS="%LDFLAGS% -lgobject-2.0"
set LDFLAGS="%LDFLAGS% -lglib-2.0"
set LDFLAGS="%LDFLAGS% -lgstreamer-1.0"
set LDFLAGS="%LDFLAGS% -lgstapp-1.0"
set LDFLAGS="%LDFLAGS% -lgstpbutils-1.0"
set LDFLAGS="%LDFLAGS% -lgstaudio-1.0"
set LDFLAGS="%LDFLAGS% -lgstvideo-1.0"

cmake ^
    -LA ^
    -S . ^
    -B "%buildDir%" ^
    -G "Visual Studio 16 2019" ^
    -A "%VS_ARCH%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%" ^
    -DCMAKE_C_COMPILER="%COMPILER_C%" ^
    -DCMAKE_CXX_COMPILER="%COMPILER_CXX%"

goto Make

:DailyBuild

set PATH=%QTDIR%\bin;%TOOLSDIR%\bin;%CD%\ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-shared\bin;%PATH%

rem Add FFmpeg includes and libraries paths
set CXXFLAGS="-I%FFMPEG_DEV_PATH%\include"
set LDFLAGS="-L%FFMPEG_DEV_PATH%\lib"
set LDFLAGS="%LDFLAGS% -lavcodec"
set LDFLAGS="%LDFLAGS% -lavdevice"
set LDFLAGS="%LDFLAGS% -lavformat"
set LDFLAGS="%LDFLAGS% -lavutil"
set LDFLAGS="%LDFLAGS% -lswresample"
set LDFLAGS="%LDFLAGS% -lswscale"

cmake ^
    -LA ^
    -S . ^
    -B "%buildDir%" ^
    -G "Visual Studio 16 2019" ^
    -A "%VS_ARCH%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%" ^
    -DCMAKE_C_COMPILER="%COMPILER_C%" ^
    -DCMAKE_CXX_COMPILER="%COMPILER_CXX%" ^
    -DDAILY_BUILD=1

:Make

cmake --build "%buildDir%" --parallel "%NJOBS%"
cmake --install "%buildDir%"

set PATH=%PATH_ORIG%
