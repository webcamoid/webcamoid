REM Webcamoid, webcam capture application.
REM Copyright (C) 2017  Gonzalo Exequiel Pedone
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

rem Installing various utilities
choco install -y curl 7zip InnoSetup

rem Visual Studio init
if not "%VSPATH%" == "" call "%VSPATH%\vcvarsall" %VC_ARGS%

set PATH=%PATH%;"C:\Program Files\7-Zip";"C:\Program Files (x86)\Inno Setup 5";%QTDIR%\bin;%TOOLSDIR%\bin

rem Install FFmpeg development headers and libraries
set FFMPEG_DEV_FILE=ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-dev.zip

if not exist %FFMPEG_DEV_FILE% curl --retry 10 -kLOC - https://ffmpeg.zeranoe.com/builds/%FF_ARCH%/dev/%FFMPEG_DEV_FILE%

if exist %FFMPEG_DEV_FILE% 7z x %FFMPEG_DEV_FILE% -aoa -bb

set FFMPEG_DEV_PATH=%CD%\ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-dev

rem Install FFmpeg binaries
set FFMPEG_BIN_FILE=ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-shared.zip

if not exist %FFMPEG_BIN_FILE% curl --retry 10 -kLOC - https://ffmpeg.zeranoe.com/builds/%FF_ARCH%/shared/%FFMPEG_BIN_FILE%

if exist %FFMPEG_BIN_FILE% 7z x %FFMPEG_BIN_FILE% -aoa -bb

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
