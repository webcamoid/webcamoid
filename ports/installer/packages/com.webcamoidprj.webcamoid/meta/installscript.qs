function Component()
{
}

Component.prototype.beginInstallation = function()
{
    component.beginInstallation();

    if (installer.value("os") == "win")
        installer.setValue("RunProgram", "@TargetDir@/bin/webcamoid.exe");
    else if (installer.value("os") == "mac")
        installer.setValue("RunProgram", "@TargetDir@/webcamoid.app/Contents/MacOS/webcamoid");
    else if (installer.value("os") == "x11")
        installer.setValue("RunProgram", "@TargetDir@/bin/webcamoid");
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (installer.value("os") == "win") {
        // Create shortcuts.
        var installDir = ["@TargetDir@", "@StartMenuDir@", "@DesktopDir@"];

        for (var dir in installDir)
            component.addOperation("CreateShortcut",
                                   "@TargetDir@\\bin\\webcamoid.exe",
                                   installDir[dir] + "\\webcamoid.lnk");
    } else if (installer.value("os") == "x11") {
    }
}
