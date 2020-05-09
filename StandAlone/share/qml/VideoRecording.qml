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
            target: Recording

            onAvailableVideoFormatsChanged: {
                cbxVideoFormat.model.clear()

                for (let i in availableVideoFormats) {
                    let fmt = availableVideoFormats[i]

                    cbxVideoFormat.model.append({
                        format: fmt,
                        description: Recording.videoFormatDescription(fmt)
                    })
                }

                cbxVideoFormat.currentIndex =
                        availableVideoFormats.indexOf(Recording.videoFormat)
            }
            onAvailableVideoCodecsChanged: {
                cbxVideoCodec.model.clear()

                for (let i in availableVideoCodecs) {
                    let cdc = availableVideoCodecs[i]

                    cbxVideoCodec.model.append({
                        codec: cdc,
                        description: Recording.codecDescription(cdc)
                    })
                }

                cbxVideoCodec.currentIndex =
                        availableVideoCodecs.indexOf(Recording.videoCodec)
            }
            onAvailableAudioCodecsChanged: {
                cbxAudioCodec.model.clear()

                for (let i in availableAudioCodecs) {
                    let cdc = availableAudioCodecs[i]

                    cbxAudioCodec.model.append({
                        codec: cdc,
                        description: Recording.codecDescription(cdc)
                    })
                }

                cbxAudioCodec.currentIndex =
                        availableAudioCodecs.indexOf(Recording.audioCodec)
            }
            onVideoFormatChanged: {
                cbxVideoFormat.currentIndex =
                        Recording.availableVideoFormats.indexOf(videoFormat)
            }
            onVideoCodecChanged: {
                cbxVideoCodec.currentIndex =
                        Recording.availableVideoCodecs.indexOf(videoCodec)
            }
            onAudioCodecChanged: {
                cbxAudioCodec.currentIndex =
                        Recording.availableAudioCodecs.indexOf(audioCodec)
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
                text: Recording.videoDirectory
                selectByMouse: true

                onTextChanged: Recording.videoDirectory = text
            }
            Button {
                text: qsTr("Search")

                onClicked: {
                    Webcamoid.makedirs(Recording.videoDirectory)
                    folderDialog.open()
                }
            }
            Label {
                text: qsTr("Record audio")
            }
            Switch {
                Layout.columnSpan: 2
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                checked: Recording.recordAudio

                onToggled: Recording.recordAudio = checked
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

                    for (let i in Recording.availableVideoFormats) {
                        let fmt = Recording.availableVideoFormats[i]

                        model.append({
                            format: fmt,
                            description: Recording.videoFormatDescription(fmt)
                        })
                    }

                    currentIndex =
                        Recording.availableVideoFormats.indexOf(Recording.videoFormat)
                }
                onCurrentIndexChanged:
                    Recording.videoFormat =
                        Recording.availableVideoFormats[currentIndex]
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

                    for (let i in Recording.availableVideoCodecs) {
                        let cdc = Recording.availableVideoCodecs[i]

                        model.append({
                            codec: cdc,
                            description: Recording.codecDescription(cdc)
                        })
                    }

                    currentIndex =
                        Recording.availableVideoCodecs.indexOf(Recording.videoCodec)
                }
                onCurrentIndexChanged:
                    Recording.videoCodec =
                        Recording.availableVideoCodecs[currentIndex]
            }
            Button {
                text: qsTr("Configure")
                flat: true

                onClicked: videoRecording.openVideoCodecDialog()
            }
            Label {
                text: qsTr("Audio codec")
                enabled: Recording.recordAudio
            }
            ComboBox {
                id: cbxAudioCodec
                textRole: "description"
                Layout.fillWidth: true
                enabled: Recording.recordAudio
                model: ListModel {
                }

                Component.onCompleted: {
                    model.clear()

                    for (let i in Recording.availableAudioCodecs) {
                        let cdc = Recording.availableAudioCodecs[i]

                        model.append({
                            codec: cdc,
                            description: Recording.codecDescription(cdc)
                        })
                    }

                    currentIndex =
                        Recording.availableAudioCodecs.indexOf(Recording.audioCodec)
                }
                onCurrentIndexChanged:
                    Recording.audioCodec =
                        Recording.availableAudioCodecs[currentIndex]
            }
            Button {
                text: qsTr("Configure")
                enabled: Recording.recordAudio
                flat: true

                onClicked: videoRecording.openAudioCodecDialog()
            }
        }
    }
    LABS.FolderDialog {
        id: folderDialog
        title: qsTr("Select the folder to save your videos")
        folder: "file://" + Recording.videoDirectory

        onAccepted: Recording.videoDirectory =
                    currentFolder.toString().replace("file://", "")
    }
}
