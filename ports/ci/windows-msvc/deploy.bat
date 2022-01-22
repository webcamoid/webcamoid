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
    set QTDIR=C:\Qt\%QTVER%\msvc2019
    set PYTHON_PATH=C:\%PYTHON_VERSION%
) else (
    set FF_ARCH=win64
    set GST_ARCH=x86_64
    set QTDIR=C:\Qt\%QTVER%\msvc2019_64
    set PYTHON_PATH=C:\%PYTHON_VERSION%-x64
)

git clone "https://github.com/webcamoid/DeployTools.git"

set TOOLSDIR=C:\Qt\Tools\QtCreator
set GSTREAMER_DEV_PATH=C:\gstreamer\1.0\%GST_ARCH%
set PATH=%QTDIR%\bin;%TOOLSDIR%\bin;%CD%\ffmpeg-%FFMPEG_VERSION%-%FF_ARCH%-shared\bin;%GSTREAMER_DEV_PATH%\bin;%PATH%
set INSTALL_PREFIX=%CD%/webcamoid-data-%TARGET_ARCH%
set PACKAGES_DIR=%CD%/webcamoid-packages/windows-%TARGET_ARCH%
set BUILD_PATH=%CD%/build-%TARGET_ARCH%
set PYTHONPATH=%CD%/DeployTools

"%PYTHON_PATH%\python3.exe" DeployTools/deploy.py ^
    -d "%INSTALL_PREFIX%" ^
    -c "%BUILD_PATH%/package_info.conf" ^
    -o "%PACKAGES_DIR%"
