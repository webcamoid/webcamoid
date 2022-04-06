/* Webcamoid, webcam capture application.
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

import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import Qt.labs.platform 1.1 as LABS
import Ak 1.0

Page {
    id: videoRecording

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: layout.height
        clip: true

        Connections {
            target: recording

            function onAvailableVideoFormatsChanged(availableVideoFormats)
            {
                cbxVideoFormat.model.clear()

                for (let i in availableVideoFormats) {
                    let fmt = availableVideoFormats[i]

                    cbxVideoFormat.model.append({
                        format: fmt,
                        description: recording.videoFormatDescription(fmt)
                    })
                }

                cbxVideoFormat.currentIndex =
                        availableVideoFormats.indexOf(recording.videoFormat)
            }

            function onAvailableVideoCodecsChanged(availableVideoCodecs)
            {
                cbxVideoCodec.model.clear()

                for (let i in availableVideoCodecs) {
                    let cdc = availableVideoCodecs[i]

                    cbxVideoCodec.model.append({
                        codec: cdc,
                        description: recording.codecDescription(cdc)
                    })
                }

                cbxVideoCodec.currentIndex =
                        availableVideoCodecs.indexOf(recording.videoCodec)
            }

            function onAvailableAudioCodecsChanged(availableAudioCodecs)
            {
                cbxAudioCodec.model.clear()

                for (let i in availableAudioCodecs) {
                    let cdc = availableAudioCodecs[i]

                    cbxAudioCodec.model.append({
                        codec: cdc,
                        description: recording.codecDescription(cdc)
                    })
                }

                cbxAudioCodec.currentIndex =
                        availableAudioCodecs.indexOf(recording.audioCodec)
            }

            function onVideoFormatChanged(videoFormat)
            {
                cbxVideoFormat.currentIndex =
                        recording.availableVideoFormats.indexOf(videoFormat)
            }

            function onVideoCodecChanged(videoCodec)
            {
                cbxVideoCodec.currentIndex =
                        recording.availableVideoCodecs.indexOf(videoCodec)
            }

            function onAudioCodecChanged(audioCodec)
            {
                cbxAudioCodec.currentIndex =
                        recording.availableAudioCodecs.indexOf(audioCodec)
            }
        }

        GridLayout {
            id: layout
            width: scrollView.width
            columns: 3

            Label {
                id: txtVideosDirectory
                text: qsTr("Videos directory")
            }
            TextField {
                text: recording.videoDirectory
                Accessible.name: txtVideosDirectory.text
                selectByMouse: true
                Layout.fillWidth: true

                onTextChanged: recording.videoDirectory = text
            }
            Button {
                text: qsTr("Search")
                Accessible.description: qsTr("Search directory to save videos")

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
                id: txtFileFormat
                text: qsTr("File format")
            }
            ComboBox {
                id: cbxVideoFormat
                Accessible.description: txtFileFormat.text
                textRole: "description"
                Layout.fillWidth: true
                model: ListModel {
                }

                Component.onCompleted: {
                    model.clear()

                    for (let i in recording.availableVideoFormats) {
                        let fmt = recording.availableVideoFormats[i]

                        model.append({
                            format: fmt,
                            description: recording.videoFormatDescription(fmt)
                        })
                    }

                    currentIndex =
                        recording.availableVideoFormats.indexOf(recording.videoFormat)
                }
                onCurrentIndexChanged:
                    recording.videoFormat =
                        recording.availableVideoFormats[currentIndex]
            }
            Button {
                id: configureVideoFormat
                text: qsTr("Configure")
                Accessible.description: qsTr("Configure file format")
                flat: true

                onClicked: videoFormatOptions.open()
            }
            Label {
                id: txtVideoCodec
                text: qsTr("Video codec")
            }
            ComboBox {
                id: cbxVideoCodec
                Accessible.description: txtVideoCodec.text
                textRole: "description"
                Layout.fillWidth: true
                model: ListModel {
                }

                Component.onCompleted: {
                    model.clear()

                    for (let i in recording.availableVideoCodecs) {
                        let cdc = recording.availableVideoCodecs[i]

                        model.append({
                            codec: cdc,
                            description: recording.codecDescription(cdc)
                        })
                    }

                    currentIndex =
                        recording.availableVideoCodecs.indexOf(recording.videoCodec)
                }
                onCurrentIndexChanged:
                    recording.videoCodec =
                        recording.availableVideoCodecs[currentIndex]
            }
            Button {
                id: configureVideoCodec
                text: qsTr("Configure")
                Accessible.description: qsTr("Configure video codec")
                flat: true

                onClicked: videoCodecOptions.open()
            }
            Label {
                id: txtAudioCodec
                text: qsTr("Audio codec")
                enabled: recording.recordAudio
            }
            ComboBox {
                id: cbxAudioCodec
                Accessible.description: txtAudioCodec.text
                textRole: "description"
                Layout.fillWidth: true
                enabled: recording.recordAudio
                model: ListModel {
                }

                Component.onCompleted: {
                    model.clear()

                    for (let i in recording.availableAudioCodecs) {
                        let cdc = recording.availableAudioCodecs[i]

                        model.append({
                            codec: cdc,
                            description: recording.codecDescription(cdc)
                        })
                    }

                    currentIndex =
                        recording.availableAudioCodecs.indexOf(recording.audioCodec)
                }
                onCurrentIndexChanged:
                    recording.audioCodec =
                        recording.availableAudioCodecs[currentIndex]
            }
            Button {
                id: configureAudioCodec
                text: qsTr("Configure")
                Accessible.description: qsTr("Configure audio codec")
                enabled: recording.recordAudio
                flat: true

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
        folder: "file://" + recording.videoDirectory

        onAccepted: recording.videoDirectory =
                    currentFolder.toString().replace("file://", "")
    }
}
