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

ScrollView {
    id: effectsView

    signal openVideoEffectsDialog()
    signal openVideoEffectOptions(int effectIndex)

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
        ListView {
            id: effectsList
            model: ListModel {}
            implicitWidth: childrenRect.width
            implicitHeight: childrenRect.height
            Layout.fillWidth: true
            Layout.fillHeight: true

            function updateEffectList() {
                let effects = videoEffects.effects
                model.clear()

                for (let i = effects.length - 1; i >= 0; i--) {
                    let effect = effects[i]
                    let info = videoEffects.effectInfo(effect)

                    model.append({
                        effect: effect,
                        description: info["MetaData"]["description"]})
                }
            }

            delegate: ItemDelegate {
                text: index < 0 && index >= effectsList.count?
                          "":
                      effectsList.model.get(index)?
                          effectsList.model.get(index)["description"]:
                          ""
                anchors.right: parent.right
                anchors.left: parent.left
                height: implicitHeight

                onClicked:
                    effectsView.openVideoEffectOptions(effectsList.count
                                                       - index
                                                       - 1)
            }

            Connections {
                target: videoEffects

                onEffectsChanged: effectsList.updateEffectList()
            }

            Component.onCompleted: effectsList.updateEffectList()
        }
    }
}
