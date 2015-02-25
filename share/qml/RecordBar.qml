/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

import QtQuick 2.3
import QtQuick.Controls 1.2

Rectangle {
    id: recRecordBar
    color: Qt.rgba(0, 0, 0, 0)
    clip: true
    width: 200
    height: 400

    function updateRecordingFormatList() {
        var curRecordingFormat = Webcamoid.curRecordingFormat
        var formats = Webcamoid.recordingFormats
        lsvRecordingFormatList.model.clear()

        if (formats.length > 0)
            Webcamoid.curRecordingFormat = formats.indexOf(curRecordingFormat) < 0?
                        formats[0]: curRecordingFormat
        else
            Webcamoid.curRecordingFormat = ""

        for (var format in formats) {
            var selected = formats[format] === Webcamoid.curRecordingFormat? true: false

            lsvRecordingFormatList.model.append({
                "name": formats[format],
                "description": Webcamoid.recordingFormatSuffix(formats[format]).join(),
                "selected": selected})
        }
    }

    Component.onCompleted: recRecordBar.updateRecordingFormatList()

    Connections {
        target: Webcamoid
        onRecordingFormatsChanged: recRecordBar.updateRecordingFormatList()
    }

    OptionList {
        id: lsvRecordingFormatList
        anchors.bottom: recAddRecordingFormat.top
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: parent.top
        showField: "name"

        onCurOptionNameChanged: Webcamoid.curRecordingFormat = curOptionName
    }

    Rectangle {
        id: recAddRecordingFormat
        y: 192
        height: 48
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        property color gradUp: Qt.rgba(0, 0.5, 0, 1)
        property color gradLow: Qt.rgba(0, 1, 0, 1)

        gradient: Gradient {
            GradientStop {
                position: 0
                color: recAddRecordingFormat.gradUp
            }
            GradientStop {
                position: 1
                color: recAddRecordingFormat.gradLow
            }
        }

        Image {
            id: imgAddRecordingFormat
            width: 32
            height: 32
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            source: "qrc:/icons/hicolor/scalable/effect-add.svg"
        }

        MouseArea {
            id: msaAddEffect
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            anchors.fill: parent

            onEntered: {
                recAddRecordingFormat.gradUp = Qt.rgba(0, 0.75, 0, 1)
                recAddRecordingFormat.gradLow = Qt.rgba(0.25, 1, 0.25, 1)
            }
            onExited: {
                imgAddRecordingFormat.scale = 1
                recAddRecordingFormat.gradUp = Qt.rgba(0, 0.5, 0, 1)
                recAddRecordingFormat.gradLow = Qt.rgba(0, 1, 0, 1)
            }
            onPressed: imgAddRecordingFormat.scale = 0.75
            onReleased: imgAddRecordingFormat.scale = 1
            onClicked: dlgAddRecordingFormat.visible = true
        }
    }

    AddRecordingFormat {
        id: dlgAddRecordingFormat
    }
}
