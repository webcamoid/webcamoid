/* Webcamoid, camera capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import Qt.labs.settings 1.0
import Ak
import Webcamoid

Dialog {
    id: updatesDialog
    title: {
        if (!dailyMode)
            return qsTr("New version available!")

        switch (stage) {
        case "downloading":
            return qsTr("Downloading the new daily build")
        case "downloaded":
            return qsTr("New daily build ready")
        case "error":
            return qsTr("The download failed")
        default:
            return qsTr("New daily build available!")
        }
    }
    width: AkUnit.create(420 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create((dailyMode? 280: 240) * AkTheme.controlScale, "dp").pixels
    modal: true

    property bool dailyMode: false
    property string dailyCommit: ""
    // The user accepted to download the daily build.
    property bool downloadRequested: false
    property int webcamoidStatus: updates.status("Webcamoid")
    property string webcamoidLatestVersion: updates.latestVersion("Webcamoid")

    // prompt: asks to download, downloading, downloaded: waits for the user
    // to run it, error.
    readonly property string stage: {
        if (!dailyMode || !downloadRequested)
            return "prompt"

        if (dailyBuilds.state == DailyBuilds.Downloading)
            return "downloading"

        // An error after the download, for example when the installer cannot
        // be started, is shown with the Install button to try again.
        if (dailyBuilds.state == DailyBuilds.Downloaded)
            return "downloaded"

        if (dailyBuilds.error != DailyBuilds.NoError)
            return "error"

        // The download did not start, for example because it was checking
        // for a new build.
        return "prompt"
    }
    readonly property bool canInstall:
        dailyBuilds.installKind != DailyBuilds.InstallUnsupported
    // It is not possible to download again when the file was downloaded but
    // it could not be started.
    readonly property bool canRetry:
        dailyBuilds.state != DailyBuilds.Downloaded
        && (dailyBuilds.error == DailyBuilds.NetworkError
            || dailyBuilds.error == DailyBuilds.RateLimited
            || dailyBuilds.error == DailyBuilds.DownloadCorrupt
            || dailyBuilds.error == DailyBuilds.DownloadFailed)

    function notifyUpdate()
    {
        if (updates.isEnabled
            && updates.notifyNewVersion
            && showNextTime.checked
            && webcamoidStatus == Updates.ComponentOutdated) {
            dailyMode = false
            open()
        }
    }

    function startDownload()
    {
        // The platforms without an installer only offer the downloads page.
        if (!canInstall) {
            Qt.openUrlExternally(mediaTools.projectDownloadsUrl)
            close()

            return
        }

        dailyBuilds.resetError()
        downloadRequested = true
        dailyBuilds.download()
    }

    function sizeText()
    {
        return qsTr("%1 MB").arg(Math.round(dailyBuilds.buildSize / 1048576))
    }

    function progressText()
    {
        return qsTr("%1% of %2")
                .arg(Math.round(dailyBuilds.progress * 100))
                .arg(sizeText())
    }

    function escapeHtml(text)
    {
        return text.replace(/&/g, "&amp;")
                   .replace(/</g, "&lt;")
                   .replace(/>/g, "&gt;")
    }

    Component.onCompleted: notifyUpdate()
    onWebcamoidLatestVersionChanged: notifyUpdate()
    onVisibleChanged: forceActiveFocus()

    Connections {
        target: updates

        function onNewVersionAvailable(component, latestVersion)
        {
            if (updates.isEnabled && component == "Webcamoid") {
                updatesDialog.webcamoidStatus = updates.status("Webcamoid");
                updatesDialog.webcamoidLatestVersion = latestVersion;
            }
        }
    }

    Connections {
        target: dailyBuilds

        function onNewBuildAvailable(commit)
        {
            if (updates.isEnabled
                && updates.notifyNewVersion
                && showNextTime.checked
                && dailyBuilds.channel == DailyBuilds.Daily
                && dailyBuilds.state != DailyBuilds.Downloading) {
                updatesDialog.dailyMode = true
                updatesDialog.dailyCommit = commit
                updatesDialog.downloadRequested = false
                updatesDialog.open()
            }
        }
    }

    ScrollView {
        id: view
        anchors.fill: parent

        ColumnLayout {
            width: view.width
            clip: true

            Label {
                visible: updatesDialog.stage == "prompt"
                text: updatesDialog.dailyMode?
                          qsTr("A new daily build is available (commit %1). Download it now?")
                            .arg(updatesDialog.dailyCommit):
                          qsTr("Download %1 %2 NOW!")
                            .arg(mediaTools.applicationName)
                            .arg(updatesDialog.webcamoidLatestVersion)
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                visible: updatesDialog.stage == "downloading"
                text: qsTr("Downloading and verifying the new build. You can keep using Webcamoid meanwhile.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            ProgressBar {
                visible: updatesDialog.stage == "downloading"
                value: dailyBuilds.progress
                indeterminate: dailyBuilds.progress <= 0
                Layout.fillWidth: true
            }
            Label {
                visible: updatesDialog.stage == "downloading"
                text: updatesDialog.progressText()
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                visible: updatesDialog.stage == "downloaded"
                text: {
                    switch (dailyBuilds.installKind) {
                    case DailyBuilds.InstallPackage:
                        return qsTr("Download complete and verified. Android will ask you to confirm the installation.")
                    case DailyBuilds.InstallRunsInstaller:
                        return qsTr("Download complete and verified. Webcamoid will close to run the installer.")
                    case DailyBuilds.InstallBuildsFromSource:
                        return qsTr("Download complete and verified. The new version will be compiled and installed from a Terminal window, you can keep using Webcamoid meanwhile.")
                    default:
                        return qsTr("Download complete and verified. Webcamoid will close to run the new version.")
                    }
                }
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                visible: updatesDialog.stage == "downloaded"
                         && dailyBuilds.installerLaunched
                text: qsTr("The installer is running in Terminal. When the compilation ends, it will ask you to close Webcamoid to finish the installation.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                visible: updatesDialog.stage == "downloaded"
                         && dailyBuilds.error != DailyBuilds.NoError
                text: dailyBuilds.errorMessage
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                visible: updatesDialog.stage == "downloaded"
                         || updatesDialog.stage == "downloading"
                text: updatesDialog.escapeHtml(dailyBuilds.downloadPath)
                textFormat: Text.StyledText
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                Layout.fillWidth: true
            }
            Label {
                visible: updatesDialog.stage == "error"
                text: dailyBuilds.errorMessage
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            CheckBox {
                id: showNextTime
                visible: updatesDialog.stage == "prompt"
                text: qsTr("Show this dialog next time")
                checked: true
                Layout.fillWidth: true
            }
        }
    }

    footer: Item {
        implicitHeight: buttonsRow.implicitHeight
                        + 2 * AkUnit.create(8 * AkTheme.controlScale, "dp").pixels

        RowLayout {
            id: buttonsRow
            anchors.fill: parent
            anchors.margins: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels

            Item {
                Layout.fillWidth: true
            }
            // Primary action of the current stage.
            Button {
                text: {
                    switch (updatesDialog.stage) {
                    case "downloading":
                        return qsTr("Hide")
                    case "downloaded":
                        if (dailyBuilds.installerLaunched)
                            return qsTr("Run again")

                        return dailyBuilds.installKind == DailyBuilds.InstallRunsInstaller
                               || dailyBuilds.installKind == DailyBuilds.InstallPackage
                               || dailyBuilds.installKind == DailyBuilds.InstallBuildsFromSource?
                                   qsTr("Install"):
                                   qsTr("Run")
                    case "error":
                        return qsTr("Try again")
                    default:
                        return qsTr("Yes")
                    }
                }
                highlighted: true
                visible: updatesDialog.stage != "error"
                         || updatesDialog.canRetry

                onClicked: {
                    switch (updatesDialog.stage) {
                    case "downloading":
                        // It keeps downloading, the progress is also shown in
                        // the preferences.
                        updatesDialog.close()
                        break
                    case "downloaded":
                        installDialog.open()
                        break
                    case "error":
                        updatesDialog.startDownload()
                        break
                    default:
                        if (updatesDialog.dailyMode) {
                            updatesDialog.startDownload()
                        } else {
                            Qt.openUrlExternally(mediaTools.projectDownloadsUrl)
                            updatesDialog.close()
                        }
                    }
                }
            }
            // Secondary action of the current stage.
            Button {
                text: {
                    switch (updatesDialog.stage) {
                    case "downloading":
                        return qsTr("Cancel")
                    case "downloaded":
                        return qsTr("Later")
                    case "error":
                        return qsTr("Close")
                    default:
                        return qsTr("No")
                    }
                }

                onClicked: {
                    if (updatesDialog.stage == "downloading")
                        dailyBuilds.cancelDownload()

                    updatesDialog.close()
                }
            }
        }
    }

    DailyBuildInstallDialog {
        id: installDialog
    }

    Settings {
        category: "Updates"

        property alias showDialog: showNextTime.checked
    }
}
