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
import AkControls as AK
import Webcamoid

ScrollView {
    id: view

    property int localPort: 8080
    property string localResource: "stream"
    property string localFormatId: localStreaming.locationFormat(localStreaming.location)
                                    || localStreaming.defaultVideoFormat
    property string localFormat: view.extensionForFormat(view.localFormatId)
    property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    signal removed()
    signal openLocalStreamingAdvancedDialog()

    // Extracts the muxer/extension part ("webm", "mp4", etc.) from a
    // canonical "pluginID:name" format id. Falls back to the input itself
    // if it doesn't follow that convention.
    function extensionForFormat(formatId)
    {
        if (!formatId)
            return ""

        let parts = formatId.split(':')

        return parts.length > 1? parts[1]: formatId
    }

    function parseLocalUrl(url)
    {
        if (!url)
            return { port: 8080, resource: "stream", format: "webm" }

        let match = url.match(/:(\d+)\/(.+)\.(webm|mp4)$/i)

        if (match) {
            return {
                port: parseInt(match[1]),
                resource: match[2],
                format: match[3].toLowerCase()
            }
        }

        let addrMatch = url.match(/(https?:\/\/[^:]+)/)

        if (addrMatch)
            return { port: 8080, resource: "stream", format: "webm" }

        return { port: 8080, resource: "stream", format: "webm" }
    }

    function buildLocalUrl()
    {
        let address = localStreaming.location
        let addrMatch = address.match(/(https?:\/\/[^:/]+)/)
        let base = addrMatch? addrMatch[1]: "http://localhost"

        return base + ":" + view.localPort + "/" + view.localResource + "." + view.localFormat
    }

    function updateLocalUrl()
    {
        localStreaming.location = view.buildLocalUrl()
    }

    ColumnLayout {
        width: view.width
        spacing: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        layoutDirection: view.rtl? Qt.RightToLeft: Qt.LeftToRight

        Button {
            text: qsTr("Remove")
            icon.source: "image://icons/no"
            flat: true
            Layout.leftMargin: view.leftMargin

            onClicked: {
                localStreaming.location = ""
                view.removed()
            }
        }
        RowLayout {
            layoutDirection: view.rtl? Qt.RightToLeft: Qt.LeftToRight
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true

            Label {
                text: qsTr("Port")
                font.bold: true
            }
            SpinBox {
                id: portSpinBox
                from: 1024
                to: 65535
                value: view.localPort

                onValueChanged: {
                    if (view.localPort !== value) {
                        view.localPort = value
                        view.updateLocalUrl()
                    }
                }
            }
        }

        Label {
            text: qsTr("Format")
            font.bold: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true
        }
        ComboBox {
            id: formatCombo
            model: localStreaming.videoFormats
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true

            // The model holds canonical "pluginID:name" ids; show the
            // human-readable description instead of the raw id.
            delegate: ItemDelegate {
                width: formatCombo.width
                text: view.extensionForFormat(modelData)
                highlighted: formatCombo.highlightedIndex === index
            }
            displayText: currentIndex >= 0 && currentIndex < model.length?
                             view.extensionForFormat(model[currentIndex]):
                             ""

            // Keep the selection in sync whenever the available formats or
            // the current location-derived format id change.
            function syncCurrentIndex()
            {
                let idx = formatCombo.model.indexOf(view.localFormatId)

                if (idx < 0)
                    idx = formatCombo.model.indexOf(localStreaming.defaultVideoFormat)

                formatCombo.currentIndex = idx >= 0? idx: 0
            }

            Component.onCompleted: formatCombo.syncCurrentIndex()

            Connections {
                target: view

                function onLocalFormatIdChanged()
                {
                    formatCombo.syncCurrentIndex()
                }
            }

            onActivated: {
                let formatId = formatCombo.model[currentIndex]

                if (view.localFormatId !== formatId) {
                    view.localFormatId = formatId
                    view.updateLocalUrl()
                }
            }
        }

        Label {
            text: qsTr("Resource name")
            font.bold: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true
        }
        TextField {
            id: resourceField
            text: view.localResource
            placeholderText: "stream"
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true

            onEditingFinished: {
                if (view.localResource !== text) {
                    view.localResource = text
                    view.updateLocalUrl()
                }
            }
        }

        Label {
            text: qsTr("Streaming URL")
            font.bold: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true
        }
        AK.ActionTextField {
            id: localStreamingUrlField
            icon.source: "image://icons/internet"
            labelText: {
                // Dummy dependencies to force reevaluation
                view.localPort
                view.localResource
                view.localFormat

                return view.buildLocalUrl()
            }
            placeholderText: localStreaming.defaultURL
            buttonText: qsTr("Open URL in the webbrowser")
            readOnly: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true

            onButtonClicked: Qt.openUrlExternally(localStreaming.location)
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

    Settings {
        category: "LocalStreamingConfigs"

        property alias localPort: view.localPort
        property alias localResource: view.localResource
    }
}
