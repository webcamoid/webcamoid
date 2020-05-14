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
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import Qt.labs.platform 1.1 as LABS
import Ak 1.0

Page {
    id: videoRecording

    signal openVideoFormatDialog()
    signal openVideoCodecDialog()
    signal openAudioCodecDialog()

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: layout.height
        clip: true

        Connections {
            target: recording

            onAvailableVideoFormatsChanged: {
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
            onAvailableVideoCodecsChanged: {
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
            onAvailableAudioCodecsChanged: {
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
            onVideoFormatChanged: {
                cbxVideoFormat.currentIndex =
                        recording.availableVideoFormats.indexOf(videoFormat)
            }
            onVideoCodecChanged: {
                cbxVideoCodec.currentIndex =
                        recording.availableVideoCodecs.indexOf(videoCodec)
            }
            onAudioCodecChanged: {
                cbxAudioCodec.currentIndex =
                        recording.availableAudioCodecs.indexOf(audioCodec)
            }
        }

        GridLayout {
            id: layout
            width: scrollView.width
            columns: 3

            Label {
                text: qsTr("Videos directory")
            }
            TextField {
                Layout.fillWidth: true
                text: recording.videoDirectory
                selectByMouse: true

                onTextChanged: recording.videoDirectory = text
            }
            Button {
                text: qsTr("Search")

                onClicked: {
                    mediaTools.makedirs(recording.videoDirectory)
                    folderDialog.open()
                }
            }
            Label {
                text: qsTr("Record audio")
            }
            Switch {
                Layout.columnSpan: 2
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                checked: recording.recordAudio

                onToggled: recording.recordAudio = checked
            }
            Label {
                text: qsTr("File format")
            }
            ComboBox {
                id: cbxVideoFormat
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
                text: qsTr("Configure")
                flat: true

                onClicked: videoRecording.openVideoFormatDialog()
            }
            Label {
                text: qsTr("Video codec")
            }
            ComboBox {
                id: cbxVideoCodec
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
                text: qsTr("Configure")
                flat: true

                onClicked: videoRecording.openVideoCodecDialog()
            }
            Label {
                text: qsTr("Audio codec")
                enabled: recording.recordAudio
            }
            ComboBox {
                id: cbxAudioCodec
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
                text: qsTr("Configure")
                enabled: recording.recordAudio
                flat: true

                onClicked: videoRecording.openAudioCodecDialog()
            }
        }
    }
    LABS.FolderDialog {
        id: folderDialog
        title: qsTr("Select the folder to save your videos")
        folder: "file://" + recording.videoDirectory

        onAccepted: recording.videoDirectory =
                    currentFolder.toString().replace("file://", "")
    }
}
