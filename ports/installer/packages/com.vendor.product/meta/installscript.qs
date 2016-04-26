function Component()
{
    installer.installationFinished.connect(this, Component.prototype.installationFinishedPageIsShown);
    installer.finishButtonClicked.connect(this, Component.prototype.installationFinished);
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (systemInfo.productType === "windows") {
        // Create a shortcut in the application dir.
        component.addOperation("CreateShortcut",
                               "@TargetDir@\\bin\\webcamoid.exe",
                               "@TargetDir@\\webcamoid.lnk",
                               ' -q "@TargetDir@\\lib\\qt\\qml"',
                               ' -p "@TargetDir@\\lib\\avkys"',
                               ' -c "@TargetDir@\\share\\config"',
                               "workingDirectory=@TargetDir@\\bin",
                               "iconPath=@TargetDir@\\bin\\webcamoid.exe",
                               "iconId=0");

        // Create a shortcut in the menu.
        component.addOperation("CreateShortcut",
                               "@TargetDir@\\bin\\webcamoid.exe",
                               "@StartMenuDir@\\webcamoid.lnk",
                               ' -q "@TargetDir@\\lib\\qt\\qml"',
                               ' -p "@TargetDir@\\lib\\avkys"',
                               ' -c "@TargetDir@\\share\\config"',
                               "workingDirectory=@TargetDir@\\bin",
                               "iconPath=@TargetDir@\\bin\\webcamoid.exe",
                               "iconId=0");

        var libdir = installer.value("TargetDir") + "\\lib";
        var path = installer.environmentVariable("PATH");

        if (path.indexOf(libdir) < 0) {
            component.addOperation("EnvironmentVariable",
                                   "PATH",
                                   libdir + ";" + path,
                                   true,
                                   true);
        }

        var platformPlugins = installer.environmentVariable("QT_QPA_PLATFORM_PLUGIN_PATH");

        if (!platformPlugins || platformPlugins.length < 1) {
            component.addOperation("EnvironmentVariable",
                                   "QT_QPA_PLATFORM_PLUGIN_PATH",
                                   libdir + "\\qt\\plugins",
                                   true,
                                   true);
        }
    }
}

Component.prototype.installationFinishedPageIsShown = function()
{
    try {
        if (installer.isInstaller() && installer.status == QInstaller.Success)
            installer.addWizardPageItem(component,
                                        "AppLauncherForm",
                                        QInstaller.InstallationFinished);
    } catch(e) {
        console.log(e);
    }
}

Component.prototype.installationFinished = function()
{
    try {
        if (installer.isInstaller() && installer.status == QInstaller.Success) {
            var isReadMeCheckBoxChecked = component.userInterface("AppLauncherForm").launchApp.checked;

            if (isReadMeCheckBoxChecked)
                QDesktopServices.openUrl("file:///" + installer.value("TargetDir") + "/webcamoid.lnk");
        }
    } catch(e) {
        console.log(e);
    }
}
