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
    title: dailyMode?
               qsTr("New daily build available!"):
               qsTr("New version available!")
    standardButtons: Dialog.Yes | Dialog.No
    width: AkUnit.create(420 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(240 * AkTheme.controlScale, "dp").pixels
    modal: true

    property bool dailyMode: false
    property string dailyCommit: ""
    property int webcamoidStatus: updates.status("Webcamoid")
    property string webcamoidLatestVersion: updates.latestVersion("Webcamoid")

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
                && dailyBuilds.channel == DailyBuilds.Daily) {
                updatesDialog.dailyMode = true
                updatesDialog.dailyCommit = commit
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
                text: updatesDialog.dailyMode?
                          qsTr("A new daily build is available (commit %1). Download it now?")
                            .arg(updatesDialog.dailyCommit):
                          qsTr("Download %1 %2 NOW!")
                            .arg(mediaTools.applicationName)
                            .arg(updatesDialog.webcamoidLatestVersion)
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            CheckBox {
                id: showNextTime
                text: qsTr("Show this dialog next time")
                checked: true
                Layout.fillWidth: true
            }
        }
    }

    onAccepted: {
        if (dailyMode)
            dailyBuilds.download()
        else
            Qt.openUrlExternally(mediaTools.projectDownloadsUrl)
    }

    Settings {
        category: "Updates"

        property alias showDialog: showNextTime.checked
    }
}
