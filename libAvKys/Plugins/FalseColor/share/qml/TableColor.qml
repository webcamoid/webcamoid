/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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
import AkControls 1.0 as AK

RowLayout {
    id: rlyColor

    property color tableColor: "black"
    property int index: 0

    signal colorChanged(int index, color tableColor)
    signal colorRemoved(int index)

    AK.ColorButton {
        currentColor: rlyColor.tableColor
        title: qsTr("Select the new color")
        Layout.minimumWidth: AkUnit.create(100 * AkTheme.controlScale, "dp").pixels
        Accessible.description: qsTr("Color %1").arg(rlyColor.index)

        onCurrentColorChanged: rlyColor.colorChanged(rlyColor.index,
                                                     currentColor)
    }
    Button {
        Accessible.name: qsTr("Remove color %1").arg(rlyColor.index)
        icon.source: "image://icons/minus"
        flat: true

        onClicked: rlyColor.colorRemoved(rlyColor.index)
    }
}
