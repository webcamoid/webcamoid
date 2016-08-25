#!/bin/sh

dataFolder=$(ls -A ./packages/com.webcamoidprj.webcamoid/data)

if [ -n "$dataFolder" ]; then
    wine ~/.wine/drive_c/Qt/QtIFW2.0.3/bin/binarycreator.exe -c config/config.xml -p packages webcamoid-7.2.1-win32.exe
else
    echo "Put the files to install into ./packages/com.webcamoidprj.webcamoid/data"
fi
