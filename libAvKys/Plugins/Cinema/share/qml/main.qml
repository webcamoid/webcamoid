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
import Ak 1.0
import AkControls 1.0 as AK

GridLayout {
    columns: 3

    Connections {
        target: Cinema

        function onStripSizeChanged(stripSize)
        {
            sldStripSize.value = stripSize
            spbStripSize.value = stripSize * spbStripSize.multiplier
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
        Accessible.name: lblStripSize.text

        onValueChanged: Cinema.stripSize = value
    }
    SpinBox {
        id: spbStripSize
        value: multiplier * Cinema.stripSize
        to: multiplier * sldStripSize.to
        stepSize: multiplier * sldStripSize.stepSize
        editable: true
        Accessible.name: lblStripSize.text

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
        id: txtColor
        text: qsTr("Color")
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
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
