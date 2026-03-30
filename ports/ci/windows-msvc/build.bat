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

if not "%GITHUB_SHA%" == "" set GIT_COMMIT_HASH="%GITHUB_SHA%"
if not "%CIRRUS_CHANGE_IN_REPO%" == "" set GIT_COMMIT_HASH="%CIRRUS_CHANGE_IN_REPO%"

set QTDIR=C:\Qt\%QTVER%\msvc2019_64
set TOOLSDIR=C:\Qt\Tools\QtCreator

rem Visual Studio init
set VSPATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build
call "%VSPATH%\vcvarsall" amd64

set FFMPEG_PATH=%CD%\ffmpeg-%FFMPEG_VERSION%-full_build-shared
set PATH_ORIG=%PATH%

set INSTALL_PREFIX=%CD%\webcamoid-data
set buildDir=build
mkdir "%buildDir%"

if not "%DAILY_BUILD%" == "" goto DailyBuild

set PATH=%QTDIR%\bin;%TOOLSDIR%\bin;%FFMPEG_PATH%\bin;%PATH%

rem Add FFmpeg includes and libraries paths
set CXXFLAGS=-I%FFMPEG_PATH%\include
set LDFLAGS=-L%FFMPEG_PATH%\lib
set LDFLAGS=%LDFLAGS% -lavcodec
set LDFLAGS=%LDFLAGS% -lavdevice
set LDFLAGS=%LDFLAGS% -lavformat
set LDFLAGS=%LDFLAGS% -lavutil
set LDFLAGS=%LDFLAGS% -lswresample
set LDFLAGS=%LDFLAGS% -lswscale

cmake ^
    -LA ^
    -S . ^
    -B "%buildDir%" ^
    -G "%CMAKE_GENERATOR%" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%" ^
    -DGIT_COMMIT_HASH="%GIT_COMMIT_HASH%"

goto Make

:DailyBuild

set PATH=%QTDIR%\bin;%TOOLSDIR%\bin;%FFMPEG_PATH%\bin;%PATH%

rem Add FFmpeg includes and libraries paths
set CXXFLAGS=-I%FFMPEG_PATH%\include
set LDFLAGS=-L%FFMPEG_PATH%\lib
set LDFLAGS=%LDFLAGS% -lavcodec
set LDFLAGS=%LDFLAGS% -lavdevice
set LDFLAGS=%LDFLAGS% -lavformat
set LDFLAGS=%LDFLAGS% -lavutil
set LDFLAGS=%LDFLAGS% -lswresample
set LDFLAGS=%LDFLAGS% -lswscale

cmake ^
    -LA ^
    -S . ^
    -B "%buildDir%" ^
    -G "%CMAKE_GENERATOR%" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%" ^
    -DGIT_COMMIT_HASH="%GIT_COMMIT_HASH%" ^
    -DDAILY_BUILD=1

:Make

cmake --build "%buildDir%" --config Release
cmake --build "%buildDir%" --config Release --target install

set PATH=%PATH_ORIG%
