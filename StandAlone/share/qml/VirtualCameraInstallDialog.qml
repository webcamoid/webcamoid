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
import Ak
import Webcamoid

Dialog {
    id: root
    standardButtons: isDefaultVCam? (Dialog.Yes | Dialog.No): Dialog.Ok
    width: AkUnit.create(400 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(300 * AkTheme.controlScale, "dp").pixels
    modal: true
    title: qsTr("Virtual camera install")

    readonly property bool isDefaultVCam:
        virtualCameras.vcamDriver == virtualCameras.defaultVCamDriver

    signal openVCamDownloadDialog()
    signal openVCamManualDownloadDialog()

    onVisibleChanged: forceActiveFocus()

    ScrollView {
        id: view
        anchors.fill: parent

        ColumnLayout {
            width: view.width
            clip: true

            Label {
                text: Ak.platform == "windows"?
                          qsTr("The virtual camera in Windows may not work properly and may cause inestability in the system.<br/>")
                          + qsTr("If you want to share audio and video of your current seesion, consider using <b>local streaming</b> which is more stable and powerful.<br/><br/>")
                          + qsTr("Are you sure that you want to continue?"):
                      root.isDefaultVCam?
                          qsTr("The virtual camera is not installed, do you want to install it?"):
                          qsTr("The virtual camera is not installed. Please, install <b>v4l2loopback</b>.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }

    onAccepted: {
        if (root.isDefaultVCam)
            root.openVCamManualDownloadDialog()
    }
}
