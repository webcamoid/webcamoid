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
import Ak
import AkControls as AK

AK.MenuOption {
    id: root
    title: qsTr("Encoding")
    subtitle: qsTr("Configure the video and audio encoding shared by recording and streaming.")
    icon: "image://icons/settings"

    property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    // The outputs that have to restart to use the new settings, like the local
    // streaming server, wait until the page is closed.
    onVisibleChanged: {
        if (!visible)
            encoding.finishEditing()
    }

    function encoderText(encoder) {
        let text = encoding.encoderDescription(encoder)

        return encoding.encoderIsHardware(encoder)?
                   qsTr("%1 (hardware)").arg(text):
                   text
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: layout.height
        clip: true

        ColumnLayout {
            id: layout
            width: scrollView.width
            layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight

            // Changes each time the encoders or its options change, to force
            // the update of the bindings that call functions.
            property int revision: 0

            // The MJPEG encoder only has a quality setting.
            readonly property string mjpegEncoder: {
                revision

                return encoding.encoder(AkCaps.CapsVideo, "mjpeg")
            }
            readonly property var mjpegQuality: {
                revision

                let options = encoding.encoderOptions(mjpegEncoder)

                for (let i in options) {
                    let option = AkPropertyOption.create(options[i])

                    if (option.name === "quality"
                        && option.type === AkPropertyOption.OptionType_Number
                        && option.menu.length < 1)
                        return option
                }

                return null
            }

            Connections {
                target: encoding

                function onEncoderChanged(type, codec, encoder) {
                    layout.revision++
                }

                function onEncoderOptionValueChanged(encoder, option, value) {
                    layout.revision++
                }
            }

            // Video

            Label {
                text: qsTr("Video")
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
                    id: txtVideoBitrate
                    text: qsTr("Video bitrate (kbps)")
                }
                SpinBox {
                    id: spbVideoBitrate
                    value: Math.round(encoding.videoBitrate / 1000)
                    from: 100
                    to: 200000
                    stepSize: 100
                    editable: true
                    Accessible.name: txtVideoBitrate.text
                    Layout.rightMargin: root.rightMargin

                    onValueModified: encoding.videoBitrate = value * 1000
                }

                Label {
                    id: txtVideoGOP
                    text: qsTr("Keyframes interval (ms)")
                }
                SpinBox {
                    id: spbVideoGOP
                    value: encoding.videoGOP
                    from: 1
                    to: 3600000
                    stepSize: 100
                    editable: true
                    Accessible.name: txtVideoGOP.text
                    Layout.rightMargin: root.rightMargin

                    onValueModified: encoding.videoGOP = value
                }
            }

            Switch {
                id: chkVideoAdvanced
                text: qsTr("Advanced video options")
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
                checked: false
            }

            ColumnLayout {
                visible: chkVideoAdvanced.checked
                Layout.fillWidth: true

                Label {
                    text: qsTr("Video codec associations")
                    font: AkTheme.fontSettings.h6
                    Layout.leftMargin: root.leftMargin
                    Layout.rightMargin: root.rightMargin
                    Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                    Layout.bottomMargin: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
                    Layout.fillWidth: true
                }

                // Only the codecs with an encoder available are shown.
                Repeater {
                    model: {
                        layout.revision

                        return encoding.codecs(AkCaps.CapsVideo)
                                .filter(c => c !== "mjpeg")
                    }

                    RowLayout {
                        id: videoRow
                        layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight
                        Layout.leftMargin: root.leftMargin
                        Layout.rightMargin: root.rightMargin
                        Layout.fillWidth: true

                        required property string modelData

                        readonly property var encoders: {
                            layout.revision

                            return encoding.encoders(AkCaps.CapsVideo, modelData)
                        }
                        readonly property string currentEncoder: {
                            layout.revision

                            return encoding.encoder(AkCaps.CapsVideo, modelData)
                        }

                        AK.LabeledComboBox {
                            label: encoding.codecDescription(videoRow.modelData)
                            Accessible.description: label
                            model: videoRow.encoders.map(e => root.encoderText(e))
                            currentIndex: Math.max(videoRow.encoders.indexOf(videoRow.currentEncoder), 0)
                            Layout.fillWidth: true

                            onActivated: encoding.setEncoder(AkCaps.CapsVideo,
                                                             videoRow.modelData,
                                                             videoRow.encoders[index])
                        }
                        ToolButton {
                            text: qsTr("Configure the encoder")
                            icon.source: "image://icons/edit"
                            implicitWidth: implicitHeight
                            display: AbstractButton.IconOnly
                            ToolTip.visible: hovered
                            ToolTip.text: text
                            Accessible.description: qsTr("Configure the %1 encoder")
                                                    .arg(encoding.codecDescription(videoRow.modelData))
                            Layout.alignment: Qt.AlignBottom

                            onClicked: {
                                videoEncoderOptions.currentEncoder = videoRow.currentEncoder
                                videoEncoderOptions.open()
                            }
                        }
                    }
                }

                Label {
                    visible: layout.mjpegQuality !== null
                    text: layout.mjpegQuality?
                              qsTr("MJPEG quality: %1").arg(mjpegQualitySlider.value):
                              ""
                    font.bold: true
                    Layout.leftMargin: root.leftMargin
                    Layout.rightMargin: root.rightMargin
                    Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                    Layout.fillWidth: true
                }
                Slider {
                    id: mjpegQualitySlider
                    visible: layout.mjpegQuality !== null
                    from: layout.mjpegQuality? layout.mjpegQuality.min: 1
                    to: layout.mjpegQuality? layout.mjpegQuality.max: 100
                    stepSize: layout.mjpegQuality? layout.mjpegQuality.step: 1
                    value: {
                        layout.revision

                        return layout.mjpegQuality?
                                   Number(encoding.encoderOptionValue(layout.mjpegEncoder,
                                                                      "quality")):
                                   0
                    }
                    Accessible.name: qsTr("MJPEG quality")
                    Layout.leftMargin: root.leftMargin
                    Layout.rightMargin: root.rightMargin
                    Layout.fillWidth: true

                    onMoved: encoding.setEncoderOptionValue(layout.mjpegEncoder,
                                                            "quality",
                                                            value)
                }
            }

            // Audio

            Label {
                text: qsTr("Audio")
                font: AkTheme.fontSettings.h6
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.topMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
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
                    id: txtAudioBitrate
                    text: qsTr("Audio bitrate (kbps)")
                }
                SpinBox {
                    id: spbAudioBitrate
                    value: Math.round(encoding.audioBitrate / 1000)
                    from: 8
                    to: 50000
                    stepSize: 8
                    editable: true
                    Accessible.name: txtAudioBitrate.text
                    Layout.rightMargin: root.rightMargin

                    onValueModified: encoding.audioBitrate = value * 1000
                }

                Label {
                    id: txtAudioSampleRate
                    text: qsTr("Audio sampling rate (Hz)")
                }
                SpinBox {
                    id: spbAudioSampleRate
                    value: encoding.audioSampleRate
                    from: 8000
                    to: 512000
                    stepSize: 1000
                    editable: true
                    Accessible.name: txtAudioSampleRate.text
                    Layout.rightMargin: root.rightMargin

                    onValueModified: encoding.audioSampleRate = value
                }
            }

            Switch {
                id: chkAudioAdvanced
                text: qsTr("Advanced audio options")
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
                checked: false
            }

            ColumnLayout {
                visible: chkAudioAdvanced.checked
                Layout.fillWidth: true

                Label {
                    text: qsTr("Audio codec associations")
                    font: AkTheme.fontSettings.h6
                    Layout.leftMargin: root.leftMargin
                    Layout.rightMargin: root.rightMargin
                    Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                    Layout.bottomMargin: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
                    Layout.fillWidth: true
                }

                Repeater {
                    model: {
                        layout.revision

                        return encoding.codecs(AkCaps.CapsAudio)
                    }

                    RowLayout {
                        id: audioRow
                        layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight
                        Layout.leftMargin: root.leftMargin
                        Layout.rightMargin: root.rightMargin
                        Layout.fillWidth: true

                        required property string modelData

                        readonly property var encoders: {
                            layout.revision

                            return encoding.encoders(AkCaps.CapsAudio, modelData)
                        }
                        readonly property string currentEncoder: {
                            layout.revision

                            return encoding.encoder(AkCaps.CapsAudio, modelData)
                        }

                        AK.LabeledComboBox {
                            label: encoding.codecDescription(audioRow.modelData)
                            Accessible.description: label
                            model: audioRow.encoders.map(e => root.encoderText(e))
                            currentIndex: Math.max(audioRow.encoders.indexOf(audioRow.currentEncoder), 0)
                            Layout.fillWidth: true

                            onActivated: encoding.setEncoder(AkCaps.CapsAudio,
                                                             audioRow.modelData,
                                                             audioRow.encoders[index])
                        }
                        ToolButton {
                            // Only when the encoder has options to configure.
                            visible: encoding.encoderOptions(audioRow.currentEncoder).length > 0
                            text: qsTr("Configure the encoder")
                            icon.source: "image://icons/edit"
                            implicitWidth: implicitHeight
                            display: AbstractButton.IconOnly
                            ToolTip.visible: hovered
                            ToolTip.text: text
                            Accessible.description: qsTr("Configure the %1 encoder")
                                                    .arg(encoding.codecDescription(audioRow.modelData))
                            Layout.alignment: Qt.AlignBottom

                            onClicked: {
                                audioEncoderOptions.currentEncoder = audioRow.currentEncoder
                                audioEncoderOptions.open()
                            }
                        }
                    }
                }
            }
        }

        VideoCodecOptions {
            id: videoEncoderOptions
            width: root.Window.width
            height: root.Window.height
            anchors.centerIn: Overlay.overlay
        }
        AudioCodecOptions {
            id: audioEncoderOptions
            width: root.Window.width
            height: root.Window.height
            anchors.centerIn: Overlay.overlay
        }
    }
}
