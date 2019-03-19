REM Webcamoid, webcam capture application.
REM Copyright (C) 2016  Gonzalo Exequiel Pedone
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

if "%PLATFORM%" == "x86" (
    set FF_ARCH=win32
    set GST_ARCH=x86
    set VC_ARGS=x86
) else (
    set FF_ARCH=win64
    set GST_ARCH=x86_64
    set VC_ARGS=amd64
)

rem Visual Studio init
if not "%VSPATH%" == "" call "%VSPATH%\vcvarsall" %VC_ARGS%

set FFMPEG_DEV_PATH=%CD%\ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-dev
set PATH_ORIG=%PATH%

if not "%DAILY_BUILD%" == "" goto DailyBuild

set GSTREAMER_DEV_PATH=C:\gstreamer\1.0\%GST_ARCH%
set PATH=%QTDIR%\bin;%TOOLSDIR%\bin;%CD%\ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-shared\bin;%GSTREAMER_DEV_PATH%\bin;%PATH%

qmake -query
qmake Webcamoid.pro ^
    CONFIG+=%CONFIGURATION% ^
    CONFIG+=silent ^
    PREFIX="%INSTALL_PREFIX%" ^
    FFMPEGINCLUDES="%FFMPEG_DEV_PATH%\include" ^
    FFMPEGLIBS=-L"%FFMPEG_DEV_PATH%\lib" ^
    FFMPEGLIBS+=-lavcodec ^
    FFMPEGLIBS+=-lavdevice ^
    FFMPEGLIBS+=-lavformat ^
    FFMPEGLIBS+=-lavutil ^
    FFMPEGLIBS+=-lswresample ^
    FFMPEGLIBS+=-lswscale ^
    GSTREAMERINCLUDES="%GSTREAMER_DEV_PATH%\include" ^
    GSTREAMERINCLUDES+="%GSTREAMER_DEV_PATH%\include\glib-2.0" ^
    GSTREAMERINCLUDES+="%GSTREAMER_DEV_PATH%\lib\glib-2.0\include" ^
    GSTREAMERINCLUDES+="%GSTREAMER_DEV_PATH%\include\gstreamer-1.0" ^
    GSTREAMERLIBS=-L"%GSTREAMER_DEV_PATH%\lib2" ^
    GSTREAMERLIBS+=-lgobject-2.0 ^
    GSTREAMERLIBS+=-lglib-2.0 ^
    GSTREAMERLIBS+=-lgstreamer-1.0 ^
    GSTREAMERLIBS+=-lgstapp-1.0 ^
    GSTREAMERLIBS+=-lgstpbutils-1.0 ^
    GSTREAMERLIBS+=-lgstaudio-1.0 ^
    GSTREAMERLIBS+=-lgstvideo-1.0

goto Make

:DailyBuild

set PATH=%QTDIR%\bin;%TOOLSDIR%\bin;%CD%\ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-shared\bin;%PATH%

qmake -query
qmake Webcamoid.pro ^
    CONFIG+=%CONFIGURATION% ^
    CONFIG+=silent ^
    PREFIX="%INSTALL_PREFIX%" ^
    FFMPEGINCLUDES="%FFMPEG_DEV_PATH%\include" ^
    FFMPEGLIBS=-L"%FFMPEG_DEV_PATH%\lib" ^
    FFMPEGLIBS+=-lavcodec ^
    FFMPEGLIBS+=-lavdevice ^
    FFMPEGLIBS+=-lavformat ^
    FFMPEGLIBS+=-lavutil ^
    FFMPEGLIBS+=-lswresample ^
    FFMPEGLIBS+=-lswscale

:Make

%MAKETOOL% -j4

if "%DAILY_BUILD%" == "" goto EndScript

if "%PLATFORM%" == "x86" (
    set DRV_ARCH=x64
) else (
    set DRV_ARCH=x86
)

echo.
echo Building %DRV_ARCH% virtual camera driver
echo.

mkdir akvcam
cd akvcam

set PATH=%QTDIR_ALT%\bin;%TOOLSDIR_ALT%\bin;%PATH_ORIG%
qmake -query
qmake ^
    ..\libAvKys\Plugins\VirtualCamera\VirtualCamera.pro ^
    VIRTUALCAMERAONLY=1
%MAKETOOL% -j4

cd ..
mkdir libAvKys\Plugins\VirtualCamera\src\dshow\VirtualCamera\AkVirtualCamera.plugin\%DRV_ARCH%
xcopy ^
    akvcam\src\dshow\VirtualCamera\AkVirtualCamera.plugin\%DRV_ARCH%\* ^
    libAvKys\Plugins\VirtualCamera\src\dshow\VirtualCamera\AkVirtualCamera.plugin\%DRV_ARCH% ^
    /i /y

:EndScript

set PATH=%PATH_ORIG%
