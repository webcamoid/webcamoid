if not "%DAILY_BUILD%" == "" (
    curl ^
        -T ports\deploy\packages_auto\windows\* ^
        -uhipersayanx:%BT_KEY% ^
        https://api.bintray.com/content/webcamoid/webcamoid/webcamoid/daily/windows
)
