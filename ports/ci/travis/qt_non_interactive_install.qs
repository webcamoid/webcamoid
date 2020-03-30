function Controller()
{
    installer.autoRejectMessageBoxes();
    installer.setMessageBoxAutomaticAnswer("OverwriteTargetDirectory", QMessageBox.Yes);
    installer.setMessageBoxAutomaticAnswer("stopProcessesForUpdates", QMessageBox.Ignore);
    installer.installationFinished.connect(function() {
        gui.clickButton(buttons.NextButton);
    })
}

Controller.prototype.WelcomePageCallback = function()
{
    gui.clickButton(buttons.NextButton, 10000);
}

Controller.prototype.CredentialsPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.IntroductionPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.TargetDirectoryPageCallback = function()
{
    //gui.currentPageWidget().TargetDirectoryLineEdit.setText(installer.value("HomeDir") + "/Qt");
    gui.currentPageWidget().TargetDirectoryLineEdit.setText(installer.value("InstallerDirPath") + "/Qt");
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ComponentSelectionPageCallback = function()
{
    var widget = gui.currentPageWidget();

    widget.deselectAll();
    widget.selectComponent("qt.qt5.5141.android");

    gui.clickButton(buttons.NextButton);
}

Controller.prototype.LicenseAgreementPageCallback = function()
{
    gui.currentPageWidget().AcceptLicenseRadioButton.setChecked(true);
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ReadyForInstallationPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.FinishedPageCallback = function()
{
    var checkBoxForm = gui.currentPageWidget().LaunchQtCreatorCheckBoxForm

    if (checkBoxForm && checkBoxForm.launchQtCreatorCheckBox)
        checkBoxForm.launchQtCreatorCheckBox.checked = false;

    gui.clickButton(buttons.FinishButton);
}
