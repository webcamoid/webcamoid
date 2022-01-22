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

git clone "https://github.com/webcamoid/DeployTools.git"

set QTDIR=C:\Qt\%QTVER%\msvc2019_64
set TOOLSDIR=C:\Qt\Tools\QtCreator
set FFMPEG_PATH=%CD%\ffmpeg-%FFMPEG_VERSION%-full_build-shared
set GSTREAMER_DEV_PATH=C:\gstreamer\1.0\x86_64
set PATH=%QTDIR%\bin;%TOOLSDIR%\bin;%FFMPEG_PATH%\bin;%GSTREAMER_DEV_PATH%\bin;%PATH%
set INSTALL_PREFIX=%CD%/webcamoid-data
set PACKAGES_DIR=%CD%/webcamoid-packages/windows
set BUILD_PATH=%CD%/build
set PYTHONPATH=%CD%/DeployTools

python DeployTools/deploy.py ^
    -d "%INSTALL_PREFIX%" ^
    -c "%BUILD_PATH%/package_info.conf" ^
    -o "%PACKAGES_DIR%"
