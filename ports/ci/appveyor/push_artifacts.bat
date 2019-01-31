if not "%DAILY_BUILD%" == "" (
    for %%f in (ports\deploy\packages_auto\windows\*) do (
        appveyor PushArtifact %%f
    )
)
