if not "%DAILY_BUILD%" == "" (
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
        "ports/deploy/packages_auto/windows/*" ^
        webcamoid/webcamoid/webcamoid/daily ^
        windows/
)
