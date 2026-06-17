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

ScrollView {
    id: view

    property string videoOutput: ""
    property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    signal videoOutputRemoved()

    function setOutput(videoOutput)
    {
        view.videoOutput = videoOutput
        let url = streaming.platformWebsite(videoOutput)
        deviceInfo.text =
                "<b>" + videoOutput + "</b><br/>"
                + "<i>" + url + "</i>"
    }

    ColumnLayout {
        width: view.width
        spacing: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        layoutDirection: view.rtl? Qt.RightToLeft: Qt.LeftToRight

        Label {
            id: deviceInfo
            elide: Label.ElideRight
            Layout.fillWidth: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.bottomMargin:
                AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        }
        Button {
            text: qsTr("Remove")
            icon.source: "image://icons/no"
            flat: true
            Layout.leftMargin: view.leftMargin

            onClicked: {
                streaming.platforms =
                    streaming.platforms.filter(p => p !== view.videoOutput)
                view.videoOutputRemoved()
            }
        }
        Label {
            text: qsTr("Website")
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

            TextField {
                id: websiteField
                placeholderText: "https://www.website.com/"
                readOnly: true
                text: streaming.platformWebsite(view.videoOutput)
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Visit website")
                icon.source: "image://icons/internet"
                flat: true
                display: AbstractButton.IconOnly
                implicitWidth: implicitHeight
                ToolTip.visible: hovered
                ToolTip.text: text
                Accessible.name: text
                Accessible.description: text

                onClicked: Qt.openUrlExternally(websiteField.text)
            }
        }

        Label {
            text: qsTr("Streaming URL")
            font.bold: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true
        }
        RowLayout {
            id: streaminhUrlLayout
            layoutDirection: view.rtl? Qt.RightToLeft: Qt.LeftToRight
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true

            property bool isStreaminhUrlVisible: false

            TextField {
                id: streamingUrlField
                text: streaming.platformStreamingUrl(view.videoOutput)
                placeholderText: "rtmp://streaming.website.com/${KEY}"
                readOnly: streaming.platformNeedsKey(view.videoOutput)
                echoMode: streaminhUrlLayout.isStreaminhUrlVisible
                          && (streaming.state != AkElement.ElementStatePlaying
                              || videoLayer.deviceType(videoLayer.videoInput) != VideoLayer.InputScreen)?
                            TextInput.Normal:
                            TextInput.Password
                Layout.fillWidth: true

                onEditingFinished: streaming.setPlatformStreamingUrl(view.videoOutput, text)
            }
            Button {
                text: streaminhUrlLayout.isStreaminhUrlVisible?
                        qsTr("Hide streaming URL"):
                        qsTr("Show streaming URL")
                icon.source: streaminhUrlLayout.isStreaminhUrlVisible?
                                "image://icons/closed-eye":
                                "image://icons/open-eye"
                flat: true
                display: AbstractButton.IconOnly
                enabled: streaming.state != AkElement.ElementStatePlaying
                         || videoLayer.deviceType(videoLayer.videoInput) != VideoLayer.InputScreen
                implicitWidth: implicitHeight
                ToolTip.visible: hovered
                ToolTip.text: text
                Accessible.name: text
                Accessible.description: text

                onClicked: streaminhUrlLayout.isStreaminhUrlVisible = !streaminhUrlLayout.isStreaminhUrlVisible
            }
        }

        Label {
            text: qsTr("Streaming key")
            font.bold: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true
            visible: streaming.platformNeedsKey(view.videoOutput)
        }
        RowLayout {
            id: keyLayout
            layoutDirection: view.rtl? Qt.RightToLeft: Qt.LeftToRight
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.fillWidth: true
            visible: streaming.platformNeedsKey(view.videoOutput)

            property bool isKeyVisible: false

            TextField {
                id: keyField
                text: streaming.platformStreamingKey(view.videoOutput)
                placeholderText: "********************"
                Layout.fillWidth: true
                echoMode: keyLayout.isKeyVisible
                          && (streaming.state != AkElement.ElementStatePlaying
                              || videoLayer.deviceType(videoLayer.videoInput) != VideoLayer.InputScreen)?
                            TextInput.Normal:
                            TextInput.Password

                onEditingFinished:
                    streaming.setPlatformStreamingKey(view.videoOutput, text)
            }
            Button {
                text: keyLayout.isKeyVisible?
                        qsTr("Hide streaming key"):
                        qsTr("Show streaming key")
                icon.source: keyLayout.isKeyVisible?
                                "image://icons/closed-eye":
                                "image://icons/open-eye"
                flat: true
                display: AbstractButton.IconOnly
                enabled: streaming.state != AkElement.ElementStatePlaying
                         || videoLayer.deviceType(videoLayer.videoInput) != VideoLayer.InputScreen
                implicitWidth: implicitHeight
                ToolTip.visible: hovered
                ToolTip.text: text
                Accessible.name: text
                Accessible.description: text

                onClicked: keyLayout.isKeyVisible = !keyLayout.isKeyVisible
            }
        }

        Button {
            text: qsTr("Streaming configuration help")
            Layout.leftMargin: view.leftMargin
            flat: true

            onClicked: {
                let url = streaming.platformDocsUrl(view.videoOutput)

                if (url)
                    Qt.openUrlExternally(url)
            }
        }
        Button {
            text: qsTr("Get streaming key")
            Layout.leftMargin: view.leftMargin
            flat: true
            visible: streaming.platformNeedsKey(view.videoOutput)

            onClicked: {
                let url = streaming.platformKeyConfigsUrl(view.videoOutput)

                if (url)
                    Qt.openUrlExternally(url)
            }
        }
    }
}
