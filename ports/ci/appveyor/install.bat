if %PLATFORM% == "x86" (
    set FF_ARCH=win32
    set GST_ARCH=x86
) else (
    set FF_ARCH=win64
    set GST_ARCH=x86_64
)

rem Installing various utilities

choco install -y curl 7zip InnoSetup

set PATH=%PATH%;"C:\Program Files\7-Zip";"C:\Program Files (x86)\Inno Setup 5";%QTDIR%\bin

if %QMAKESPEC% == "win32-g++" set PATH=%PATH%;%MINGW%\bin

rem Installing FFmpeg dev

set FFMPEG_FILE=ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-dev.zip

if not exist %FFMPEG_FILE% curl -kLOC - https://ffmpeg.zeranoe.com/builds/%FF_ARCH%/dev/%FFMPEG_FILE%

if exist %FFMPEG_FILE% 7z x %FFMPEG_FILE%

set FFMPEG_DEV_PATH=%CD%\ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-dev

rem Installing GStreamer

set GSTREAMER_FILE=gstreamer-1.0-devel-%GST_ARCH%-%GSTREAMER_VERSION%.msi

if not exist %GSTREAMER_FILE% curl -kLOC - https://gstreamer.freedesktop.org/data/pkg/windows/%GSTREAMER_VERSION%/%GSTREAMER_FILE%

if exist %GSTREAMER_FILE% msiexec /a %CD%\%GSTREAMER_FILE% /qn TARGETDIR=%CD%\gstreamer-1.0

set GSTREAMER_DEV_PATH=%CD%\gstreamer-1.0\gstreamer\1.0\%GST_ARCH%

rem Copy necessary libraries to an alternative path to avoid conflicts with
rem Qt's MinGW system libraries

xcopy %GSTREAMER_DEV_PATH%\lib\libgobject-2.0.* %GSTREAMER_DEV_PATH%\lib2 /i /y
xcopy %GSTREAMER_DEV_PATH%\lib\libglib-2.0.* %GSTREAMER_DEV_PATH%\lib2 /i /y
xcopy %GSTREAMER_DEV_PATH%\lib\libgstreamer-1.0.* %GSTREAMER_DEV_PATH%\lib2 /i /y
xcopy %GSTREAMER_DEV_PATH%\lib\libgstapp-1.0.* %GSTREAMER_DEV_PATH%\lib2 /i /y
xcopy %GSTREAMER_DEV_PATH%\lib\libgstpbutils-1.0.* %GSTREAMER_DEV_PATH%\lib2 /i /y
xcopy %GSTREAMER_DEV_PATH%\lib\libgstaudio-1.0.* %GSTREAMER_DEV_PATH%\lib2 /i /y
xcopy %GSTREAMER_DEV_PATH%\lib\libgstvideo-1.0.* %GSTREAMER_DEV_PATH%\lib2 /i /y
