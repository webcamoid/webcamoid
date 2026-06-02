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

AK.MenuOption {
    id: root
    title: qsTr("Streaming")
    subtitle: qsTr("Configure video streaming to many platforms.")
    icon: "image://icons/broadcast"

    property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: layout.height
        clip: true

        ColumnLayout {
            id: layout
            width: scrollView.width

            property var videoEncoderOptions:
                streaming.codecOptions(AkCaps.CapsVideo)
            readonly property string currentPlatform:
                streaming.supportedPlatforms[platformCombo.currentIndex] ?? ""

            Connections {
                target: streaming

                function onCodecOptionsChanged(type, options) {
                    if (type === AkCaps.CapsVideo)
                        layout.videoEncoderOptions = options
                }

                function onCodecChanged(type, codec) {
                    if (type === AkCaps.CapsVideo)
                        layout.videoEncoderOptions = streaming.codecOptions(AkCaps.CapsVideo)
                }
            }

            Label {
                text: qsTr("Video quality")
                font: AkTheme.fontSettings.h6
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.topMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
            }

            GridLayout {
                columns: 2
                layoutDirection: root.rtl ? Qt.RightToLeft : Qt.LeftToRight
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true

                Label {
                    id: txtOutputWidth
                    text: qsTr("Output width")
                }
                SpinBox {
                    id: spbOutputWidth
                    value: AkVideoCaps.create(streaming.videoCaps).width
                    from: 160
                    to: 32768
                    stepSize: 1
                    editable: true
                    Accessible.name: txtOutputWidth.text
                    Layout.rightMargin: root.rightMargin

                    onValueChanged: {
                        let videoCaps = AkVideoCaps.create(streaming.videoCaps)
                        videoCaps.width = value
                        streaming.videoCaps = videoCaps.toVariant()
                    }
                }

                Label {
                    id: txtOutputHeight
                    text: qsTr("Output height")
                }
                SpinBox {
                    id: spbOutputHeight
                    value: AkVideoCaps.create(streaming.videoCaps).height
                    from: 90
                    to: 32768
                    stepSize: 1
                    editable: true
                    Accessible.name: txtOutputHeight.text
                    Layout.rightMargin: root.rightMargin

                    onValueChanged: {
                        let videoCaps = AkVideoCaps.create(streaming.videoCaps)
                        videoCaps.height = value
                        streaming.videoCaps = videoCaps.toVariant()
                    }
                }

                Label {
                    id: txtOutputFrameRate
                    text: qsTr("Output frame rate")
                }
                SpinBox {
                    id: spbOutputFrameRate
                    value: Math.round(AkFrac.create(AkVideoCaps.create(streaming.videoCaps).fps).value)
                    from: 1
                    to: 256
                    stepSize: 1
                    editable: true
                    Accessible.name: txtOutputFrameRate.text
                    Layout.rightMargin: root.rightMargin

                    onValueChanged: {
                        let videoCaps = AkVideoCaps.create(streaming.videoCaps)
                        videoCaps.fps = AkFrac.create(value, 1).toVariant()
                        streaming.videoCaps = videoCaps.toVariant()
                    }
                }

                Label {
                    id: txtVideoBitrate
                    text: qsTr("Video bitrate (kbps)")
                }
                SpinBox {
                    id: spbVideoBitrate
                    value: streaming.bitrate(AkCaps.CapsVideo) / 1000
                    from: 100
                    to: 50000
                    stepSize: 100
                    editable: true
                    Accessible.name: txtVideoBitrate.text
                    Layout.rightMargin: root.rightMargin

                    onValueModified: streaming.setBitrate(AkCaps.CapsVideo, value * 1000)
                }

                Label {
                    id: txtGOP
                    text: qsTr("Keyframes stride (ms)")
                }
                SpinBox {
                    id: spbVideoGOP
                    value: streaming.videoGOP
                    from: 1
                    to: 3600000
                    stepSize: 1
                    editable: true
                    Accessible.name: txtGOP.text

                    onValueModified: streaming.videoGOP = value
                }
            }

            AK.LabeledComboBox {
                id: videoCodecCombo
                label: qsTr("Video codec")
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
                model: streaming.supportedCodecs(layout.currentPlatform, AkCaps.CapsVideo)
                                .map(c => streaming.codecDescription(c))
                currentIndex: {
                    let codecs = streaming.supportedCodecs(layout.currentPlatform, AkCaps.CapsVideo)
                    let idx = codecs.indexOf(streaming.codec(AkCaps.CapsVideo))

                    return idx < 0 ? 0 : idx
                }

                onCurrentIndexChanged: {
                    let codecs = streaming.supportedCodecs(layout.currentPlatform, AkCaps.CapsVideo)

                    if (currentIndex >= 0 && currentIndex < codecs.length)
                        streaming.setCodec(AkCaps.CapsVideo, codecs[currentIndex])
                }
            }

            Repeater {
                readonly property var relevantOptions: [
                    "deadline",
                    "latency",
                    "preset",
                    "quality",
                    "scenario",
                    "speed",
                    "tune",
                    "usage"
                ]

                model: {
                    let opts = []

                    for (let i = 0; i < layout.videoEncoderOptions.length; i++) {
                        let opt = AkPropertyOption.create(layout.videoEncoderOptions[i])

                        if (relevantOptions.indexOf(opt.name) >= 0
                            && opt.menu.length > 0)
                            opts.push({
                                name:        opt.name,
                                description: opt.description,
                                choices:     opt.menu.map(m => AkMenuOption.create(m))
                            })
                    }

                    return opts
                }

                AK.LabeledComboBox {
                   label: modelData.description.length > 0?
                            qsTr(modelData.description):
                            qsTr(modelData.name)
                    Layout.leftMargin: root.leftMargin
                    Layout.rightMargin: root.rightMargin
                    Layout.fillWidth: true
                    model: modelData.choices.map(c => c.description.length > 0?
                                                        c.description:
                                                        c.name)
                    currentIndex: {
                        let cur = streaming.codecOptionValue(AkCaps.CapsVideo,
                                                             modelData.name)

                        for (let i = 0; i < modelData.choices.length; i++)
                            if (modelData.choices[i].value == cur)
                                return i

                        return 0
                    }
                    onCurrentIndexChanged: {
                        if (currentIndex >= 0 && currentIndex < modelData.choices.length)
                            streaming.setCodecOptionValue(AkCaps.CapsVideo,
                                                          modelData.name,
                                                          modelData.choices[currentIndex].value)
                    }
                 }
            }

            Label {
                text: qsTr("Audio quality")
                font: AkTheme.fontSettings.h6
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.topMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
            }

            GridLayout {
                columns: 2
                layoutDirection: root.rtl ? Qt.RightToLeft : Qt.LeftToRight
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true

                Label {
                    id: txtAudioSampleRate
                    text: qsTr("Sample rate")
                }
                SpinBox {
                    id: spbAudioSampleRate
                    value: AkAudioCaps.create(streaming.audioCaps).rate
                    from: 4000
                    to: 512000
                    stepSize: 1000
                    editable: true
                    Accessible.name: txtAudioSampleRate.text
                    Layout.rightMargin: root.rightMargin

                    onValueChanged: {
                        let audioCaps = AkAudioCaps.create(streaming.audioCaps)
                        audioCaps.rate = value
                        streaming.audioCaps = audioCaps.toVariant()
                    }
                }

                Label {
                    id: txtAudioBitrate
                    text: qsTr("Audio bitrate (kbps)")
                }
                SpinBox {
                    id: spbAudioBitrate
                    value: streaming.bitrate(AkCaps.CapsAudio) / 1000
                    from: 8
                    to: 50000
                    stepSize: 8
                    editable: true
                    Accessible.name: txtAudioBitrate.text
                    Layout.rightMargin: root.rightMargin

                    onValueModified: streaming.setBitrate(AkCaps.CapsAudio, value * 1000)
                }
            }

            AK.LabeledComboBox {
                id: audioCodecCombo
                label: qsTr("Audio codec")
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
                model: streaming.supportedCodecs(layout.currentPlatform, AkCaps.CapsAudio)
                                .map(c => streaming.codecDescription(c))
                currentIndex: {
                    let codecs = streaming.supportedCodecs(layout.currentPlatform, AkCaps.CapsAudio)
                    let idx = codecs.indexOf(streaming.codec(AkCaps.CapsAudio))

                    return idx < 0? 0: idx
                }

                onCurrentIndexChanged: {
                    let codecs = streaming.supportedCodecs(layout.currentPlatform, AkCaps.CapsAudio)

                    if (currentIndex >= 0 && currentIndex < codecs.length)
                        streaming.setCodec(AkCaps.CapsAudio, codecs[currentIndex])
                }
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
                layoutDirection: root.rtl ? Qt.RightToLeft : Qt.LeftToRight
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
                layoutDirection: root.rtl ? Qt.RightToLeft : Qt.LeftToRight
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true

                property bool isStreaminhUrlVisible: false

                TextField {
                    id: streamingUrlField
                    text: streaming.platformStreamingUrl(layout.currentPlatform)
                    placeholderText: "rtmp://streaming.website.com/${KEY}"
                    readOnly: streaming.platformNeedsKey(layout.currentPlatform)
                    echoMode: streaminhUrlLayout.isStreaminhUrlVisible?
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
                layoutDirection: root.rtl ? Qt.RightToLeft : Qt.LeftToRight
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
                    echoMode: keyLayout.isKeyVisible?
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
