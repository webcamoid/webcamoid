/* Webcamoid, webcam capture application.
 * Copyright (C) 2015  Gonzalo Exequiel Pedone
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

import QtQuick 2.7
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3

ColumnLayout {
    id: recEffectBar

    property string curEffect: ""
    property int curEffectIndex: -1
    property bool editMode: false
    property bool advancedMode: VideoEffects.advancedMode
    property bool lock: false

    function updateAppliedEffectList() {
        var effects = VideoEffects.effects

        var option = lsvAppliedEffectList.model.get(lsvAppliedEffectList.currentIndex)
        var curEffect = option? option.effect: ""
        var currentIndex = -1
        lsvAppliedEffectList.model.clear()

        for (var effect in effects) {
            if (effects[effect] === curEffect)
                currentIndex = effect

            lsvAppliedEffectList.model.append({
                effect: effects[effect],
                description: VideoEffects.effectInfo(effects[effect])["MetaData"]["description"]})
        }

        lsvAppliedEffectList.currentIndex = currentIndex < 0? 0: currentIndex
    }

    function updateEffectList() {
        var effects = VideoEffects.availableEffects
        var curEffects = VideoEffects.effects
        var curEffect = curEffects.length > 0?
                    curEffects[curEffects.length - 1]: advancedMode?
                        effects[0]: ""
        var currentIndex = -1
        lsvEffectList.model.clear()

        for (var effect in effects) {
            if (effects[effect] === curEffect)
                currentIndex = effect

            lsvEffectList.model.append({
                effect: effects[effect],
                description: VideoEffects.effectInfo(effects[effect])["MetaData"]["description"]})
        }

        lsvEffectList.currentIndex = currentIndex >= 0 || !advancedMode?
                                     currentIndex: -1
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
        target: VideoEffects

        onAvailableEffectsChanged: {
            recEffectBar.updateAppliedEffectList()
            recEffectBar.updateEffectList()
        }
        onEffectsChanged: {
            if (lock)
                return

            recEffectBar.updateAppliedEffectList()
            recEffectBar.updateEffectList()
            recEffectBar.editMode = false
        }
        onAdvancedModeChanged: {
            updateAppliedEffectList()
            updateEffectList()
        }
    }

    TextField {
        id: txtSearchEffect
        placeholderText: qsTr("Search effect")
        Layout.fillWidth: true
    }
    Button {
        id: btnAddEffect
        text: qsTr("Add effect")
        visible: advancedMode
        Layout.fillWidth: true

        onClicked: {
            recEffectBar.state = (recEffectBar.state == "")?
                        "showEffects": ""
        }
    }
    Button {
        id: effectResetButton
        text: qsTr("None")
        Layout.fillWidth: true

        onClicked: {
            lsvEffectList.currentIndex = -1
            VideoEffects.effects = []
            recEffectBar.curEffect = ""
            recEffectBar.curEffectIndex = -1
        }
    }
    OptionList {
        id: lsvAppliedEffectList
        textRole: "description"
        filter: txtSearchEffect.text
        visible: advancedMode
        Layout.fillWidth: true

        onCurrentIndexChanged: {
            var option = model.get(currentIndex)
            recEffectBar.curEffect = option? option.effect: ""
            recEffectBar.curEffectIndex = currentIndex
            txtSearchEffect.text = ""
        }
        onVisibleChanged: {
            if (visible) {
                var option = model.get(currentIndex)
                recEffectBar.curEffect = option? option.effect: ""
                recEffectBar.curEffectIndex = currentIndex
                txtSearchEffect.text = ""
            }
        }
    }

    OptionList {
        id: lsvEffectList
        textRole: "description"
        filter: txtSearchEffect.text
        visible: !advancedMode
        Layout.fillWidth: true

        onVisibleChanged: {
            if (!advancedMode)
                return

            if (visible) {
                var option = lsvEffectList.model.get(lsvEffectList.currentIndex)
                var effect = option? option.effect: ""
                VideoEffects.showPreview(effect)
                recEffectBar.curEffect = effect
                recEffectBar.curEffectIndex = 0
            } else
                VideoEffects.removeAllPreviews()
        }

        onCurrentIndexChanged: {
            var option = model.get(currentIndex)

            if (!option)
                return

            recEffectBar.lock = true
            var effect = option.effect

            if (advancedMode)
                VideoEffects.showPreview(effect)
            else {
                VideoEffects.effects = []
                VideoEffects.appendEffect(effect, false)
            }

            recEffectBar.curEffect = effect

            if (!advancedMode)
                recEffectBar.curEffectIndex = 0

            txtSearchEffect.text = ""
            recEffectBar.lock = false
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
                target: lsvEffectList
                visible: true
            }
            PropertyChanges {
                target: btnAddEffect
                text: qsTr("Go back")
            }
            PropertyChanges {
                target: recEffectBar
                editMode: true
            }
        }
    ]
}
