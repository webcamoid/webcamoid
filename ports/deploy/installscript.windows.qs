function Component()
{
}

Component.prototype.beginInstallation = function()
{
    component.beginInstallation();
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    // Create shortcuts.
    var installDir = ["@TargetDir@", "@StartMenuDir@", "@DesktopDir@"];

    for (var dir in installDir)
        component.addOperation("CreateShortcut",
                                "@TargetDir@/bin/webcamoid.exe",
                                installDir[dir] + "/webcamoid.lnk");
}
