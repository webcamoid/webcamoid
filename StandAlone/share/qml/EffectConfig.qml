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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3

ColumnLayout {
    id: recEffectConfig

    property string curEffect: ""
    property int curEffectIndex: -1
    property bool inUse: false
    property bool editMode: false
    property bool advancedMode: VideoEffects.advancedMode

    signal effectAdded(string effectId)

    function showDefaultEffect()
    {
        var currentEffects = VideoEffects.effects

        if (currentEffects.length > 0) {
            VideoEffects.removeInterface("itmEffectControls")
            VideoEffects.embedControls("itmEffectControls", 0)
            curEffect = currentEffects[0]
            curEffectIndex = 0
            inUse = true
        } else {
            curEffect = ""
            curEffectIndex = -1
            inUse = false
        }
    }

    Connections {
        target: Webcamoid

        onInterfaceLoaded: showDefaultEffect()
    }
    Connections {
        target: VideoEffects

        onEffectsChanged: showDefaultEffect()
    }

    onCurEffectChanged: {
        VideoEffects.removeInterface("itmEffectControls")

        recEffectConfig.curEffect = curEffect
        recEffectConfig.curEffectIndex = VideoEffects.effects.indexOf(curEffect)
        recEffectConfig.inUse = recEffectConfig.curEffectIndex >= 0

        if (curEffect.length > 0) {
            var effectIndex = recEffectConfig.curEffectIndex

            if (effectIndex < 0)
                effectIndex = VideoEffects.effects.length

            VideoEffects.embedControls("itmEffectControls", effectIndex)
        }
    }

    Label {
        text: qsTr("Description")
        font.bold: true
    }
    TextField {
        id: txtDescription
        text: VideoEffects.effectDescription(recEffectConfig.curEffect)
        placeholderText: qsTr("Description")
        readOnly: true
        Layout.fillWidth: true
    }
    Label {
        text: qsTr("Plugin ID")
        font.bold: true
    }
    TextField {
        id: txtEffect
        text: recEffectConfig.curEffect
        placeholderText: qsTr("Plugin ID")
        readOnly: true
        Layout.fillWidth: true
    }

    RowLayout {
        id: rowControls
        Layout.fillWidth: true
        visible: advancedMode? true: false

        Label {
            Layout.fillWidth: true
        }

        Button {
            id: btnAddRemove
            text: inUse? qsTr("Remove"): qsTr("Add")
            icon.source: inUse? "image://icons/no": "image://icons/add"
            enabled: recEffectConfig.curEffect == ""? false: true

            onClicked: {
                var effectIndex = VideoEffects.effects.indexOf(recEffectConfig.curEffect)

                if (effectIndex < 0)
                    effectIndex = VideoEffects.effects.length

                if (inUse) {
                    VideoEffects.removeEffect(effectIndex)

                    if (VideoEffects.effects.length < 1)
                        recEffectConfig.curEffect = ""
                } else {
                    VideoEffects.setAsPreview(effectIndex, false)
                    recEffectConfig.effectAdded(recEffectConfig.curEffect)
                }
            }
        }
    }
    RowLayout {
        id: itmEffectControls
        objectName: "itmEffectControls"
    }
}
