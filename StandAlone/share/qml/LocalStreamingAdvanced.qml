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
import Ak
import AkControls as AK
import Webcamoid

Dialog {
    id: root
    title: qsTr("Local streaming advanced options")
    standardButtons: Dialog.Close
    width: physicalWidth <= 100 || physicalHeight <= 100?
               wdgMainWidget.width: wdgMainWidget.width * 0.5
    height: physicalWidth <= 100 || physicalHeight <= 100?
                wdgMainWidget.height: wdgMainWidget.height * 0.5
    modal: true

    property real physicalWidth: wdgMainWidget.width / Screen.pixelDensity
    property real physicalHeight: wdgMainWidget.height / Screen.pixelDensity

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: dialogLayout.height
        clip: true

        property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

        ColumnLayout {
            id: dialogLayout
            width: scrollView.width

            property var videoEncoderOptions:
                localStreaming.codecOptions(AkCaps.CapsVideo)
            readonly property string currentFormat:
                localStreaming.locationFormat(localStreaming.location)

            readonly property var vc: AkVideoCaps.create(localStreaming.videoCaps)
            property int vbr: localStreaming.bitrate(AkCaps.CapsVideo)
            property int abr: localStreaming.bitrate(AkCaps.CapsAudio)
            readonly property var ac: AkAudioCaps.create(localStreaming.audioCaps)
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
                target: localStreaming

                function onCodecOptionsChanged(type, options) {
                    if (type === AkCaps.CapsVideo)
                        dialogLayout.videoEncoderOptions = options
                }

                function onCodecChanged(type, codec) {
                    if (type === AkCaps.CapsVideo)
                        dialogLayout.videoEncoderOptions = localStreaming.codecOptions(AkCaps.CapsVideo)
                }

                function onBitrateChanged(type, bitrate) {
                    if (type === AkCaps.CapsVideo)
                        dialogLayout.vbr = bitrate
                    else if (type === AkCaps.CapsAudio)
                        dialogLayout.abr = bitrate
                }
            }

            // Streaming quality - preset slider

            Label {
                text: qsTr("Local streaming quality")
                font: AkTheme.fontSettings.h6
                Layout.leftMargin: scrollView.leftMargin
                Layout.rightMargin: scrollView.rightMargin
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
                to: dialogLayout.streamingPresets.length - 1
                stepSize: 1
                // On load: position on the matched preset, or -1 (Custom)
                // if the current/saved settings don't match any preset.
                value: dialogLayout.matchedPresetIndex
                snapMode: Slider.SnapAlways
                Layout.leftMargin: scrollView.leftMargin
                Layout.rightMargin: scrollView.rightMargin
                Layout.fillWidth: true
                Accessible.name: qsTr("Streaming quality")

                onMoved: {
                    // -1 means Custom: the user dragged back to that position,
                    // nothing to apply.
                    if (value < 0)
                        return

                    let p = dialogLayout.streamingPresets[value]
                    dialogLayout.updatingFromPreset = true

                    let videoCaps  = AkVideoCaps.create(localStreaming.videoCaps)
                    videoCaps.width  = p.width
                    videoCaps.height = p.height
                    videoCaps.fps    = AkFrac.create(p.fps, 1).toVariant()

                    localStreaming.videoCaps = videoCaps.toVariant()

                    localStreaming.setBitrate(AkCaps.CapsVideo, p.videoBitrate)
                    localStreaming.videoGOP = 2000

                    let audioCaps  = AkAudioCaps.create(localStreaming.audioCaps)
                    audioCaps.rate = p.audioRate
                    localStreaming.audioCaps = audioCaps.toVariant()
                    localStreaming.setBitrate(AkCaps.CapsAudio, p.audioBitrate)

                    dialogLayout.updatingFromPreset = false
                }

                // Keep the slider in sync when settings change from outside
                // (e.g. loading a saved profile).  Sync in both directions:
                // to a preset index when matched, or back to -1 (Custom) when
                // the advanced controls produce a non-preset combination.
                Connections {
                    target: dialogLayout
                    function onMatchedPresetIndexChanged() {
                        if (!dialogLayout.updatingFromPreset)
                            qualitySlider.value = dialogLayout.matchedPresetIndex
                    }
                }
            }

            // Summary line shown below the slider.
            Label {
                id: qualitySummary
                Layout.leftMargin: scrollView.leftMargin
                Layout.rightMargin: scrollView.rightMargin
                Layout.bottomMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
                wrapMode: Text.WordWrap

                text: {
                    let header = qualitySlider.value < 0?
                                    "<b>" + qsTr("Custom") + "</b><br/>": ""

                    return header
                        + qsTr("Video quality: %1×%2 %3 FPS").arg(dialogLayout.vc.width).arg(dialogLayout.vc.height).arg(dialogLayout.fps) + "<br/>"
                        + qsTr("Video bitrate: %1 Mbps").arg((dialogLayout.vbr / 1000000).toFixed(1)) + "<br/>"
                        + qsTr("Audio sample rate: %1 kHz").arg(dialogLayout.ac.rate / 1000) + "<br/>"
                        + qsTr("Audio bitrate: %1 kbps").arg(dialogLayout.abr / 1000)
                }
            }

            // Advanced

            // Video

            Label {
                text: qsTr("Video quality")
                font: AkTheme.fontSettings.h6
                Layout.leftMargin: scrollView.leftMargin
                Layout.rightMargin: scrollView.rightMargin
                Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
            }

            GridLayout {
                columns: 2
                layoutDirection: scrollView.rtl? Qt.RightToLeft: Qt.LeftToRight
                Layout.leftMargin: scrollView.leftMargin
                Layout.rightMargin: scrollView.rightMargin
                Layout.fillWidth: true

                Label {
                    id: txtOutputWidth
                    text: qsTr("Output width")
                }
                SpinBox {
                    id: spbOutputWidth
                    value: AkVideoCaps.create(localStreaming.videoCaps).width
                    from: 160
                    to: 32768
                    stepSize: 1
                    editable: true
                    Accessible.name: txtOutputWidth.text
                    Layout.rightMargin: scrollView.rightMargin

                    onValueChanged: {
                        if (dialogLayout.updatingFromPreset)
                            return
                        let videoCaps = AkVideoCaps.create(localStreaming.videoCaps)
                        videoCaps.width = value
                        localStreaming.videoCaps = videoCaps.toVariant()
                    }
                }

                Label {
                    id: txtOutputHeight
                    text: qsTr("Output height")
                }
                SpinBox {
                    id: spbOutputHeight
                    value: AkVideoCaps.create(localStreaming.videoCaps).height
                    from: 90
                    to: 32768
                    stepSize: 1
                    editable: true
                    Accessible.name: txtOutputHeight.text
                    Layout.rightMargin: scrollView.rightMargin

                    onValueChanged: {
                        if (dialogLayout.updatingFromPreset)
                            return
                        let videoCaps = AkVideoCaps.create(localStreaming.videoCaps)
                        videoCaps.height = value
                        localStreaming.videoCaps = videoCaps.toVariant()
                    }
                }

                Label {
                    id: txtOutputFrameRate
                    text: qsTr("Output frame rate")
                }
                SpinBox {
                    id: spbOutputFrameRate
                    value: Math.round(AkFrac.create(AkVideoCaps.create(localStreaming.videoCaps).fps).value)
                    from: 1
                    to: 256
                    stepSize: 1
                    editable: true
                    Accessible.name: txtOutputFrameRate.text
                    Layout.rightMargin: scrollView.rightMargin

                    onValueChanged: {
                        if (dialogLayout.updatingFromPreset)
                            return
                        let videoCaps = AkVideoCaps.create(localStreaming.videoCaps)
                        videoCaps.fps = AkFrac.create(value, 1).toVariant()
                        localStreaming.videoCaps = videoCaps.toVariant()
                    }
                }

                Label {
                    id: txtVideoBitrate
                    text: qsTr("Video bitrate (kbps)")
                }
                SpinBox {
                    id: spbVideoBitrate
                    value: localStreaming.bitrate(AkCaps.CapsVideo) / 1000
                    from: 100
                    to: 50000
                    stepSize: 100
                    editable: true
                    Accessible.name: txtVideoBitrate.text
                    Layout.rightMargin: scrollView.rightMargin

                    onValueModified: {
                        if (!dialogLayout.updatingFromPreset)
                            localStreaming.setBitrate(AkCaps.CapsVideo, value * 1000)
                    }
                }

                Label {
                    id: txtGOP
                    text: qsTr("Keyframes stride (ms)")
                }
                SpinBox {
                    id: spbVideoGOP
                    value: localStreaming.videoGOP
                    from: 1
                    to: 3600000
                    stepSize: 1
                    editable: true
                    Accessible.name: txtGOP.text

                    onValueModified: localStreaming.videoGOP = value
                }
            }

            // Video codec

            AK.LabeledComboBox {
                id: videoCodecCombo
                label: qsTr("Video codec")
                Layout.leftMargin: scrollView.leftMargin
                Layout.rightMargin: scrollView.rightMargin
                Layout.fillWidth: true
                model: localStreaming.supportedCodecs(dialogLayout.currentFormat, AkCaps.CapsVideo)
                                .map(c => localStreaming.codecDescription(c))
                currentIndex: {
                    let codecs = localStreaming.supportedCodecs(dialogLayout.currentFormat, AkCaps.CapsVideo)
                    let idx = codecs.indexOf(localStreaming.codec(AkCaps.CapsVideo))

                    return idx < 0? 0: idx
                }

                onCurrentIndexChanged: {
                    let codecs = localStreaming.supportedCodecs(dialogLayout.currentFormat, AkCaps.CapsVideo)

                    if (currentIndex >= 0 && currentIndex < codecs.length)
                        localStreaming.setCodec(AkCaps.CapsVideo, codecs[currentIndex])
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

                    for (let i = 0; i < dialogLayout.videoEncoderOptions.length; i++) {
                        let opt = AkPropertyOption.create(dialogLayout.videoEncoderOptions[i])

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
                    Layout.leftMargin: scrollView.leftMargin
                    Layout.rightMargin: scrollView.rightMargin
                    Layout.fillWidth: true
                    model: modelData.choices.map(c => c.description.length > 0?
                                                      c.description:
                                                      c.name)
                    currentIndex: {
                        let cur = localStreaming.codecOptionValue(AkCaps.CapsVideo,
                                                                  modelData.name)

                        for (let i = 0; i < modelData.choices.length; i++)
                            if (modelData.choices[i].value == cur)
                                return i

                        return 0
                    }
                    onCurrentIndexChanged: {
                        if (currentIndex >= 0 && currentIndex < modelData.choices.length)
                            localStreaming.setCodecOptionValue(AkCaps.CapsVideo,
                                                               modelData.name,
                                                               modelData.choices[currentIndex].value)
                    }
                }
            }

            // Audio

            Label {
                text: qsTr("Audio quality")
                font: AkTheme.fontSettings.h6
                Layout.leftMargin: scrollView.leftMargin
                Layout.rightMargin: scrollView.rightMargin
                Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
            }

            GridLayout {
                columns: 2
                layoutDirection: scrollView.rtl? Qt.RightToLeft: Qt.LeftToRight
                Layout.leftMargin: scrollView.leftMargin
                Layout.rightMargin: scrollView.rightMargin
                Layout.fillWidth: true

                Label {
                    id: txtAudioSampleRate
                    text: qsTr("Sample rate")
                }
                SpinBox {
                    id: spbAudioSampleRate
                    value: AkAudioCaps.create(localStreaming.audioCaps).rate
                    from: 4000
                    to: 512000
                    stepSize: 1000
                    editable: true
                    Accessible.name: txtAudioSampleRate.text
                    Layout.rightMargin: scrollView.rightMargin

                    onValueChanged: {
                        if (dialogLayout.updatingFromPreset)
                            return
                        let audioCaps = AkAudioCaps.create(localStreaming.audioCaps)
                        audioCaps.rate = value
                        localStreaming.audioCaps = audioCaps.toVariant()
                    }
                }

                Label {
                    id: txtAudioBitrate
                    text: qsTr("Audio bitrate (kbps)")
                }
                SpinBox {
                    id: spbAudioBitrate
                    value: localStreaming.bitrate(AkCaps.CapsAudio) / 1000
                    from: 8
                    to: 50000
                    stepSize: 8
                    editable: true
                    Accessible.name: txtAudioBitrate.text
                    Layout.rightMargin: scrollView.rightMargin

                    onValueModified: {
                        if (!dialogLayout.updatingFromPreset)
                            localStreaming.setBitrate(AkCaps.CapsAudio, value * 1000)
                    }
                }
            }

            AK.LabeledComboBox {
                id: audioCodecCombo
                label: qsTr("Audio codec")
                Layout.leftMargin: scrollView.leftMargin
                Layout.rightMargin: scrollView.rightMargin
                Layout.fillWidth: true
                model: localStreaming.supportedCodecs(dialogLayout.currentFormat, AkCaps.CapsAudio)
                                .map(c => localStreaming.codecDescription(c))
                currentIndex: {
                    let codecs = localStreaming.supportedCodecs(dialogLayout.currentFormat, AkCaps.CapsAudio)
                    let idx = codecs.indexOf(localStreaming.codec(AkCaps.CapsAudio))

                    return idx < 0? 0: idx
                }

                onCurrentIndexChanged: {
                    let codecs = localStreaming.supportedCodecs(dialogLayout.currentFormat, AkCaps.CapsAudio)

                    if (currentIndex >= 0 && currentIndex < codecs.length)
                        localStreaming.setCodec(AkCaps.CapsAudio, codecs[currentIndex])
                }
            }
        }
    }
}
