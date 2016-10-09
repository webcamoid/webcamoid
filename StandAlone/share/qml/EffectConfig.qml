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
 * Web-Site: http://webcamoid.github.io/
 */

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1

ColumnLayout {
    id: recEffectConfig

    property string curEffect: ""
    property bool inUse: false
    property bool advancedMode: Webcamoid.advancedMode

    signal effectAdded(string effectId)

    Connections {
        target: Webcamoid

        onInterfaceLoaded: {
            var currentEffects = Webcamoid.currentEffects

            if (currentEffects.length > 0) {
                Webcamoid.removeInterface("itmEffectControls")
                Webcamoid.embedEffectControls("itmEffectControls", currentEffects[0])
            }
        }
    }

    onCurEffectChanged: {
        Webcamoid.removeInterface("itmEffectControls")
        Webcamoid.embedEffectControls("itmEffectControls", curEffect)
    }

    Label {
        id: lblEffect
        text: qsTr("Plugin id")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        id: txtEffect
        text: recEffectConfig.curEffect
        placeholderText: qsTr("Plugin id")
        readOnly: true
        Layout.fillWidth: true
    }
    Label {
        id: lblDescription
        text: qsTr("Description")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        id: txtDescription
        text: Webcamoid.effectDescription(recEffectConfig.curEffect)
        placeholderText: qsTr("Plugin description")
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
            iconName: inUse? "remove": "add"
            iconSource: inUse? "image://icons/remove":
                               "image://icons/add"
            enabled: recEffectConfig.curEffect == ""? false: true

            onClicked: {
                if (inUse) {
                    Webcamoid.removeEffect(recEffectConfig.curEffect)

                    if (Webcamoid.currentEffects.length < 1)
                        recEffectConfig.curEffect = ""
                }
                else {
                    Webcamoid.setAsPreview(recEffectConfig.curEffect, false)
                    recEffectConfig.effectAdded(recEffectConfig.curEffect)
                }
            }
        }
    }

    ScrollView {
        id: scrollControls
        Layout.fillWidth: true
        Layout.fillHeight: true

        contentItem: RowLayout {
            id: itmEffectControls
            objectName: "itmEffectControls"
            width: scrollControls.viewport.width
        }
    }
}
