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
    id: view
    clip: true

    property int sourceId: -1
    property int effectIndex: -1

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft
    readonly property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    readonly property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    readonly property bool isSource: sourceId >= 0
    readonly property string effectDescription: description(effectIndex)

    signal effectRemoved()

    function description(effectIndex)
    {
        let effects = view.isSource?
                        videoEffects.sourceEffects(view.sourceId):
                        videoEffects.effects

        if (effectIndex < 0 || effectIndex >= effects.length)
            return ""

        let effect = effects[effectIndex]
        let info = view.isSource?
                       AkPluginInfo.create(videoEffects.sourceEffectInfo(view.sourceId, effectIndex)):
                       AkPluginInfo.create(videoEffects.effectInfo(effect))

        if (!info)
            return ""

        return info.description
    }

    function updateControls()
    {
        videoEffects.removeInterface("itmEffectControls")

        if (view.sourceId >= 0)
            videoEffects.embedControls("itmEffectControls",
                                        view.sourceId,
                                        view.effectIndex)
        else
            videoEffects.embedControls("itmEffectControls", view.effectIndex)
    }

    ColumnLayout {
        width: view.width
        layoutDirection: view.rtl? Qt.RightToLeft: Qt.LeftToRight

        Label {
            id: deviceInfo
            text: {
                if (!view.isSource)
                    return ""

                return "<b>" + videoLayer.sourceLabel(view.sourceId) + "</b>"
                       + "<br/><i>" + videoLayer.sourceDevice(view.sourceId) + "</i>"
            }
            elide: Label.ElideRight
            height: view.isSource? undefined: 0
            visible: view.isSource
            Layout.fillWidth: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Layout.bottomMargin:
            AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        }

        Button {
            text: qsTr("Remove")
            icon.source: "image://icons/no"
            flat: true
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
            Accessible.description: qsTr("Remove %1 video effect").arg(view.effectDescription)

            onClicked: {
                videoEffects.removeInterface("itmEffectControls")

                if (view.isSource)
                    videoEffects.removeSourceEffect(view.sourceId, view.effectIndex)
                else
                    videoEffects.removeEffect(view.effectIndex)

                view.effectRemoved()
            }
        }
        ColumnLayout {
            id: itmEffectControls
            objectName: "itmEffectControls"
            width: view.width
            Layout.leftMargin: view.leftMargin
            Layout.rightMargin: view.rightMargin
        }
    }

    onEffectIndexChanged: view.updateControls()
    onSourceIdChanged: view.updateControls()
}
