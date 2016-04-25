@echo off
set PATH=%cd%\lib;%PATH%
start /b bin\webcamoid -r -q "%cd%\lib\qt\qml" -p "%cd%\lib\AvKys" -c "%cd%\share\config"
