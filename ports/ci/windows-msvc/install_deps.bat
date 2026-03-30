REM Webcamoid, camera capture application.
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

:Exit
