/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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
import Qt.labs.settings 1.0
import Ak
import Webcamoid

Dialog {
    id: root
    standardButtons: Dialog.Yes | Dialog.No
    width: AkUnit.create(320 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(240 * AkTheme.controlScale, "dp").pixels
    modal: true
    title: qsTr("Virtual camera update")

    signal openVCamDownloadDialog()
    signal openVCamManualDownloadDialog()

    function openDialog() {
        if (showNextTime.checked)
            root.open()
    }

    onVisibleChanged: forceActiveFocus()

    ScrollView {
        id: view
        anchors.fill: parent

        ColumnLayout {
            width: view.width
            clip: true

            Label {
                text: qsTr("The virtual camera is outdated (%1), install the latest version (%2)?")
                        .arg(virtualCameras.currentVCamVersion)
                        .arg(updates.latestVersion("VirtualCamera"))
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

    Settings {
        category: "Updates"

        property alias showVCamDialog: showNextTime.checked
    }

    onAccepted: {
        if (Ak.platform() != "macos" && virtualCameras.downloadVCam())
            root.openVCamDownloadDialog()
        else
            root.openVCamManualDownloadDialog()
    }
}
