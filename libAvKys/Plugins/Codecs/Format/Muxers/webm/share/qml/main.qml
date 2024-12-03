/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

GridLayout {
    columns: 2

    Label {
        id: txtBitrate
        text: qsTr("Bitrate")
    }
    TextField {
        text: VideoMuxerWebm.bitrate
        placeholderText: qsTr("Bitrate (bits/secs)")
        Accessible.name: txtBitrate.text
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /\d+/
        }
        Layout.fillWidth: true

        onTextChanged: VideoMuxerWebm.bitrate = Number(text)
    }
    Label {
        id: txtGOP
        text: qsTr("Keyframes stride (ms)")
    }
    TextField {
        text: VideoMuxerWebm.gop
        placeholderText: qsTr("1000")
        Accessible.name: txtGOP.text
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /\d+/
        }
        Layout.fillWidth: true

        onTextChanged: VideoMuxerWebm.gop = Number(text)
    }
    GroupBox {
        title: qsTr("Error resiliency")
        Layout.columnSpan: 2
        Layout.fillWidth: true
        clip: true
        Accessible.name: title

        ColumnLayout {
            CheckBox {
                checked: MultiSrc.errorResilient & MultiSrc.ErrorResilientFlag_Default
                text: qsTr("Default")
                Accessible.description: text

                onToggled: {
                    if (checked)
                        MultiSrc.errorResilient |= MultiSrc.ErrorResilientFlag_Default
                    else
                        MultiSrc.errorResilient &= ~MultiSrc.ErrorResilientFlag_Default
                }
            }
            CheckBox {
                checked: MultiSrc.errorResilient & MultiSrc.ErrorResilientFlag_Partitions
                text: qsTr("Partitions")
                Accessible.description: text

                onToggled: {
                    if (checked)
                        MultiSrc.errorResilient |= MultiSrc.ErrorResilientFlag_Partitions
                    else
                        MultiSrc.errorResilient &= ~MultiSrc.ErrorResilientFlag_Partitions
                }
            }
        }
    }
    Label {
        id: txtDeadline
        text: qsTr("Deadline (us)")
        Layout.columnSpan: 2
    }
    RowLayout {
        Layout.columnSpan: 2
        Layout.fillWidth: true

        ComboBox {
            id: cbxQualityOptions
            textRole: "text"
            currentIndex: VideoMuxerWebm.deadline == VideoMuxerWebm.Deadline_Realtime?
                                0:
                          VideoMuxerWebm.deadline == VideoMuxerWebm.Deadline_GoodQuality?
                                1:
                          VideoMuxerWebm.deadline == VideoMuxerWebm.Deadline_BestQuality?
                                2:
                                cbxQualityOptions.model.count - 1
            Accessible.description: txtDeadline.text

            model: ListModel {
                ListElement {
                    text: qsTr("Realtime")
                    deadline: VideoMuxerWebm.Deadline_Realtime
                }
                ListElement {
                    text: qsTr("Good quality")
                    deadline: VideoMuxerWebm.Deadline_GoodQuality
                }
                ListElement {
                    text: qsTr("Best quality")
                    deadline: VideoMuxerWebm.Deadline_BestQuality
                }
                ListElement {
                    text: qsTr("Custom")
                    deadline: VideoMuxerWebm.Deadline_Realtime
                }
            }

            onCurrentIndexChanged: VideoMuxerWebm.deadline = model.get(currentIndex).deadline
        }
        TextField {
            text: VideoMuxerWebm.deadline
            placeholderText: qsTr("Time in micro-seconds")
            Accessible.name: txtDeadline.text
            selectByMouse: true
            validator: RegularExpressionValidator {
                regularExpression: /\d+/
            }
            Layout.fillWidth: true
            enabled: cbxQualityOptions.currentindex == (cbxQualityOptions.model.count - 1)

            onTextChanged: VideoMuxerWebm.deadline = Number(text)
        }
    }
}
