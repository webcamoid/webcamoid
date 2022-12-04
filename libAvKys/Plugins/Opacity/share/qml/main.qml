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

GridLayout {
    columns: 3

    Connections {
        target: Opacity

        function onOpacityChanged(opacity)
        {
            sldOpacity.value = opacity
            spbOpacity.value = opacity * spbOpacity.multiplier
        }
    }

    Label {
        id: lblOpacity
        text: qsTr("Opacity")
    }
    Slider {
        id: sldOpacity
        value: Opacity.opacity
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblOpacity.text

        onValueChanged: Opacity.opacity = value
    }
    SpinBox {
        id: spbOpacity
        value: multiplier * Opacity.opacity
        to: multiplier * sldOpacity.to
        stepSize: multiplier * sldOpacity.stepSize
        editable: true
        Accessible.name: lblOpacity.text

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbOpacity.from, spbOpacity.to)
            top:  Math.max(spbOpacity.from, spbOpacity.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Opacity.opacity = value / multiplier
    }
}
