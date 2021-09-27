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
        target: Scroll

        function onSpeedChanged(speed)
        {
            sldSpeed.value = speed
            spbSpeed.value = spbSpeed.multiplier * speed
        }

        function onNoiseChanged(noise)
        {
            sldNoise.value = noise
            spbNoise.value = spbNoise.multiplier * noise
        }
    }

    Label {
        id: lblSpeed
        text: qsTr("Vertical Sync")
    }
    Slider {
        id: sldSpeed
        value: Scroll.speed
        stepSize: 0.01
        from: -1
        to: 1
        Layout.fillWidth: true

        onValueChanged: Scroll.speed = value
    }
    SpinBox {
        id: spbSpeed
        value: multiplier * Scroll.speed
        from: multiplier * sldSpeed.from
        to: multiplier * sldSpeed.to
        stepSize: multiplier * sldSpeed.stepSize
        editable: true

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
        onValueModified: Scroll.speed = value / multiplier
    }

    Label {
        id: lblNoise
        text: qsTr("Noise")
    }
    Slider {
        id: sldNoise
        value: Scroll.noise
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true

        onValueChanged: Scroll.noise = value
    }
    SpinBox {
        id: spbNoise
        value: multiplier * Scroll.noise
        to: multiplier * sldNoise.to
        stepSize: multiplier * sldNoise.stepSize
        editable: true

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbNoise.from, spbNoise.to)
            top:  Math.max(spbNoise.from, spbNoise.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Scroll.noise = value / multiplier
    }
}
