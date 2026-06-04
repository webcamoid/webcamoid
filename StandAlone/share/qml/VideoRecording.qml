/* Webcamoid, camera capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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
import Qt.labs.platform as LABS
import Qt.labs.settings 1.0
import Ak
import AkControls as AK

AK.MenuOption {
    id: videoRecording
    title: qsTr("Video Recording")
    subtitle: qsTr("Configure video recording format and codecs.")
    icon: "image://icons/video"

    property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft
    readonly property string filePrefix: Ak.platform() == "windows"?
                                             "file:///":
                                             "file://"

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: layout.height
        clip: true

        ColumnLayout {
            id: layout
            width: scrollView.width
            layoutDirection: videoRecording.rtl? Qt.RightToLeft: Qt.LeftToRight

            property bool isPathCustomizable: Ak.platform() != "android"

            readonly property var vc: AkVideoCaps.create(recording.videoCaps)
            property int vbr: recording.bitrate(AkCaps.CapsVideo)
            property int abr: recording.bitrate(AkCaps.CapsAudio)
            readonly property var ac: AkAudioCaps.create(recording.audioCaps)
            readonly property int fps: Math.round(AkFrac.create(vc.fps).value)

            // Flag to prevent re-entrant signal loops when the slider writes
            // back to the recording object, which would trigger the SpinBox
            // onValueChanged handlers and confuse preset detection.
            property bool updatingFromPreset: false

            // Recording quality presets broadly applicable to all common
            // recording formats.
            readonly property var recordingPresets: [
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
            // recording settings, or -1 if none matches (Custom).
            readonly property int matchedPresetIndex: {
                for (let i = 0; i < recordingPresets.length; i++) {
                    let p = recordingPresets[i]

                    if (vc.width === p.width
                        && vc.height === p.height
                        && fps === p.fps
                        && vbr === p.videoBitrate
                        && abr === p.audioBitrate
                        && ac.rate === p.audioRate)
                        return i
                }

                return -1
            }

            Connections {
                target: recording

                function onBitrateChanged(type, bitrate) {
                    if (type === AkCaps.CapsVideo)
                        layout.vbr = bitrate
                    else if (type === AkCaps.CapsAudio)
                        layout.abr = bitrate
                }
            }

            Label {
                id: txtVideosDirectory
                text: qsTr("Videos directory")
                visible: layout.isPathCustomizable
                height: layout.isPathCustomizable? 0: undefined
                font.bold: true
                Layout.leftMargin: videoRecording.leftMargin
                Layout.rightMargin: videoRecording.rightMargin
                Layout.fillWidth: true
            }
            AK.ActionTextField {
                icon.source: "image://icons/search"
                labelText: recording.videoDirectory
                placeholderText: txtVideosDirectory.text
                buttonText: qsTr("Select the save directory")
                visible: layout.isPathCustomizable
                height: layout.isPathCustomizable? 0: undefined
                Layout.leftMargin: videoRecording.leftMargin
                Layout.rightMargin: videoRecording.rightMargin
                Layout.fillWidth: true

                onLabelTextChanged: recording.videoDirectory = labelText
                onButtonClicked: {
                    mediaTools.makedirs(recording.videoDirectory)
                    folderDialog.open()
                }
            }
            Switch {
                text: qsTr("Record audio")
                checked: recording.recordAudio
                Accessible.name: text
                Layout.leftMargin: videoRecording.leftMargin
                Layout.rightMargin: videoRecording.rightMargin
                Layout.fillWidth: true

                onToggled: recording.recordAudio = checked
            }
            Switch {
                id: chkFlash
                text: qsTr("Use flash")
                checked: recording.useVideoFlash
                Accessible.name: text
                Accessible.description: qsTr("Use flash when recording a video")
                Layout.leftMargin: videoRecording.leftMargin
                Layout.rightMargin: videoRecording.rightMargin
                Layout.fillWidth: true

                onCheckedChanged: recording.useVideoFlash = checked
            }

            // Recording quality - preset slider

            Label {
                text: qsTr("Recording quality")
                font: AkTheme.fontSettings.h6
                Layout.leftMargin: videoRecording.leftMargin
                Layout.rightMargin: videoRecording.rightMargin
                Layout.topMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
            }

            // Slider: -1 = Custom, 0..N = preset index.
            // Writing back to the recording object is guarded by
            // updatingFromPreset so the SpinBoxes inside the advanced block
            // don't trigger a re-evaluation loop.
            Slider {
                id: qualitySlider
                from: -1
                to: layout.recordingPresets.length - 1
                stepSize: 1
                // On load: position on the matched preset, or -1 (Custom)
                // if the current/saved settings don't match any preset.
                value: layout.matchedPresetIndex
                snapMode: Slider.SnapAlways
                Layout.leftMargin: videoRecording.leftMargin
                Layout.rightMargin: videoRecording.rightMargin
                Layout.fillWidth: true
                Accessible.name: qsTr("Recording quality")

                onMoved: {
                    // -1 means Custom: the user dragged back to that position,
                    // nothing to apply.
                    if (value < 0)
                        return

                    let p = layout.recordingPresets[value]
                    layout.updatingFromPreset = true

                    let videoCaps = AkVideoCaps.create(recording.videoCaps)
                    videoCaps.width  = p.width
                    videoCaps.height = p.height
                    videoCaps.fps    = AkFrac.create(p.fps, 1).toVariant()
                    recording.videoCaps = videoCaps.toVariant()
                    recording.setBitrate(AkCaps.CapsVideo, p.videoBitrate)
                    recording.videoGOP = 2000

                    let audioCaps = AkAudioCaps.create(recording.audioCaps)
                    audioCaps.rate = p.audioRate
                    recording.audioCaps = audioCaps.toVariant()
                    recording.setBitrate(AkCaps.CapsAudio, p.audioBitrate)

                    layout.updatingFromPreset = false
                }

                // Keep the slider in sync when settings change from outside
                // (e.g. loading a saved profile or editing advanced controls).
                Connections {
                    target: layout
                    function onMatchedPresetIndexChanged() {
                        if (!layout.updatingFromPreset)
                            qualitySlider.value = layout.matchedPresetIndex
                    }
                }
            }

            // Summary line shown below the slider. Displays the preset
            // label when matched, or "Custom" with the raw values otherwise.
            Label {
                id: qualitySummary
                Layout.leftMargin: videoRecording.leftMargin
                Layout.rightMargin: videoRecording.rightMargin
                Layout.bottomMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
                wrapMode: Text.WordWrap

                text: {
                    let header = qualitySlider.value < 0
                        ? "<b>" + qsTr("Custom") + "</b><br/>"
                        : ""

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
                Layout.leftMargin: videoRecording.leftMargin
                Layout.rightMargin: videoRecording.rightMargin
                checked: false
            }

            ColumnLayout {
                visible: chkAdvanced.checked
                Layout.fillWidth: true

                // Video

                Label {
                    text: qsTr("Video quality")
                    font: AkTheme.fontSettings.h6
                    Layout.leftMargin: videoRecording.leftMargin
                    Layout.rightMargin: videoRecording.rightMargin
                    Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                    Layout.bottomMargin: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
                    Layout.fillWidth: true
                }

                GridLayout {
                    columns: 2
                    layoutDirection: videoRecording.rtl? Qt.RightToLeft: Qt.LeftToRight
                    Layout.leftMargin: videoRecording.leftMargin
                    Layout.rightMargin: videoRecording.rightMargin
                    Layout.fillWidth: true

                    Label {
                        id: txtOutputWidth
                        text: qsTr("Output width")
                    }
                    SpinBox {
                        id: spbOutputWidth
                        value: AkVideoCaps.create(recording.videoCaps).width
                        from: 160
                        to: 32768
                        stepSize: 1
                        editable: true
                        Accessible.name: txtOutputWidth.text
                        Layout.rightMargin: videoRecording.rightMargin

                        onValueChanged: {
                            if (layout.updatingFromPreset)
                                return
                            let videoCaps = AkVideoCaps.create(recording.videoCaps)
                            videoCaps.width = value
                            recording.videoCaps = videoCaps.toVariant()
                        }
                    }

                    Label {
                        id: txtOutputHeight
                        text: qsTr("Output height")
                    }
                    SpinBox {
                        id: spbOutputHeight
                        value: AkVideoCaps.create(recording.videoCaps).height
                        from: 90
                        to: 32768
                        stepSize: 1
                        editable: true
                        Accessible.name: txtOutputHeight.text
                        Layout.rightMargin: videoRecording.rightMargin

                        onValueChanged: {
                            if (layout.updatingFromPreset)
                                return
                            let videoCaps = AkVideoCaps.create(recording.videoCaps)
                            videoCaps.height = value
                            recording.videoCaps = videoCaps.toVariant()
                        }
                    }

                    Label {
                        id: txtOutputFrameRate
                        text: qsTr("Output Frame rate")
                    }
                    SpinBox {
                        id: spbOutputFrameRate
                        value: Math.round(AkFrac.create(AkVideoCaps.create(recording.videoCaps).fps).value)
                        from: 1
                        to: 256
                        stepSize: 1
                        editable: true
                        Accessible.name: txtOutputFrameRate.text
                        Layout.rightMargin: videoRecording.rightMargin

                        onValueChanged: {
                            if (layout.updatingFromPreset)
                                return
                            let videoCaps = AkVideoCaps.create(recording.videoCaps)
                            videoCaps.fps = AkFrac.create(value, 1).toVariant()
                            recording.videoCaps = videoCaps.toVariant()
                        }
                    }
                }

                // Audio

                Label {
                    text: qsTr("Audio quality")
                    font: AkTheme.fontSettings.h6
                    Layout.leftMargin: videoRecording.leftMargin
                    Layout.rightMargin: videoRecording.rightMargin
                    Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                    Layout.bottomMargin: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
                    Layout.fillWidth: true
                }

                GridLayout {
                    columns: 2
                    layoutDirection: videoRecording.rtl? Qt.RightToLeft: Qt.LeftToRight
                    Layout.leftMargin: videoRecording.leftMargin
                    Layout.rightMargin: videoRecording.rightMargin
                    Layout.fillWidth: true

                    Label {
                        id: txtAudioSampleRate
                        text: qsTr("Sample rate")
                    }
                    SpinBox {
                        id: spbAudioSampleRate
                        value: AkAudioCaps.create(recording.audioCaps).rate
                        from: 4000
                        to: 512000
                        stepSize: 1
                        editable: true
                        Accessible.name: txtAudioSampleRate.text
                        Layout.rightMargin: videoRecording.rightMargin

                        onValueChanged: {
                            if (layout.updatingFromPreset)
                                return
                            let audioCaps = AkAudioCaps.create(recording.audioCaps)
                            audioCaps.rate = value
                            recording.audioCaps = audioCaps.toVariant()
                        }
                    }
                }

                // Codec/format configuration

                Label {
                    text: qsTr("File format and codecs")
                    font: AkTheme.fontSettings.h6
                    Layout.leftMargin: videoRecording.leftMargin
                    Layout.rightMargin: videoRecording.rightMargin
                    Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                    Layout.bottomMargin: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
                    Layout.fillWidth: true
                }

                Button {
                    id: configureVideoFormat
                    text: qsTr("Configure the file format")
                    flat: true
                    Accessible.description: qsTr("Configure the file format for recording")
                    Layout.leftMargin: videoRecording.leftMargin
                    Layout.rightMargin: videoRecording.rightMargin

                    onClicked: videoFormatOptions.open()
                }
                Button {
                    id: configureVideoCodec
                    text: qsTr("Configure the video codec")
                    flat: true
                    Accessible.description: qsTr("Configure the video codec for recording")
                    Layout.leftMargin: videoRecording.leftMargin
                    Layout.rightMargin: videoRecording.rightMargin

                    onClicked: videoCodecOptions.open()
                }
                Button {
                    id: configureAudioCodec
                    text: qsTr("Configure the audio codec")
                    flat: true
                    enabled: recording.recordAudio
                    Accessible.description: qsTr("Configure the audio codec for recording")
                    Layout.leftMargin: videoRecording.leftMargin
                    Layout.rightMargin: videoRecording.rightMargin

                    onClicked: audioCodecOptions.open()
                }
            } // end advanced ColumnLayout
        }
        VideoFormatOptions {
            id: videoFormatOptions
            width: videoRecording.Window.width
            height: videoRecording.Window.height
            anchors.centerIn: Overlay.overlay

            onClosed: configureVideoFormat.forceActiveFocus()
        }
        VideoCodecOptions {
            id: videoCodecOptions
            width: videoRecording.Window.width
            height: videoRecording.Window.height
            anchors.centerIn: Overlay.overlay

            onClosed: configureVideoCodec.forceActiveFocus()
        }
        AudioCodecOptions {
            id: audioCodecOptions
            width: videoRecording.Window.width
            height: videoRecording.Window.height
            anchors.centerIn: Overlay.overlay

            onClosed: configureAudioCodec.forceActiveFocus()
        }
        LABS.FolderDialog {
            id: folderDialog
            title: qsTr("Select the folder to save your videos")
            folder: videoRecording.filePrefix + recording.videoDirectory

            onAccepted: {
                recording.videoDirectory = mediaTools.urlToLocalFolder(currentFolder)
            }
        }
        Settings {
            category: "GeneralConfigs"

            property alias useVideoFlash: chkFlash.checked
        }
        Settings {
            category: "RecordConfigs"

            property alias outputWidth: spbOutputWidth.value
            property alias outputHeight: spbOutputHeight.value
            property alias outputFPS: spbOutputFrameRate.value
            property alias audioSampleRate: spbAudioSampleRate.value
        }
    }
}
