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

rem Install Qt
pip install -U pip
pip install aqtinstall
aqt install-qt windows desktop "%QTVER%" win64_msvc2019_64 -O "C:\Qt"
aqt install-tool windows desktop tools_qtcreator -O "C:\Qt"
set QTDIR=C:\Qt\%QTVER%\msvc2019_64
set TOOLSDIR=C:\Qt\Tools\QtCreator
set PATH=%QTDIR%\bin;%TOOLSDIR%\bin;%PATH%

rem Install FFmpeg development headers and libraries
set FFMPEG_FILE=ffmpeg-%FFMPEG_VERSION%-full_build-shared.7z

if not exist %FFMPEG_FILE% curl --retry 10 -kLOC - "https://www.gyan.dev/ffmpeg/builds/packages/%FFMPEG_FILE%"

if exist %FFMPEG_FILE% 7z x %FFMPEG_FILE% -aoa -bb

rem Installing GStreamer development headers and libraries
if not "%DAILY_BUILD%" == "" goto Exit

set GSTREAMER_DEV_FILE=gstreamer-1.0-devel-x86_64-%GSTREAMER_VERSION%.msi

if not exist %GSTREAMER_DEV_FILE% curl --retry 10 -kLOC - "https://gstreamer.freedesktop.org/data/pkg/windows/%GSTREAMER_VERSION%/%GSTREAMER_DEV_FILE%"

if exist %GSTREAMER_DEV_FILE% (
    start /b /wait msiexec /i "%CD%\%GSTREAMER_DEV_FILE%" /quiet /qn /norestart
    set GSTREAMER_DEV_PATH=C:\gstreamer\1.0\x86_64
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

set GSTREAMER_BIN_FILE=gstreamer-1.0-x86_64-%GSTREAMER_VERSION%.msi

if not exist %GSTREAMER_BIN_FILE% curl --retry 10 -kLOC - "https://gstreamer.freedesktop.org/data/pkg/windows/%GSTREAMER_VERSION%/%GSTREAMER_BIN_FILE%"

if exist %GSTREAMER_BIN_FILE% (
    start /b /wait msiexec /i "%CD%\%GSTREAMER_BIN_FILE%" /quiet /qn /norestart
)

:Exit
