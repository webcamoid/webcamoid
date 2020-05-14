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
    id: view
    property int effectIndex: -1
    readonly property string effectDescription: description(effectIndex)

    signal effectRemoved()

    function description(effectIndex)
    {
        if (effectIndex < 0 || effectIndex >= videoEffects.effects.lenght)
            return ""

        let effect = videoEffects.effects[effectIndex]
        let info = videoEffects.effectInfo(effect)

        if (!info)
            return ""

        let metaData = info["MetaData"]

        if (!metaData)
            return ""

        let description_ = metaData["description"]

        if (!description_)
            return ""

        return description_
    }

    ColumnLayout {
        width: view.width
        spacing: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

        Button {
            text: qsTr("Remove")
            icon.source: "image://icons/no"
            flat: true
            Layout.leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

            onClicked: {
                videoEffects.removeInterface("itmEffectControls")
                videoEffects.removeEffect(view.effectIndex)
                view.effectRemoved()
            }
        }
        ColumnLayout {
            id: itmEffectControls
            objectName: "itmEffectControls"
            width: view.width
            Layout.leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        }
    }

    onEffectIndexChanged: {
        videoEffects.removeInterface("itmEffectControls")

        if (effectIndex < 0 || effectIndex >= videoEffects.effects.lenght)
            return

        videoEffects.embedControls("itmEffectControls", effectIndex)
    }
}
