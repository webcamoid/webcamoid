/* Webcamoid, camera capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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
import AkControls as AK

ColumnLayout {
    // Configure strip size.
    Label {
        id: lblStripSize
        text: qsTr("Size")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldStripSize
        value: Cinema.stripSize
        stepSize: 0.01
        to: 1
        Accessible.name: lblStripSize.text
        Layout.fillWidth: true

        onValueChanged: Cinema.stripSize = value
    }

    // Configure strip color.
    RowLayout {
        Label {
            id: txtColor
            text: qsTr("Color")
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(Cinema.stripColor)
            title: qsTr("Choose the strips color")
            showAlphaChannel: true
            Accessible.description: txtColor.text

            onCurrentColorChanged: Cinema.stripColor = AkUtils.toRgba(currentColor)
        }
    }
}
