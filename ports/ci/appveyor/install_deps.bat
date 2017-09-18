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

choco install -y curl 7zip InnoSetup

if not "%VSVER%" == "" call "C:\Program Files (x86)\Microsoft Visual Studio %VSVER%.0\VC\vcvarsall" %VC_ARGS%

set PATH=%PATH%;"C:\Program Files\7-Zip";"C:\Program Files (x86)\Inno Setup 5";%QTDIR%\bin;%TOOLSDIR%\bin

rem Install FFmpeg development headers and libraries
set FFMPEG_DEV_FILE=ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-dev.zip

if not exist %FFMPEG_DEV_FILE% curl --retry 10 -kLOC - https://ffmpeg.zeranoe.com/builds/%FF_ARCH%/dev/%FFMPEG_DEV_FILE%

if exist %FFMPEG_DEV_FILE% 7z x %FFMPEG_DEV_FILE% -aoa

set FFMPEG_DEV_PATH=%CD%\ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-dev

rem Install FFmpeg binaries
set FFMPEG_BIN_FILE=ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-shared.zip

if not exist %FFMPEG_BIN_FILE% curl --retry 10 -kLOC - https://ffmpeg.zeranoe.com/builds/%FF_ARCH%/shared/%FFMPEG_BIN_FILE%

if exist %FFMPEG_BIN_FILE% 7z x %FFMPEG_BIN_FILE% -aoa

set PATH=%CD%\ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-shared\bin;%PATH%

rem Installing GStreamer development headers and libraries

set GSTREAMER_DEV_FILE=gstreamer-1.0-devel-%GST_ARCH%-%GSTREAMER_VERSION%.msi

if not exist %GSTREAMER_DEV_FILE% curl --retry 10 -kLOC - https://gstreamer.freedesktop.org/data/pkg/windows/%GSTREAMER_VERSION%/%GSTREAMER_DEV_FILE%

if exist %GSTREAMER_DEV_FILE% (
    start /b /wait msiexec /i %CD%\%GSTREAMER_DEV_FILE% /quiet /qn /norestart
    set GSTREAMER_DEV_PATH=C:\gstreamer\1.0\%GST_ARCH%
)

rem Copy necessary libraries to an alternative path to avoid conflicts with
rem Qt's MinGW system libraries
if exist %GSTREAMER_DEV_FILE% (
    xcopy %GSTREAMER_DEV_PATH%\lib\*gobject-2.0.* %GSTREAMER_DEV_PATH%\lib2 /i /y
    xcopy %GSTREAMER_DEV_PATH%\lib\*glib-2.0.* %GSTREAMER_DEV_PATH%\lib2 /i /y
    xcopy %GSTREAMER_DEV_PATH%\lib\*gstreamer-1.0.* %GSTREAMER_DEV_PATH%\lib2 /i /y
    xcopy %GSTREAMER_DEV_PATH%\lib\*gstapp-1.0.* %GSTREAMER_DEV_PATH%\lib2 /i /y
    xcopy %GSTREAMER_DEV_PATH%\lib\*gstpbutils-1.0.* %GSTREAMER_DEV_PATH%\lib2 /i /y
    xcopy %GSTREAMER_DEV_PATH%\lib\*gstaudio-1.0.* %GSTREAMER_DEV_PATH%\lib2 /i /y
    xcopy %GSTREAMER_DEV_PATH%\lib\*gstvideo-1.0.* %GSTREAMER_DEV_PATH%\lib2 /i /y
)

rem Installing GStreamer binaries

set GSTREAMER_BIN_FILE=gstreamer-1.0-%GST_ARCH%-%GSTREAMER_VERSION%.msi

if not exist %GSTREAMER_BIN_FILE% curl --retry 10 -kLOC - https://gstreamer.freedesktop.org/data/pkg/windows/%GSTREAMER_VERSION%/%GSTREAMER_BIN_FILE%

if exist %GSTREAMER_BIN_FILE% (
    start /b /wait msiexec /i %CD%\%GSTREAMER_BIN_FILE% /quiet /qn /norestart
    set GSTREAMER_BIN_PATH=C:\gstreamer\1.0\%GST_ARCH%
)

set PATH=%PATH%;%GSTREAMER_BIN_PATH%\bin
