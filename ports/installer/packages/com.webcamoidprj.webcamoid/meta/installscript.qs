function Component()
{
}

Component.prototype.beginInstallation = function()
{
    if (installer.value("os") === "win")
        installer.setValue("RunProgram", "@TargetDir@/bin/webcamoid.exe");
    else if (installer.value("os") === "x11")
        installer.setValue("RunProgram", "@TargetDir@/webcamoid");
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (systemInfo.productType === "windows") {
        // Create shortcuts.
        var installDir = ["@TargetDir@", "@StartMenuDir@", "@DesktopDir@"];

        for (var dir in installDir)
            component.addOperation("CreateShortcut",
                                   "@TargetDir@\\bin\\webcamoid.exe",
                                   installDir[dir] + "\\webcamoid.lnk");
    }
}
