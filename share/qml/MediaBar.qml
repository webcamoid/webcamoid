/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

import QtQuick 2.3
import QtQuick.Controls 1.2

Rectangle {
    id: recMediaBar
    color: Qt.rgba(0, 0, 0, 0)
    clip: true
    width: 200
    height: 400

    function updateMediaList() {
        var curStream = Webcamoid.curStream
        var streams = Webcamoid.streams
        lsvMediaList.model.clear()

        if (streams.length > 0)
            Webcamoid.curStream = streams.indexOf(curStream) < 0?
                        streams[0]: curStream
        else
            Webcamoid.curStream = ""

        for (var stream in streams) {
            var selected = streams[stream] === Webcamoid.curStream? true: false

            lsvMediaList.model.append({
                "name": streams[stream],
                "description": Webcamoid.streamDescription(streams[stream]),
                "selected": selected})
        }
    }

    Component.onCompleted: recMediaBar.updateMediaList()

    Connections {
        target: Webcamoid
        onStreamsChanged: recMediaBar.updateMediaList()
    }

    OptionList {
        id: lsvMediaList
        anchors.bottom: recAddMedia.top
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: parent.top

        onCurOptionNameChanged: Webcamoid.curStream = curOptionName
    }

    Rectangle {
        id: recAddMedia
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
                color: recAddMedia.gradUp
            }
            GradientStop {
                position: 1
                color: recAddMedia.gradLow
            }
        }

        Image {
            id: imgAddMedia
            width: 32
            height: 32
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            source: "qrc:/Webcamoid/share/icons/default/scalable/add.svg"
        }

        MouseArea {
            id: msaAddMedia
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            anchors.fill: parent

            onEntered: {
                recAddMedia.gradUp = Qt.rgba(0, 0.75, 0, 1)
                recAddMedia.gradLow = Qt.rgba(0.25, 1, 0.25, 1)
            }
            onExited: {
                imgAddMedia.scale = 1
                recAddMedia.gradUp = Qt.rgba(0, 0.5, 0, 1)
                recAddMedia.gradLow = Qt.rgba(0, 1, 0, 1)
            }
            onPressed: imgAddMedia.scale = 0.75
            onReleased: imgAddMedia.scale = 1
            onClicked: dlgAddMedia.visible = true
        }
    }

    AddMedia {
        id: dlgAddMedia
    }
}
