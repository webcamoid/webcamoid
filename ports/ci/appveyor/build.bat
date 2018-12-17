REM Webcamoid, webcam capture application.
REM Copyright (C) 2016  Gonzalo Exequiel Pedone
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

qmake Webcamoid.pro ^
    CONFIG+=%CONFIGURATION% ^
    CONFIG+=silent ^
    PREFIX="%INSTALL_PREFIX%" ^
    FFMPEGINCLUDES="%FFMPEG_DEV_PATH%\include" ^
    FFMPEGLIBS=-L"%FFMPEG_DEV_PATH%\lib" ^
    FFMPEGLIBS+=-lavcodec ^
    FFMPEGLIBS+=-lavdevice ^
    FFMPEGLIBS+=-lavformat ^
    FFMPEGLIBS+=-lavutil ^
    FFMPEGLIBS+=-lswresample ^
    FFMPEGLIBS+=-lswscale ^
    GSTREAMERINCLUDES="%GSTREAMER_DEV_PATH%\include" ^
    GSTREAMERINCLUDES+="%GSTREAMER_DEV_PATH%\include\glib-2.0" ^
    GSTREAMERINCLUDES+="%GSTREAMER_DEV_PATH%\lib\glib-2.0\include" ^
    GSTREAMERINCLUDES+="%GSTREAMER_DEV_PATH%\include\gstreamer-1.0" ^
    GSTREAMERLIBS=-L"%GSTREAMER_DEV_PATH%\lib2" ^
    GSTREAMERLIBS+=-lgobject-2.0 ^
    GSTREAMERLIBS+=-lglib-2.0 ^
    GSTREAMERLIBS+=-lgstreamer-1.0 ^
    GSTREAMERLIBS+=-lgstapp-1.0 ^
    GSTREAMERLIBS+=-lgstpbutils-1.0 ^
    GSTREAMERLIBS+=-lgstaudio-1.0 ^
    GSTREAMERLIBS+=-lgstvideo-1.0

%MAKETOOL% -f Makefile qmake_all
%MAKETOOL% -j4
