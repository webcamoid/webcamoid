set PLATFORM=x86
set FFMPEG_VERSION=3.2.2
set QMAKESPEC=win32-g++
set MAKETOOL=mingw32-make
set QTDIR=C:\Qt\5.8\mingw53_32
set TOOLSDIR=C:\Qt\Tools\mingw530_32
set CONFIGURATION=release

if "%PLATFORM%" == "x86" (
    set FF_ARCH=win32
    set GST_ARCH=x86
    set VC_ARGS=x86
) else (
    set FF_ARCH=win64
    set GST_ARCH=x86_64
    set VC_ARGS=amd64
)

rem Installing various utilities

if not "%VSVER%" == "" call "C:\Program Files (x86)\Microsoft Visual Studio %VSVER%.0\VC\vcvarsall" %VC_ARGS%

set PATH=%PATH%;"C:\Program Files\7-Zip";%QTDIR%\bin;%TOOLSDIR%\bin

rem Installing FFmpeg dev

set FFMPEG_FILE=ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-dev.zip

if not exist %FFMPEG_FILE% wget -c https://ffmpeg.zeranoe.com/builds/%FF_ARCH%/dev/%FFMPEG_FILE%

if exist %FFMPEG_FILE% 7z x %FFMPEG_FILE%

set FFMPEG_DEV_PATH=%CD%\ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-dev

rem Start build
set SOURCES_DIR=%CD%\..\..

qmake %SOURCES_DIR%\Webcamoid.pro ^
    CONFIG+=%CONFIGURATION% ^
    FFMPEGINCLUDES="%FFMPEG_DEV_PATH%\include" ^
    FFMPEGLIBS=-L"%FFMPEG_DEV_PATH%\lib" ^
    FFMPEGLIBS+=-lavcodec ^
    FFMPEGLIBS+=-lavdevice ^
    FFMPEGLIBS+=-lavformat ^
    FFMPEGLIBS+=-lavutil ^
    FFMPEGLIBS+=-lswresample ^
    FFMPEGLIBS+=-lswscale

%MAKETOOL% -j4
%MAKETOOL% install

pause
