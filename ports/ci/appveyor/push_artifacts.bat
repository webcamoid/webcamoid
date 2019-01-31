if not "%DAILY_BUILD%" == "" (
    appveyor PushArtifact ports/deploy/packages_auto/*/*
)
