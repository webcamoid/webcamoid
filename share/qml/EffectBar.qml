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
    id: recEffectBar
    color: Qt.rgba(0, 0, 0, 1)
    clip: true
    width: 200
    height: 400

    function updateEffectList() {
        var curEffect = Webcamoid.curStream
        var effects = Webcamoid.availableEffects()
        lsvEffectList.model.clear()

        for (var effect in effects) {
            var selected = false //streams[stream] === Webcamoid.curStream? true: false

            lsvEffectList.model.append({
                "name": effects[effect],
                "description": Webcamoid.effectInfo(effects[effect])["MetaData"]["description"],
                "selected": selected})
        }
    }

    Component.onCompleted: recEffectBar.updateEffectList()

    Connections {
        target: Webcamoid
        onStreamsChanged: recEffectBar.updateEffectList()
    }

    TextField {
        id: txtSearchEffect
        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.rightMargin: 8
        anchors.leftMargin: 8
        anchors.right: parent.right
        anchors.left: parent.left
        placeholderText: "Search effect..."
    }

    OptionList {
        id: lsvAppliedEffectList
        anchors.topMargin: 8
        anchors.bottom: recAddEffect.top
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: txtSearchEffect.bottom
        filter: txtSearchEffect.text

        onCurOptionNameChanged: {
        }
    }

    ScrollView {
        id: scrollEffects
        visible: false
        anchors.topMargin: 8
        anchors.bottom: recAddEffect.top
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: txtSearchEffect.bottom

        OptionList {
            id: lsvEffectList
            filter: txtSearchEffect.text

            onCurOptionNameChanged: {
            }
        }
    }

    Rectangle {
        id: recAddEffect
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
                color: recAddEffect.gradUp
            }
            GradientStop {
                position: 1
                color: recAddEffect.gradLow
            }
        }

        Image {
            id: imgAddEffect
            width: 32
            height: 32
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            source: "qrc:/Webcamoid/share/icons/add.svg"
        }

        MouseArea {
            id: msaAddEffect
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            anchors.fill: parent

            onEntered: {
                recAddEffect.gradUp = Qt.rgba(0, 0.75, 0, 1)
                recAddEffect.gradLow = Qt.rgba(0.25, 1, 0.25, 1)
            }
            onExited: {
                imgAddEffect.scale = 1
                recAddEffect.gradUp = Qt.rgba(0, 0.5, 0, 1)
                recAddEffect.gradLow = Qt.rgba(0, 1, 0, 1)
            }
            onPressed: imgAddEffect.scale = 0.75
            onReleased: imgAddEffect.scale = 1
            onClicked: {
                if (recEffectBar.state == "") {
                    imgAddEffect.source = "qrc:/Webcamoid/share/icons/down.svg"
                    recEffectBar.state = "showEffects"
                }
                else {
                    imgAddEffect.source = "qrc:/Webcamoid/share/icons/add.svg"
                    recEffectBar.state = ""
                }
            }
        }
    }

    states: [
        State {
            name: "showEffects"
            PropertyChanges {
                target: lsvAppliedEffectList
                visible: false
            }
            PropertyChanges {
                target: scrollEffects
                visible: true
            }
        }
    ]
}
