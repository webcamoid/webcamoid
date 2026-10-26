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
import Webcamoid

ScrollView {
    id: view

    property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    signal removed()
    signal openLocalStreamingAdvancedDialog()
    signal openWindowsVirtualCameraDialog()

    // Copies the text to the clipboard.
    function copyToClipboard(text)
    {
        clipboardHelper.text = text
        clipboardHelper.selectAll()
        clipboardHelper.copy()
    }

    ColumnLayout {
        width: view.width
        spacing: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        layoutDirection: view.rtl? Qt.RightToLeft: Qt.LeftToRight
        clip: true

        TextEdit {
            id: clipboardHelper
            visible: false
        }

        Button {
            text: qsTr("Remove")
            icon.source: "image://icons/no"
            flat: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin

            onClicked: {
                localStreaming.location = ""
                view.removed()
            }
        }

        Button {
            text: qsTr("Use as virtual camera")
            highlighted: true
            icon.source: "image://icons/webcam"
            visible: Ak.platform() == "windows"
            Layout.fillWidth: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Accessible.description: qsTr("Use the network camera as a virtual camera")

            onClicked: view.openWindowsVirtualCameraDialog()
        }

        // Ports

        GridLayout {
            columns: 2
            layoutDirection: view.rtl? Qt.RightToLeft: Qt.LeftToRight
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true

            Label {
                id: httpPort
                text: qsTr("HTTP port")
            }
            SpinBox {
                id: httpPortSpinBox
                value: localStreaming.httpPort
                from: 1024
                to: 65535
                editable: true
                Layout.leftMargin: view.leftMargin
                Layout.rightMargin: view.rightMargin
                Accessible.name: httpPort.text

                // The ports are not quantities: no thousands separators.
                textFromValue: function (value, locale) {
                    return String(value)
                }
                valueFromText: function (text, locale) {
                    let port = parseInt(text)

                    return isNaN(port)? value: port
                }

                onValueModified: localStreaming.httpPort = value
            }

            Label {
                id: rtspPort
                text: qsTr("RTSP port")
            }
            SpinBox {
                id: rtspPortSpinBox
                value: localStreaming.rtspPort
                from: 1024
                to: 65535
                editable: true
                Layout.leftMargin: view.leftMargin
                Layout.rightMargin: view.rightMargin
                Accessible.name: rtspPort.text

                textFromValue: function (value, locale) {
                    return String(value)
                }
                valueFromText: function (text, locale) {
                    let port = parseInt(text)

                    return isNaN(port)? value: port
                }

                onValueModified: localStreaming.rtspPort = value
            }
        }

        // Resource name

        Label {
            text: qsTr("Resource name")
            font.bold: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true
        }
        TextField {
            id: resourceField
            text: localStreaming.resource
            placeholderText: "stream"
            selectByMouse: true
            validator: RegularExpressionValidator {
                regularExpression: /[A-Za-z0-9_.-]+/
            }
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true
            Accessible.name: qsTr("Resource name")

            onEditingFinished: {
                localStreaming.resource = text

                // An empty name goes back to the default one.
                text = localStreaming.resource
            }

            Connections {
                target: localStreaming

                function onResourceChanged(resource)
                {
                    resourceField.text = resource
                }
            }
        }

        // Authentication

        Switch {
            id: authSwitch
            text: qsTr("Enable authentication")
            checked: localStreaming.authEnabled
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true

            onToggled: localStreaming.authEnabled = checked
        }

        // Empty credentials are filled when the authentication is enabled.

        ColumnLayout {
            visible: localStreaming.authEnabled
            spacing: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.fillWidth: true

            Label {
                text: qsTr("User")
                font.bold: true
                Layout.leftMargin: view.leftMargin
                Layout.rightMargin: view.rightMargin
                Layout.fillWidth: true
            }
            TextField {
                id: userField
                text: localStreaming.user
                selectByMouse: true
                Layout.leftMargin: view.leftMargin
                Layout.rightMargin: view.rightMargin
                Layout.fillWidth: true
                Accessible.name: qsTr("User")

                onEditingFinished: {
                    localStreaming.user = text
                    text = localStreaming.user
                }

                Connections {
                    target: localStreaming

                    function onUserChanged(user)
                    {
                        userField.text = user
                    }
                }
            }

            Label {
                text: qsTr("Password")
                font.bold: true
                Layout.leftMargin: view.leftMargin
                Layout.rightMargin: view.rightMargin
                Layout.fillWidth: true
            }
            TextField {
                id: passwordField
                text: localStreaming.password
                selectByMouse: true
                Layout.leftMargin: view.leftMargin
                Layout.rightMargin: view.rightMargin
                Layout.fillWidth: true
                Accessible.name: qsTr("Password")

                onEditingFinished: {
                    localStreaming.password = text
                    text = localStreaming.password
                }

                Connections {
                    target: localStreaming

                    function onPasswordChanged(password)
                    {
                        passwordField.text = password
                    }
                }
            }
        }

        // Streaming URL

        Label {
            text: qsTr("Streaming URL")
            font.bold: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true
        }
        RowLayout {
            layoutDirection: view.rtl? Qt.RightToLeft: Qt.LeftToRight
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true

            // The streams are chosen in the web page, and the RTSP ones are
            // found by the ONVIF discovery.
            AK.ActionTextField {
                id: localStreamingUrlField
                icon.source: "image://icons/internet"
                labelText: localStreaming.location
                placeholderText: localStreaming.defaultURL
                buttonText: qsTr("Open URL in the webbrowser")
                readOnly: true
                Layout.fillWidth: true

                onButtonClicked: Qt.openUrlExternally(localStreaming.location)
            }
            Button {
                text: qsTr("Copy")
                icon.source: "image://icons/copy"
                flat: true
                display: AbstractButton.IconOnly
                enabled: localStreaming.location !== ""
                implicitWidth: implicitHeight
                ToolTip.visible: hovered
                ToolTip.text: help
                Accessible.name: text
                Accessible.description: help

                readonly property string help:
                    qsTr("Copy the URL to the clipboard")

                onClicked: view.copyToClipboard(localStreaming.location)
            }
        }

        Button {
            id: btnAdvancedOptions
            text: qsTr("Advanced options")
            icon.source: "image://icons/settings"
            flat: true
            Layout.leftMargin: view.leftMargin
            Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels

            onClicked: view.openLocalStreamingAdvancedDialog()
        }
    }
}
