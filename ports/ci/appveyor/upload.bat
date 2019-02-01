if not "%DAILY_BUILD%" == "" (
rem      curl ^
rem          -T ports\deploy\packages_auto\windows\* ^
rem          -uhipersayanx:%BT_KEY% ^
rem          https://api.bintray.com/content/webcamoid/webcamoid/webcamoid/daily/windows

    for %%f in (ports\deploy\packages_auto\windows\*) do (
        jfrog bt upload ^
            --user=hipersayanx ^
            --key=%BT_KEY% ^
            --override=true ^
            --publish=true ^
            %%f ^
            webcamoid/webcamoid/webcamoid/daily ^
            windows/
    )
)
