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

Page {
    id: videoRecording

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: layout.height
        clip: true

        readonly property string filePrefix: Ak.platform() == "windows"?
                                                 "file:///":
                                                 "file://"

        GridLayout {
            id: layout
            width: scrollView.width
            columns: 3

            property bool isPathCustomizable: Ak.platform() != "android"

            Label {
                id: txtVideosDirectory
                text: qsTr("Videos directory")
                visible: layout.isPathCustomizable
                height: layout.isPathCustomizable? 0: undefined
            }
            TextField {
                text: recording.videoDirectory
                Accessible.name: txtVideosDirectory.text
                selectByMouse: true
                Layout.fillWidth: true
                visible: layout.isPathCustomizable
                height: layout.isPathCustomizable? 0: undefined

                onTextChanged: recording.videoDirectory = text
            }
            Button {
                text: qsTr("Search")
                Accessible.description: qsTr("Search directory to save videos")
                visible: layout.isPathCustomizable
                height: layout.isPathCustomizable? 0: undefined

                onClicked: {
                    mediaTools.makedirs(recording.videoDirectory)
                    folderDialog.open()
                }
            }
            Label {
                id: txtRecordAudio
                text: qsTr("Record audio")
            }
            Switch {
                Accessible.name: txtRecordAudio.text
                Layout.columnSpan: 2
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                checked: recording.recordAudio

                onToggled: recording.recordAudio = checked
            }
            Label {
                text: qsTr("Video quality")
                font: AkTheme.fontSettings.h6
                Layout.topMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.columnSpan: 3
            }
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
                Layout.columnSpan: 2

                onValueChanged: {
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
                Layout.columnSpan: 2

                onValueChanged: {
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
                to: 1024
                stepSize: 1
                editable: true
                Accessible.name: txtOutputFrameRate.text
                Layout.columnSpan: 2

                onValueChanged: {
                    let videoCaps = AkVideoCaps.create(recording.videoCaps)
                    videoCaps.fps = AkFrac.create(value, 1).toVariant()
                    recording.videoCaps = videoCaps.toVariant()
                }
            }
            Label {
                text: qsTr("Audio quality")
                font: AkTheme.fontSettings.h6
                Layout.topMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.columnSpan: 3
            }
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
                Layout.columnSpan: 2

                onValueChanged: {
                    let audioCaps = AkAudioCaps.create(recording.audioCaps)
                    audioCaps.rate = value
                    recording.audioCaps = audioCaps.toVariant()
                }
            }
            Label {
                text: qsTr("File format and codecs")
                font: AkTheme.fontSettings.h6
                Layout.topMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.columnSpan: 3
            }
            Button {
                id: configureVideoFormat
                text: qsTr("Configure the file format")
                flat: true
                Accessible.description: qsTr("Configure the file format for recording")
                Layout.columnSpan: 3

                onClicked: videoFormatOptions.open()
            }
            Button {
                id: configureVideoCodec
                text: qsTr("Configure the video codec")
                Accessible.description: qsTr("Configure the video codec for recording")
                Layout.columnSpan: 3
                flat: true

                onClicked: videoCodecOptions.open()
            }
            Button {
                id: configureAudioCodec
                text: qsTr("Configure the audio codec")
                flat: true
                Accessible.description: qsTr("Configure the audio codec for recording")
                Layout.columnSpan: 3
                enabled: recording.recordAudio

                onClicked: audioCodecOptions.open()
            }
        }
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
        folder: scrollView.filePrefix + recording.videoDirectory

        onAccepted: {
            recording.videoDirectory = mediaTools.urlToLocalFile(currentFolder)
        }
    }
    Settings {
        category: "RecordConfigs"

        property alias outputWidth: spbOutputWidth.value
        property alias outputHeight: spbOutputHeight.value
        property alias outputFPS: spbOutputFrameRate.value
        property alias audioSampleRate: spbAudioSampleRate.value
    }
}
