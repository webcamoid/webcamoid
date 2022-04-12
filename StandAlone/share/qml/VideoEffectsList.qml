/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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
import Ak 1.0

ScrollView {
    id: effectsView

    signal openVideoEffectsDialog()
    signal openVideoEffectOptions(int effectIndex)

    Component.onCompleted: effectsList.update()
    onVisibleChanged: effectsList.forceActiveFocus()

    Connections {
        target: videoEffects

        function onEffectsChanged()
        {
            effectsList.update()
        }
    }

    ColumnLayout {
        width: effectsView.width

        Button {
            text: qsTr("Add effect")
            icon.source: "image://icons/add"
            flat: true

            onClicked: effectsView.openVideoEffectsDialog()
        }
        Button {
            text: qsTr("Remove all effects")
            icon.source: "image://icons/no"
            flat: true

            onClicked: videoEffects.removeAllEffects()
        }
        OptionList {
            id: effectsList
            enableHighlight: false
            Layout.fillWidth: true

            function update() {
                let effects = videoEffects.effects

                for (let i = count - 1; i >= 0; i--)
                    removeItem(itemAt(i))

                for (let i = effects.length - 1; i >= 0; i--) {
                    let component = Qt.createComponent("VideoEffectItem.qml")

                    if (component.status !== Component.Ready)
                        continue

                    let obj = component.createObject(effectsList)
                    let info = AkPluginInfo.create(videoEffects.effectInfo(effects[i]))
                    obj.text = info.description
                    obj.effect = effects[i]

                    obj.onClicked.connect((index => function () {
                        effectsView.openVideoEffectOptions(index)
                    })(i))
                }
            }

            onActiveFocusChanged:
                if (activeFocus && count > 0)
                    itemAt(currentIndex).forceActiveFocus()
            Keys.onUpPressed:
                if (count > 0)
                    itemAt(currentIndex).forceActiveFocus()
            Keys.onDownPressed:
                if (count > 0)
                    itemAt(currentIndex).forceActiveFocus()
        }
    }
}
