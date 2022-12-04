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
        target: Dizzy

        function onSpeedChanged(speed)
        {
            sldSpeed.value = speed
            spbSpeed.value = speed * spbSpeed.multiplier
        }

        function onZoomRateChanged(zoomRate)
        {
            sldZoomRate.value = zoomRate
            spbZoomRate.value = zoomRate * spbZoomRate.multiplier
        }

        function onStrengthChanged(strength)
        {
            sldStrength.value = strength
            spbStrength.value = strength * spbStrength.multiplier
        }
    }

    Label {
        id: lblSpeed
        text: qsTr("Speed")
    }
    Slider {
        id: sldSpeed
        value: Dizzy.speed
        stepSize: 0.01
        from: 0.01
        to: 60
        Layout.fillWidth: true
        Accessible.name: lblSpeed.text

        onValueChanged: Dizzy.speed = value
    }
    SpinBox {
        id: spbSpeed
        value: multiplier * Dizzy.speed
        from: multiplier * sldSpeed.from
        to: multiplier * sldSpeed.to
        stepSize: multiplier * sldSpeed.stepSize
        editable: true
        Accessible.name: lblSpeed.text

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbSpeed.from, spbSpeed.to)
            top:  Math.max(spbSpeed.from, spbSpeed.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Dizzy.speed = value / multiplier
    }

    Label {
        id: lblZoomRate
        text: qsTr("Zoom rate")
    }
    Slider {
        id: sldZoomRate
        value: Dizzy.zoomRate
        stepSize: 0.001
        to: 0.25
        Layout.fillWidth: true
        Accessible.name: lblZoomRate.text

        onValueChanged: Dizzy.zoomRate = value
    }
    SpinBox {
        id: spbZoomRate
        value: multiplier * Dizzy.zoomRate
        to: multiplier * sldZoomRate.to
        stepSize: multiplier * sldZoomRate.stepSize
        editable: true
        Accessible.name: lblZoomRate.text

        readonly property int decimals: 3
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbZoomRate.from, spbZoomRate.to)
            top:  Math.max(spbZoomRate.from, spbZoomRate.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Dizzy.zoomRate = value / multiplier
    }

    Label {
        id: lblStrength
        text: qsTr("Strength")
    }
    Slider {
        id: sldStrength
        value: Dizzy.strength
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblStrength.text

        onValueChanged: Dizzy.strength = value
    }
    SpinBox {
        id: spbStrength
        value: multiplier * Dizzy.strength
        to: multiplier * sldStrength.to
        stepSize: multiplier * sldStrength.stepSize
        editable: true
        Accessible.name: lblStrength.text

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbStrength.from, spbStrength.to)
            top:  Math.max(spbStrength.from, spbStrength.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Dizzy.strength = value / multiplier
    }
}
