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
        target: Warp

        function onRipplesChanged(ripples)
        {
            sldRipples.value = ripples
            spbRipples.value = ripples * spbRipples.multiplier
        }

        function onDurationChanged(duration)
        {
            sldDuration.value = duration
            spbDuration.value = duration
        }
    }

    Label {
        id: txtRipples
        text: qsTr("Ripples")
    }
    Slider {
        id: sldRipples
        value: Warp.ripples
        stepSize: 0.1
        to: 256
        Layout.fillWidth: true
        Accessible.name: txtRipples.text

        onValueChanged: Warp.ripples = value
    }
    SpinBox {
        id: spbRipples
        value: multiplier * Warp.ripples
        to: multiplier * sldRipples.to
        stepSize: multiplier * sldRipples.stepSize
        editable: true
        Accessible.name: txtRipples.text

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbRipples.from, spbRipples.to)
            top:  Math.max(spbRipples.from, spbRipples.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Warp.ripples = value / multiplier
    }
    Label {
        id: txtDuration
        text: qsTr("Duration (in seconds)")
    }
    Slider {
        id: sldDuration
        value: Warp.duration
        stepSize: 1
        to: 128
        Layout.fillWidth: true
        Accessible.name: txtDuration.text

        onValueChanged: Warp.duration = value
    }
    SpinBox {
        id: spbDuration
        value: Warp.duration
        to: sldDuration.to
        stepSize: sldDuration.stepSize
        editable: true
        Accessible.name: txtDuration.text

        onValueChanged: Warp.duration = Number(value)
    }
}
