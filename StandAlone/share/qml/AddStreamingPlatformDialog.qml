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
import AkControls as AK

Dialog {
    id: root
    title: qsTr("Add platform")
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel

    signal acceptedPlatform(var platform)

    onVisibleChanged: nameField.forceActiveFocus()

    // Reset fields each time the dialog opens
    onOpened: {
        nameField.text = ""
        websiteField.text = ""
        streamingUrlField.text = ""
        keyConfigsUrlField.text = ""
        docsUrlField.text = ""
        needsKeyCheck.checked = true
    }

    ScrollView {
        id: view
        anchors.fill: parent

        ColumnLayout {
            width: view.width

            ColumnLayout {
                Layout.fillWidth: true

                Label {
                    text: qsTr("Name")
                    font.bold: true
                    Layout.fillWidth: true
                }
                TextField {
                    id: nameField
                    placeholderText: qsTr("My Platform")
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Website")
                    font.bold: true
                    Layout.fillWidth: true
                }
                TextField {
                    id: websiteField
                    placeholderText: "https://www.website.com/"
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Streaming URL")
                    font.bold: true
                    Layout.fillWidth: true
                }
                TextField {
                    id: streamingUrlField
                    placeholderText: "rtmp://streaming.website.com/"
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Key configs URL")
                    font.bold: true
                    Layout.fillWidth: true
                }
                TextField {
                    id: keyConfigsUrlField
                    placeholderText: "https://www.website.com/settings"
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Documentation URL")
                    font.bold: true
                    Layout.fillWidth: true
                }
                TextField {
                    id: docsUrlField
                    placeholderText: "https://www.website.com/docs"
                    Layout.fillWidth: true
                }

                CheckBox {
                    id: needsKeyCheck
                    text: qsTr("Needs key")
                    checked: true
                    Layout.fillWidth: true
                }
            }
        }
    }

    onAccepted: {
        root.acceptedPlatform({
            "name":          nameField.text,
            "website":       websiteField.text,
            "streamingUrl":  streamingUrlField.text,
            "keyConfigsUrl": keyConfigsUrlField.text,
            "docsUrl":       docsUrlField.text,
            "needsKey":      needsKeyCheck.checked
        })
    }
}
