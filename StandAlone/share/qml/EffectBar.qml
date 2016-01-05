/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

import QtQuick 2.5
import QtQuick.Controls 1.4

Rectangle {
    id: recEffectBar
    color: Qt.rgba(0, 0, 0, 0)
    clip: true
    width: 200
    height: 400

    property string curEffect: ""
    property bool editMode: false
    property bool advancedMode: Webcamoid.advancedMode

    function updateAppliedEffectList() {
        var effects = Webcamoid.currentEffects

        var option = lsvAppliedEffectList.model.get(lsvAppliedEffectList.currentIndex)
        var curEffect = option? option.effect: ""
        var currentIndex = -1
        lsvAppliedEffectList.model.clear()

        for (var effect in effects) {
            if (effects[effect] === curEffect)
                currentIndex = effect

            lsvAppliedEffectList.model.append({
                effect: effects[effect],
                description: Webcamoid.effectInfo(effects[effect])["MetaData"]["description"]})
        }

        lsvAppliedEffectList.currentIndex = currentIndex < 0? 0: currentIndex
    }

    function updateEffectList() {
        var effects = Webcamoid.availableEffects()

        var curEffects = Webcamoid.currentEffects
        var option = lsvEffectList.model.get(lsvEffectList.currentIndex)
        var curEffect = option? option.effect: curEffects.length > 0? curEffects[0]: ""
        var currentIndex = -1
        lsvEffectList.model.clear()

        for (var effect in effects) {
            if (effects[effect] === curEffect)
                currentIndex = effect

            lsvEffectList.model.append({
                effect: effects[effect],
                description: Webcamoid.effectInfo(effects[effect])["MetaData"]["description"]})
        }

        lsvEffectList.currentIndex = currentIndex >= 0 || !advancedMode?
                                     currentIndex: 0
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
            if (advancedMode) {
                recEffectBar.updateAppliedEffectList()
                recEffectBar.updateEffectList()
            }

            recEffectBar.editMode = false
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

    Rectangle {
        id: effectResetButton
        height: advancedMode? 0: 32
        anchors.topMargin: 8
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: txtSearchEffect.bottom
        visible: advancedMode? false: true

        property bool selected: Webcamoid.currentEffects.length < 1

        property color gradUp: selected?
                                   Qt.rgba(0.75, 0, 0, 1):
                                   Qt.rgba(0.25, 0, 0, 1)
        property color gradLow: selected?
                                    Qt.rgba(1, 0, 0, 1):
                                    Qt.rgba(0.5, 0, 0, 1)

        onSelectedChanged: {
            gradUp = selected?
                        Qt.rgba(0.75, 0, 0, 1):
                        Qt.rgba(0.25, 0, 0, 1)
            gradLow = selected?
                        Qt.rgba(1, 0, 0, 1):
                        Qt.rgba(0.5, 0, 0, 1)
        }

        gradient: Gradient {
            GradientStop {
                position: 0
                color: effectResetButton.gradUp
            }

            GradientStop {
                position: 1
                color: effectResetButton.gradLow
            }
        }

        Label {
            id: txtEffectResetButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("None")
        }

        MouseArea {
            id: msaEffectResetButton
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            anchors.fill: parent

            onEntered: {
                txtEffectResetButton.font.bold = true
                effectResetButton.gradUp = effectResetButton.selected?
                                            Qt.rgba(1, 0.25, 0.25, 1):
                                            Qt.rgba(0.5, 0.25, 0.25, 1)
                effectResetButton.gradLow = effectResetButton.selected?
                                            Qt.rgba(1, 0.25, 0.25, 1):
                                            Qt.rgba(0.75, 0.25, 0.25, 1)
            }
            onExited: {
                txtEffectResetButton.font.bold = false
                txtEffectResetButton.scale = 1
                effectResetButton.gradUp = effectResetButton.selected?
                                            Qt.rgba(0.75, 0, 0, 1):
                                            Qt.rgba(0.25, 0, 0, 1)
                effectResetButton.gradLow = effectResetButton.selected?
                                            Qt.rgba(1, 0, 0, 1):
                                            Qt.rgba(0.5, 0, 0, 1)
            }
            onPressed: txtEffectResetButton.scale = 0.75
            onReleased: txtEffectResetButton.scale = 1
            onClicked: {
                lsvEffectList.currentIndex = -1
                Webcamoid.resetEffects()
                recEffectBar.curEffect = ""
            }
        }
    }

    OptionList {
        id: lsvAppliedEffectList
        anchors.bottom: recAddEffect.top
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: effectResetButton.bottom
        textRole: "description"
        filter: txtSearchEffect.text
        visible: advancedMode? true: false

        onCurrentIndexChanged: {
            var option = model.get(currentIndex)
            recEffectBar.curEffect = option? option.effect: ""
            txtSearchEffect.text = ""
        }
        onVisibleChanged: {
            if (visible) {
                var option = model.get(currentIndex)
                recEffectBar.curEffect = option? option.effect: ""
                txtSearchEffect.text = ""
            }
        }
    }

    ScrollView {
        id: scrollEffects
        visible: advancedMode? false: true
        anchors.bottom: recAddEffect.top
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: effectResetButton.bottom

        onVisibleChanged: {
            if (!advancedMode)
                return

            if (visible) {
                var option = lsvEffectList.model.get(lsvEffectList.currentIndex)
                var effect = option? option.effect: ""
                Webcamoid.showPreview(effect)
                recEffectBar.curEffect = effect
            }
            else
                Webcamoid.removePreview()
        }

        OptionList {
            id: lsvEffectList
            filter: txtSearchEffect.text
            textRole: "description"

            onCurrentIndexChanged: {
                var option = model.get(currentIndex)

                if (!option)
                    return

                var effect = option.effect

                if (scrollEffects.visible) {
                    if (advancedMode)
                        Webcamoid.showPreview(effect)
                    else {
                        Webcamoid.resetEffects()
                        Webcamoid.appendEffect(effect, false)
                    }
                }

                recEffectBar.curEffect = effect
                txtSearchEffect.text = ""
            }
        }
    }

    Rectangle {
        id: recAddEffect
        height: advancedMode? 48: 0
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        visible: advancedMode? true: false

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
            source: "qrc:/icons/hicolor/scalable/effect-add.svg"
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
                source: "qrc:/icons/hicolor/scalable/down.svg"
            }
            PropertyChanges {
                target: recEffectBar
                editMode: true
            }
        }
    ]
}
