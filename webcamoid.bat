@echo off
set PATH=%cd%\lib;%PATH%
set QT_QPA_PLATFORM_PLUGIN_PATH=%cd%\lib\qt\plugins
start /b bin\webcamoid -q "%cd%\lib\qt\qml" -p "%cd%\lib\avkys" -c "%cd%\share\config"
