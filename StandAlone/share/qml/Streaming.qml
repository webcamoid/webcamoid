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

AK.MenuOption {
    id: root
    title: qsTr("Streaming")
    subtitle: qsTr("Configure video streaming to many platforms.")
    icon: "image://icons/broadcast"

    property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft
    readonly property bool hasActiveScreenSource: {
        let ids = videoLayer.sourceIds()

        for (let i = 0; i < ids.length; i++) {
            if (videoLayer.sourceEnabled(ids[i])
                && videoLayer.sourceType(ids[i]) == VideoLayer.InputScreen)
                return true
        }

        return false
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: layout.height
        clip: true

        ColumnLayout {
            id: layout
            width: scrollView.width

            readonly property string currentPlatform:
                streaming.supportedPlatforms[platformCombo.currentIndex] ?? ""

            // Changes each time the codecs change, to force the update of the
            // bindings that call functions.
            property int revision: 0
            readonly property var videoCodecs: {
                revision

                return streaming.supportedCodecs(currentPlatform, AkCaps.CapsVideo)
            }
            readonly property var audioCodecs: {
                revision

                return streaming.supportedCodecs(currentPlatform, AkCaps.CapsAudio)
            }

            Connections {
                target: streaming

                function onSupportedCodecsChanged() {
                    layout.revision++
                }

                function onCodecChanged(type, codec) {
                    layout.revision++
                }
            }

            // The codecs are the generic ones. The bitrate, the keyframes
            // interval, the sampling rate and the encoder used for each
            // codec are configured in the encoding options.

            AK.LabeledComboBox {
                id: videoCodecCombo
                label: qsTr("Video codec")
                visible: layout.videoCodecs.length > 1
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
                model: layout.videoCodecs.map(c => streaming.codecDescription(c))
                currentIndex: Math.max(layout.videoCodecs.indexOf(streaming.codec(AkCaps.CapsVideo)), 0)

                onActivated: streaming.setCodec(AkCaps.CapsVideo,
                                                layout.videoCodecs[index])
            }
            AK.LabeledComboBox {
                id: audioCodecCombo
                label: qsTr("Audio codec")
                visible: layout.audioCodecs.length > 1
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
                model: layout.audioCodecs.map(c => streaming.codecDescription(c))
                currentIndex: Math.max(layout.audioCodecs.indexOf(streaming.codec(AkCaps.CapsAudio)), 0)

                onActivated: streaming.setCodec(AkCaps.CapsAudio,
                                                layout.audioCodecs[index])
            }

            Label {
                text: qsTr("Streaming platform settings")
                font: AkTheme.fontSettings.h6
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.topMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
            }

            AK.LabeledComboBox {
                id: platformCombo
                label: qsTr("Platform")
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
                model: streaming.supportedPlatforms
            }
            Button {
                id: btnAddPlatform
                text: qsTr("Add platform")
                Layout.leftMargin: root.leftMargin
                flat: true

                onClicked: addStreamingPlatformDialog.open()
            }
            Button {
                text: qsTr("Remove platform")
                Layout.leftMargin: root.leftMargin
                flat: true
                enabled: platformCombo.currentIndex >= 0
                         && streaming.supportedPlatforms.length > 0

                onClicked: {
                    streaming.removeSupportedPlatform(layout.currentPlatform)
                    platformCombo.currentIndex = Math.min(platformCombo.currentIndex,
                                                          streaming.supportedPlatforms.length - 1)
                }
            }

            Label {
                text: qsTr("Website")
                font.bold: true
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
            }
            RowLayout {
                layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true

                TextField {
                    id: websiteField
                    placeholderText: "https://www.website.com/"
                    readOnly: true
                    text: streaming.platformWebsite(layout.currentPlatform)
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
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
            }
            RowLayout {
                id: streaminhUrlLayout
                layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true

                property bool isStreaminhUrlVisible: false

                TextField {
                    id: streamingUrlField
                    text: streaming.platformStreamingUrl(layout.currentPlatform)
                    placeholderText: "rtmp://streaming.website.com/${KEY}"
                    readOnly: streaming.platformNeedsKey(layout.currentPlatform)
                    echoMode: streaminhUrlLayout.isStreaminhUrlVisible
                              && (streaming.state != AkElement.ElementStatePlaying
                                  || root.hasActiveScreenSource)?
                                TextInput.Normal:
                                TextInput.Password
                    Layout.fillWidth: true

                    onEditingFinished: streaming.setPlatformStreamingUrl(layout.currentPlatform, text)
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
                             || root.hasActiveScreenSource
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
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
                visible: streaming.platformNeedsKey(layout.currentPlatform)
            }
            RowLayout {
                id: keyLayout
                layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
                visible: streaming.platformNeedsKey(layout.currentPlatform)

                property bool isKeyVisible: false

                TextField {
                    id: keyField
                    text: streaming.platformStreamingKey(layout.currentPlatform)
                    placeholderText: "********************"
                    Layout.fillWidth: true
                    echoMode: keyLayout.isKeyVisible
                              && (streaming.state != AkElement.ElementStatePlaying
                                  || root.hasActiveScreenSource)?
                                TextInput.Normal:
                                TextInput.Password

                    onEditingFinished:
                        streaming.setPlatformStreamingKey(layout.currentPlatform, text)
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
                             || root.hasActiveScreenSource
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
                Layout.leftMargin: root.leftMargin
                flat: true

                onClicked: {
                    let url = streaming.platformDocsUrl(layout.currentPlatform)

                    if (url)
                        Qt.openUrlExternally(url)
                }
            }
            Button {
                text: qsTr("Get streaming key")
                Layout.leftMargin: root.leftMargin
                flat: true
                visible: streaming.platformNeedsKey(layout.currentPlatform)

                onClicked: {
                    let url = streaming.platformKeyConfigsUrl(layout.currentPlatform)

                    if (url)
                        Qt.openUrlExternally(url)
                }
            }
        }

        AddStreamingPlatformDialog {
            id: addStreamingPlatformDialog
            width: root.Window.width
            height: root.Window.height
            anchors.centerIn: Overlay.overlay

            onAcceptedPlatform: function(platform) {
                streaming.addSupportedPlatform(platform.name)
                streaming.setPlatformWebsite(platform.name, platform.website)
                streaming.setPlatformStreamingUrl(platform.name, platform.streamingUrl)
                streaming.setPlatformKeyConfigsUrl(platform.name, platform.keyConfigsUrl)
                streaming.setPlatformDocsUrl(platform.name, platform.docsUrl)
                streaming.setPlatformNeedsKey(platform.name, platform.needsKey)
                platformCombo.currentIndex = streaming.supportedPlatforms.length - 1
            }

            onClosed: btnAddPlatform.forceActiveFocus()
        }
    }
}
