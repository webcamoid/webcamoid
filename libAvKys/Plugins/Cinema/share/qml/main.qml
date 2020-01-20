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
        target: Cinema

        onStripSizeChanged: {
            sldStripSize.value = stripSize
            spbStripSize.value = spbStripSize.multiplier * stripSize
        }
    }

    // Configure strip size.
    Label {
        id: lblStripSize
        text: qsTr("Size")
    }
    Slider {
        id: sldStripSize
        value: Cinema.stripSize
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true

        onValueChanged: Cinema.stripSize = value
    }
    SpinBox {
        id: spbStripSize
        value: multiplier * Cinema.stripSize
        to: multiplier * sldStripSize.to
        stepSize: multiplier * sldStripSize.stepSize
        editable: true

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbStripSize.from, spbStripSize.to)
            top:  Math.max(spbStripSize.from, spbStripSize.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Cinema.stripSize = value / multiplier
    }

    // Configure strip color.
    Label {
        text: qsTr("Color")
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
        }
        AK.ColorButton {
            currentColor: fromRgba(Cinema.stripColor)
            title: qsTr("Choose the strips color")
            showAlphaChannel: true

            onCurrentColorChanged: Cinema.stripColor = toRgba(currentColor)
        }
    }
}
