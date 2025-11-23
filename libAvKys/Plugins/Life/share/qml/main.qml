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
    RowLayout {
        Label {
            id: txtColor
            text: qsTr("Color")
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(Life.lifeColor)
            //: https://en.wikipedia.org/wiki/Life-like_cellular_automaton
            title: qsTr("Choose the automata color")
            showAlphaChannel: true
            Accessible.description: txtColor.text

            onCurrentColorChanged: Life.lifeColor = AkUtils.toRgba(currentColor)
        }
    }

    Label {
        id: lblThreshold
        text: qsTr("Threshold")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldThreshold
        value: Life.threshold
        to: 255
        stepSize: 1
        Layout.fillWidth: true
        Accessible.name: lblThreshold.text

        onValueChanged: Life.threshold = value
    }

    Label {
        id: txtLumaThreshold
        /*: Minimum luminance/light/white level/intensity in a gray or black and
            white picture.

            https://en.wikipedia.org/wiki/Luma_(video)
         */
        text: qsTr("Luma Threshold")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldLumaThreshold
        value: Life.lumaThreshold
        to: 255
        stepSize: 1
        Layout.fillWidth: true
        Accessible.name: txtLumaThreshold.text

        onValueChanged: Life.lumaThreshold = value
    }
}
