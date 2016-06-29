@echo off
REM SET QMLSCENE_DEVICE=softwarecontext
start /b bin\webcamoid -q "%cd%\lib\qt\qml" -p "%cd%\lib\avkys" -c "%cd%\share\config"
