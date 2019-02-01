if not "%DAILY_BUILD%" == "" (
rem      curl ^
rem          -T ports\deploy\packages_auto\windows\* ^
rem          -uhipersayanx:%BT_KEY% ^
rem          https://api.bintray.com/content/webcamoid/webcamoid/webcamoid/daily/windows

    jfrog bt upload ^
        --user hipersayanx ^
        --key %BT_KEY% ^
        --override true ^
        --publish true ^
        --licenses GPLv3+ ^
        --vcs-url "https://github.com/webcamoid/webcamoid.git" ^
        --pub-dn true ^
        --desc "Daily build" ^
        --labels daily-build ^
        --website-url "https://webcamoid.github.io" ^
        --issuetracker-url "https://github.com/webcamoid/webcamoid/issues" ^
        --github-repo "https://github.com/webcamoid/webcamoid" ^
        ports/deploy/packages_auto/windows/* ^
        webcamoid/webcamoid/webcamoid/daily ^
        windows/
)
