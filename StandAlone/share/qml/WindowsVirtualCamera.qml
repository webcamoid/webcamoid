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
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import Qt.labs.platform as LABS
import Ak
import AkControls as AK
import Webcamoid

Dialog {
    id: root
    title: qsTr("Using as virtual camera")
    standardButtons: Dialog.Close
    width: physicalWidth <= 100 || physicalHeight <= 100?
               wdgMainWidget.width: wdgMainWidget.width * 0.5
    height: physicalWidth <= 100 || physicalHeight <= 100?
                wdgMainWidget.height: wdgMainWidget.height * 0.5
    modal: true

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: dialogLayout.height
        clip: true

        property int bottomMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

        ColumnLayout {
            width: scrollView.width
            clip: true

            Label {
                text: qsTr("In Windows, you can use the network camera as any other system camera, for doing so, follow the next steps.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.bottomMargin: scrollView.bottomMargin
            }
            Label {
                text: qsTr("<b>Windows 11</b>: Go to Settings > Bluetooth & devices > Cameras. It will start searching for the local network cameras, select Search for cameras, then select Webcamoid's network camera.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.bottomMargin: scrollView.bottomMargin
            }
            Label {
                text: qsTr("<b>Windows 10</b>: Go to Settings > Devices > Bluetooth & other devices. Select Add Bluetooth or other device. In the Add a device popup, select Everything else. Wait for discovery to complete, then select Webcamoid's network camera.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.bottomMargin: scrollView.bottomMargin
            }
            Label {
                id: infoLabel
                text: qsTr("For more information you can read <a href=\"https://learn.microsoft.com/en-us/windows-hardware/drivers/stream/network-cameras\">Microsoft's network cameras documentation</a>")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true

                onLinkActivated: (link) => Qt.openUrlExternally(link)

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    cursorShape: infoLabel.hoveredLink?
                                        Qt.PointingHandCursor:
                                        Qt.ArrowCursor
                }
            }
        }
    }
}
