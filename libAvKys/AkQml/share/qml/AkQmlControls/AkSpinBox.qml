/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

import QtQuick 2.7
import QtQuick.Controls 2.0

SpinBox {
    id: spinbox
    value: Math.round((rvalue * Math.pow(10, decimals)))
    from: Math.round((minimumValue * Math.pow(10, decimals)))
    to: Math.round((maximumValue * Math.pow(10, decimals)))
    stepSize: Math.round((step * Math.pow(10, decimals)))

    property real rvalue: 0
    property real minimumValue: 0
    property real maximumValue: 99
    property real step: 1
    property int decimals: 0

    validator: DoubleValidator {
        bottom: Math.min(spinbox.from, spinbox.to)
        top:  Math.max(spinbox.from, spinbox.to)
    }

    textFromValue: function(value, locale) {
        return Number(value / Math.pow(10, spinbox.decimals)).toLocaleString(locale, 'f', spinbox.decimals)
    }

    valueFromText: function(text, locale) {
        spinbox.rvalue = Number.fromLocaleString(locale, text)

        return spinbox.value
    }

    onValueChanged: spinbox.rvalue = value / Math.pow(10, spinbox.decimals)
}
