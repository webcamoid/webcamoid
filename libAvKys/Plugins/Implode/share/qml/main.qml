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

GridLayout {
    columns: 3

    Connections {
        target: Implode

        function onAmountChanged(amount)
        {
            sldAmount.value = amount
            spbAmount.value = amount * spbAmount.multiplier
        }
    }

    Label {
        id: txtAmount
        text: qsTr("Amount")
    }
    Slider {
        id: sldAmount
        value: Implode.amount
        stepSize: 0.01
        from: -10
        to: 10
        Layout.fillWidth: true
        Accessible.name: txtAmount.text

        onValueChanged: Implode.amount = value
    }
    SpinBox {
        id: spbAmount
        value: multiplier * Implode.amount
        from: multiplier * sldAmount.from
        to: multiplier * sldAmount.to
        stepSize: multiplier * sldAmount.stepSize
        editable: true
        Accessible.name: txtAmount.text

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbAmount.from, spbAmount.to)
            top:  Math.max(spbAmount.from, spbAmount.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Implode.amount = value / multiplier
    }
}
