dir

if not "%DAILY_BUILD%" == "" (
    appveyor PushArtifact ports\deploy\packages_auto\windows\webcamoid-portable-8.5.0-win32.zip
    appveyor PushArtifact ports\deploy\packages_auto\windows\webcamoid-8.5.0-win32.exe
)

dir
