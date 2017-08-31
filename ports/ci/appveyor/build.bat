qmake Webcamoid.pro ^
    CONFIG+=%CONFIGURATION% ^
    PREFIX="/webcamoid" ^
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

%MAKETOOL% -j4
