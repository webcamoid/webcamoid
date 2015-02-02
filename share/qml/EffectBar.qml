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
    color: Qt.rgba(0, 0, 0, 0)
    clip: true
    width: 200
    height: 400

    property string curEffect: ""
    property string testEffect: ""
    property bool editMode: false

    function updateAppliedEffectList() {
        var effects = Webcamoid.currentEffects
        var curEffect = lsvAppliedEffectList.curOptionName
        lsvAppliedEffectList.model.clear()

        if (effects.length > 0
            && effects.indexOf(curEffect) < 0)
            curEffect = effects[0]

        for (var effect in effects) {
            var selected = false

            if (effects[effect] === curEffect) {
                selected = true
                lsvAppliedEffectList.curOptionName = effects[effect]
            }

            lsvAppliedEffectList.model.append({
                "name": effects[effect],
                "description": Webcamoid.effectInfo(effects[effect])["MetaData"]["description"],
                "selected": selected})
        }
    }

    function updateEffectList() {
        var effects = Webcamoid.availableEffects()
        var curEffect = lsvEffectList.curOptionName
        lsvEffectList.model.clear()

        if (effects.length > 0
            && effects.indexOf(curEffect) < 0)
            curEffect = effects[0]

        for (var effect in effects) {
            var selected = false

            if (effects[effect] === curEffect) {
                selected = true
                lsvEffectList.curOptionName = effects[effect]
            }

            lsvEffectList.model.append({
                "name": effects[effect],
                "description": Webcamoid.effectInfo(effects[effect])["MetaData"]["description"],
                "selected": selected})
        }
    }

    onEditModeChanged: {
        state = editMode? "showEffects": ""
    }

    onVisibleChanged: {
        if (!visible)
            recEffectBar.state = ""
    }

    Component.onCompleted: {
        recEffectBar.updateAppliedEffectList()
        recEffectBar.updateEffectList()
    }

    Connections {
        target: Webcamoid
        onCurrentEffectsChanged: {
            recEffectBar.updateAppliedEffectList()
            recEffectBar.updateEffectList()
        }
    }

    TextField {
        id: txtSearchEffect
        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.rightMargin: 8
        anchors.leftMargin: 8
        anchors.right: parent.right
        anchors.left: parent.left
        placeholderText: qsTr("Search effect...")
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
            recEffectBar.curEffect = curOptionName
        }
        onVisibleChanged: {
            if (visible)
                recEffectBar.curEffect = curOptionName
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

        onVisibleChanged: {
            if (visible) {
                recEffectBar.curEffect = lsvEffectList.curOptionName
                Webcamoid.showPreview(lsvEffectList.curOptionName)
                Webcamoid.removeEffectControls("itmEffectControls")
                Webcamoid.embedEffectControls("itmEffectControls", lsvEffectList.curOptionName)
            }
            else
                Webcamoid.removePreview()
        }

        OptionList {
            id: lsvEffectList
            filter: txtSearchEffect.text

            onCurOptionNameChanged: {
                recEffectBar.curEffect = curOptionName

                if (scrollEffects.visible) {
                    Webcamoid.showPreview(curOptionName)
                    Webcamoid.removeEffectControls("itmEffectControls")
                    Webcamoid.embedEffectControls("itmEffectControls", curOptionName)
                }
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
                recEffectBar.state = (recEffectBar.state == "")?
                            "showEffects": ""
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
            PropertyChanges {
                target: imgAddEffect
                source: "qrc:/Webcamoid/share/icons/down.svg"
            }
            PropertyChanges {
                editMode: true
                target: recEffectBar
            }
        }
    ]
}
