/* Webcamoid, webcam capture application.
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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import AkControls 1.0 as AK

GridLayout {
    columns: 3

    function fromRgba(rgba)
    {
        var a = ((rgba >> 24) & 0xff) / 255.0
        var r = ((rgba >> 16) & 0xff) / 255.0
        var g = ((rgba >> 8) & 0xff) / 255.0
        var b = (rgba & 0xff) / 255.0

        return Qt.rgba(r, g, b, a)
    }

    function toRgba(color)
    {
        var a = Math.round(255 * color.a) << 24
        var r = Math.round(255 * color.r) << 16
        var g = Math.round(255 * color.g) << 8
        var b = Math.round(255 * color.b)

        return a | r | g | b
    }

    Connections {
        target: Life

        onThresholdChanged: {
            sldThreshold.value = threshold
            spbThreshold.value = threshold
        }

        onLumaThresholdChanged: {
            sldLumaThreshold.value = lumaThreshold
            spbLumaThreshold.value = lumaThreshold
        }
    }

    Label {
        text: qsTr("Color")
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
        }
        AK.ColorButton {
            currentColor: fromRgba(Life.lifeColor)
            //: https://en.wikipedia.org/wiki/Life-like_cellular_automaton
            title: qsTr("Choose the automata color")
            showAlphaChannel: true

            onCurrentColorChanged: Life.lifeColor = toRgba(currentColor)
        }
    }

    Label {
        id: lblThreshold
        text: qsTr("Threshold")
    }
    Slider {
        id: sldThreshold
        value: Life.threshold
        to: 255
        stepSize: 1
        Layout.fillWidth: true

        onValueChanged: Life.threshold = value
    }
    SpinBox {
        id: spbThreshold
        value: Life.threshold
        to: sldThreshold.to
        stepSize: sldThreshold.stepSize
        editable: true

        onValueChanged: Life.threshold = value
    }

    Label {
        /*: Minimum luminance/light/white level/intensity in a gray or black and
            white picture.

            https://en.wikipedia.org/wiki/Luma_(video)
         */
        text: qsTr("Luma Threshold")
    }
    Slider {
        id: sldLumaThreshold
        value: Life.lumaThreshold
        to: 255
        stepSize: 1
        Layout.fillWidth: true

        onValueChanged: Life.lumaThreshold = value
    }
    SpinBox {
        id: spbLumaThreshold
        value: Life.lumaThreshold
        to: sldLumaThreshold.to
        stepSize: sldLumaThreshold.stepSize
        editable: true

        onValueChanged: Life.lumaThreshold = value
    }
}
