set PACKAGES_PATH=ports\deploy\packages_auto\windows

if not "%DAILY_BUILD%" == "" (
    for %%f in (%PACKAGES_PATH%\*) do (
        appveyor PushArtifact %PACKAGES_PATH%\%%f
    )
)
