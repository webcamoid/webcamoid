if not "%DAILY_BUILD%" == "" (
    jfrog bt config ^
        --user=hipersayanx ^
        --key=%BT_KEY% ^
        --licenses=GPLv3+

    for %%f in (ports\deploy\packages_auto\windows\*) do (
rem          curl ^
rem              -T %%f ^
rem              -uhipersayanx:%BT_KEY% ^
rem              https://api.bintray.com/content/webcamoid/webcamoid/webcamoid/daily/windows

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
