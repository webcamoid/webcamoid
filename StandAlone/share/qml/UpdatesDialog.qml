/* Webcamoid, webcam capture application.
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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0 as LABS
import Ak 1.0
import Webcamoid 1.0

Dialog {
    id: updatesDialog
    standardButtons: Dialog.Yes | Dialog.No
    width: AkUnit.create(420 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(240 * AkTheme.controlScale, "dp").pixels
    modal: true
    title: qsTr("New version available!")

    property int webcamoidStatus: updates.status("Webcamoid")
    property string webcamoidLatestVersion: updates.latestVersion("Webcamoid")

    function notifyUpdate()
    {
        if (updates.notifyNewVersion
            && showNextTime.checked
            && webcamoidStatus == Updates.ComponentOutdated) {
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
            if (component == "Webcamoid") {
                updatesDialog.webcamoidStatus = updates.status("Webcamoid");
                updatesDialog.webcamoidLatestVersion = latestVersion;
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
                text: qsTr("Download %1 %2 NOW!")
                        .arg(mediaTools.applicationName)
                        .arg(updatesDialog.webcamoidLatestVersion)
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

    onAccepted: Qt.openUrlExternally(mediaTools.projectDownloadsUrl)

    LABS.Settings {
        category: "Updates"

        property alias showDialog: showNextTime.checked
    }
}
