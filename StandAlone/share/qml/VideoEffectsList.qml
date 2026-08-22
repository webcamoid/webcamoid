/* Webcamoid, camera capture application.
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ak

ScrollView {
    id: effectsView

    property int sourceId: -1

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft
    readonly property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    readonly property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    readonly property bool isSource: sourceId >= 0

    signal openVideoEffectsDialog(int sourceId)
    signal openVideoEffectOptions(int sourceId, int effectIndex)

    function activeEffects() {
        if (effectsView.isSource)
            return videoEffects.sourceEffects(effectsView.sourceId)

        return videoEffects.effects
    }

    Component.onCompleted: effectsList.update()
    onVisibleChanged: {
        if (visible)
            effectsList.update()

        effectsList.forceActiveFocus()
    }
    onSourceIdChanged: effectsList.update()

    Connections {
        target: videoEffects

        function onEffectsChanged()
        {
            if (!effectsView.isSource)
                effectsList.update()
        }

        function onSourceEffectsChanged(id)
        {
            if (effectsView.isSource && id === effectsView.sourceId)
                effectsList.update()
        }
    }

    ColumnLayout {
        width: effectsView.width
        layoutDirection: effectsView.rtl? Qt.RightToLeft: Qt.LeftToRight

        Label {
            id: deviceInfo
            text: {
                if (!effectsView.isSource)
                    return ""

                return "<b>" + videoLayer.sourceLabel(effectsView.sourceId) + "</b>"
                       + "<br/><i>" + videoLayer.sourceDevice(effectsView.sourceId) + "</i>"
            }
            elide: Label.ElideRight
            height: effectsView.isSource? undefined: 0
            visible: effectsView.isSource
            Layout.fillWidth: true
            Layout.leftMargin: effectsView.leftMargin
            Layout.rightMargin: effectsView.rightMargin
            Layout.bottomMargin:
                AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        }

        Button {
            text: qsTr("Add effect")
            icon.source: "image://icons/add"
            flat: true

            onClicked: effectsView.openVideoEffectsDialog(effectsView.sourceId)
        }
        Button {
            text: qsTr("Remove all effects")
            icon.source: "image://icons/no"
            flat: true

            onClicked: {
                if (effectsView.isSource)
                    videoEffects.removeAllSourceEffects(effectsView.sourceId)
                else
                    videoEffects.removeAllEffects()
            }
        }
        OptionList {
            id: effectsList
            enableHighlight: false
            Layout.fillWidth: true
            Layout.minimumHeight: minHeight

            property int minHeight: 0

            function update() {
                effectsList.minHeight = 0
                let effects = effectsView.activeEffects()

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
                    effectsList.minHeight += obj.height

                    obj.onClicked.connect((index => function () {
                        effectsView.openVideoEffectOptions(effectsView.sourceId,
                                                           index)
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
