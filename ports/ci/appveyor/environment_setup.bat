REM Webcamoid, camera capture application.
REM Copyright (C) 2019  Gonzalo Exequiel Pedone
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

@echo off
set ENVIRONMENT_FILE=environment.sh
del %ENVIRONMENT_FILE%
echo export COMPILER=%COMPILER% >> %ENVIRONMENT_FILE%
echo export NJOBS=%NJOBS% >> %ENVIRONMENT_FILE%
echo export PLATFORM=%PLATFORM% >> %ENVIRONMENT_FILE%
