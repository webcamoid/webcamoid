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

            readonly property var vc: AkVideoCaps.create(streaming.videoCaps)
            property int vbr: streaming.bitrate(AkCaps.CapsVideo)
            property int abr: streaming.bitrate(AkCaps.CapsAudio)
            readonly property var ac: AkAudioCaps.create(streaming.audioCaps)
            readonly property int fps: Math.round(AkFrac.create(vc.fps).value)

            // Flag to prevent re-entrant signal loops when the slider writes
            // back to the streaming object, which would trigger the SpinBox
            // onValueChanged handlers and confuse preset detection.
            property bool updatingFromPreset: false

            // Streaming quality presets broadly applicable to all major
            // platforms.
            readonly property var streamingPresets: [
                { width: 480,  height: 270,  fps: 30, videoBitrate: 1000000,  audioBitrate: 64000,  audioRate: 44100 },
                { width: 640,  height: 360,  fps: 30, videoBitrate: 1500000,  audioBitrate: 128000, audioRate: 44100 },
                { width: 854,  height: 480,  fps: 30, videoBitrate: 2500000,  audioBitrate: 128000, audioRate: 44100 },
                { width: 1280, height: 720,  fps: 30, videoBitrate: 4000000,  audioBitrate: 128000, audioRate: 44100 },
                { width: 1280, height: 720,  fps: 60, videoBitrate: 6000000,  audioBitrate: 128000, audioRate: 48000 },
                { width: 1920, height: 1080, fps: 30, videoBitrate: 10000000, audioBitrate: 128000, audioRate: 48000 },
                { width: 1920, height: 1080, fps: 60, videoBitrate: 12000000, audioBitrate: 128000, audioRate: 48000 },
                { width: 2560, height: 1440, fps: 30, videoBitrate: 15000000, audioBitrate: 128000, audioRate: 48000 },
                { width: 2560, height: 1440, fps: 60, videoBitrate: 24000000, audioBitrate: 128000, audioRate: 48000 },
                { width: 3840, height: 2160, fps: 30, videoBitrate: 40000000, audioBitrate: 128000, audioRate: 48000 },
                { width: 3840, height: 2160, fps: 60, videoBitrate: 61000000, audioBitrate: 128000, audioRate: 48000 },
            ]

            // Returns the index of the preset that matches the current
            // streaming settings, or -1 if none matches (Custom).
            readonly property int matchedPresetIndex: {
                for (let i = 0; i < streamingPresets.length; i++) {
                    let p = streamingPresets[i]

                    if (vc.width === p.width
                        && vc.height === p.height
                        && fps === p.fps
                        && vbr === p.videoBitrate
                        && abr === p.audioBitrate
                        && ac.rate === p.audioRate) {
                        return i
                    }
                }

                return -1
            }

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

                function onBitrateChanged(type, bitrate) {
                    if (type === AkCaps.CapsVideo)
                        layout.vbr = bitrate
                    else if (type === AkCaps.CapsAudio)
                        layout.abr = bitrate
                }
            }

            // Streaming quality - preset slider

            Label {
                text: qsTr("Streaming quality")
                font: AkTheme.fontSettings.h6
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.topMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
            }

            // Slider: -1 = Custom, 0..N = preset index.
            // Writing back to the streaming object is guarded by
            // updatingFromPreset so the SpinBoxes inside the advanced block
            // don't trigger a re-evaluation loop.
            Slider {
                id: qualitySlider
                from: -1
                to: layout.streamingPresets.length - 1
                stepSize: 1
                // On load: position on the matched preset, or -1 (Custom)
                // if the current/saved settings don't match any preset.
                value: layout.matchedPresetIndex
                snapMode: Slider.SnapAlways
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
                Accessible.name: qsTr("Streaming quality")

                onMoved: {
                    // -1 means Custom: the user dragged back to that position,
                    // nothing to apply.
                    if (value < 0)
                        return

                    let p = layout.streamingPresets[value]
                    layout.updatingFromPreset = true

                    let videoCaps  = AkVideoCaps.create(streaming.videoCaps)
                    videoCaps.width  = p.width
                    videoCaps.height = p.height
                    videoCaps.fps    = AkFrac.create(p.fps, 1).toVariant()

                    streaming.videoCaps = videoCaps.toVariant()

                    streaming.setBitrate(AkCaps.CapsVideo, p.videoBitrate)
                    streaming.videoGOP = 2000

                    let audioCaps  = AkAudioCaps.create(streaming.audioCaps)
                    audioCaps.rate = p.audioRate
                    streaming.audioCaps = audioCaps.toVariant()
                    streaming.setBitrate(AkCaps.CapsAudio, p.audioBitrate)

                    layout.updatingFromPreset = false
                }

                // Keep the slider in sync when settings change from outside
                // (e.g. loading a saved profile).  Sync in both directions:
                // to a preset index when matched, or back to -1 (Custom) when
                // the advanced controls produce a non-preset combination.
                Connections {
                    target: layout
                    function onMatchedPresetIndexChanged() {
                        if (!layout.updatingFromPreset)
                            qualitySlider.value = layout.matchedPresetIndex
                    }
                }
            }

            // Summary line shown below the slider.
            Label {
                id: qualitySummary
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.bottomMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
                wrapMode: Text.WordWrap

                text: {
                    let header = qualitySlider.value < 0?
                                    "<b>" + qsTr("Custom") + "</b><br/>": ""

                    return header
                        + qsTr("Video quality: %1×%2 %3 FPS").arg(layout.vc.width).arg(layout.vc.height).arg(layout.fps) + "<br/>"
                        + qsTr("Video bitrate: %1 Mbps").arg((layout.vbr / 1000000).toFixed(1)) + "<br/>"
                        + qsTr("Audio sample rate: %1 kHz").arg(layout.ac.rate / 1000) + "<br/>"
                        + qsTr("Audio bitrate: %1 kbps").arg(layout.abr / 1000)
                }
            }

            // Advanced settings (collapsible)

            CheckBox {
                id: chkAdvanced
                text: qsTr("Advanced settings")
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                checked: false
            }

            // The ColumnLayout wrapper is always in the tree so that
            // bindings inside remain active; only visibility is toggled.
            ColumnLayout {
                visible: chkAdvanced.checked
                Layout.fillWidth: true

                // Video

                Label {
                    text: qsTr("Video quality")
                    font: AkTheme.fontSettings.h6
                    Layout.leftMargin: root.leftMargin
                    Layout.rightMargin: root.rightMargin
                    Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                    Layout.bottomMargin: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
                    Layout.fillWidth: true
                }

                GridLayout {
                    columns: 2
                    layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight
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
                            if (layout.updatingFromPreset)
                                return
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
                            if (layout.updatingFromPreset)
                                return
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
                            if (layout.updatingFromPreset)
                                return
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

                        onValueModified: {
                            if (!layout.updatingFromPreset)
                                streaming.setBitrate(AkCaps.CapsVideo, value * 1000)
                        }
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

                // Audio

                Label {
                    text: qsTr("Audio quality")
                    font: AkTheme.fontSettings.h6
                    Layout.leftMargin: root.leftMargin
                    Layout.rightMargin: root.rightMargin
                    Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                    Layout.bottomMargin: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
                    Layout.fillWidth: true
                }

                GridLayout {
                    columns: 2
                    layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight
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
                            if (layout.updatingFromPreset)
                                return
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

                        onValueModified: {
                            if (!layout.updatingFromPreset)
                                streaming.setBitrate(AkCaps.CapsAudio, value * 1000)
                        }
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
            } // end advanced ColumnLayout

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
                                  || videoLayer.deviceType(videoLayer.videoInput) != VideoLayer.InputScreen)?
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
                                  || videoLayer.deviceType(videoLayer.videoInput) != VideoLayer.InputScreen)?
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
