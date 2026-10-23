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
    subtitle: qsTr("Configure the video recording format and codecs.")
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

            // Changes each time the formats or the codecs change, to force
            // the update of the bindings that call functions.
            property int revision: 0
            readonly property var formats: {
                revision

                return recording.videoFormats
            }
            readonly property string currentFormat: {
                revision

                return recording.videoFormat
            }
            readonly property var videoCodecs: {
                revision

                return recording.supportedCodecs(recording.videoFormat,
                                                 AkCaps.CapsVideo)
            }
            readonly property var audioCodecs: {
                revision

                return recording.supportedCodecs(recording.videoFormat,
                                                 AkCaps.CapsAudio)
            }

            Connections {
                target: recording

                function onSupportedCodecsChanged() {
                    layout.revision++
                }

                function onVideoFormatChanged(format) {
                    layout.revision++
                }

                function onCodecChanged(type, codec) {
                    layout.revision++
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

            // The format and the codecs are the generic ones, the encoder used
            // for each codec is configured in the encoding options.

            AK.LabeledComboBox {
                id: formatCombo
                label: qsTr("File format")
                Accessible.description: label
                model: layout.formats.map(f => recording.formatDescription(f))
                currentIndex: Math.max(layout.formats.indexOf(layout.currentFormat), 0)
                Layout.leftMargin: videoRecording.leftMargin
                Layout.rightMargin: videoRecording.rightMargin
                Layout.fillWidth: true

                onActivated: recording.videoFormat = layout.formats[index]
            }
            AK.LabeledComboBox {
                id: videoCodecCombo
                label: qsTr("Video codec")
                Accessible.description: label
                model: layout.videoCodecs.map(c => recording.codecDescription(c))
                currentIndex: Math.max(layout.videoCodecs.indexOf(recording.codec(AkCaps.CapsVideo)), 0)
                Layout.leftMargin: videoRecording.leftMargin
                Layout.rightMargin: videoRecording.rightMargin
                Layout.fillWidth: true

                onActivated: recording.setCodec(AkCaps.CapsVideo,
                                                layout.videoCodecs[index])
            }
            AK.LabeledComboBox {
                id: audioCodecCombo
                label: qsTr("Audio codec")
                Accessible.description: label
                enabled: recording.recordAudio
                model: layout.audioCodecs.map(c => recording.codecDescription(c))
                currentIndex: Math.max(layout.audioCodecs.indexOf(recording.codec(AkCaps.CapsAudio)), 0)
                Layout.leftMargin: videoRecording.leftMargin
                Layout.rightMargin: videoRecording.rightMargin
                Layout.fillWidth: true

                onActivated: recording.setCodec(AkCaps.CapsAudio,
                                                layout.audioCodecs[index])
            }
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
    }
}
