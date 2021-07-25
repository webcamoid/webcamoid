REM Webcamoid, webcam capture application.
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
echo export APPVEYOR_ACCOUNT_NAME=%APPVEYOR_ACCOUNT_NAME% >> %ENVIRONMENT_FILE%
echo export APPVEYOR_JOB_ID=%APPVEYOR_JOB_ID% >> %ENVIRONMENT_FILE%
echo export APPVEYOR_PROJECT_NAME=%APPVEYOR_PROJECT_NAME% >> %ENVIRONMENT_FILE%
echo export APPVEYOR_PROJECT_SLUG=%APPVEYOR_PROJECT_SLUG% >> %ENVIRONMENT_FILE%
echo export APPVEYOR_REPO_BRANCH=%APPVEYOR_REPO_BRANCH% >> %ENVIRONMENT_FILE%
echo export COMPILER=%COMPILER% >> %ENVIRONMENT_FILE%
echo export NJOBS=%BT_KEY% >> %NJOBS%
echo export PLATFORM=%PLATFORM% >> %ENVIRONMENT_FILE%
