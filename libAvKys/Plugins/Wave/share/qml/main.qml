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

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Dialogs 1.2
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
        target: Wave

        onAmplitudeChanged: {
            sldAmplitude.value = amplitude
            spbAmplitude.value = spbAmplitude.multiplier * amplitude
        }

        onFrequencyChanged: {
            sldFrequency.value = frequency
            spbFrequency.value = spbFrequency.multiplier * frequency
        }

        onPhaseChanged: {
            sldPhase.value = phase
            spbPhase.value = spbPhase.multiplier * phase
        }
    }

    Label {
        id: lblAmplitude
        text: qsTr("Amplitude")
    }

    Slider {
        id: sldAmplitude
        value: Wave.amplitude
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true

        onValueChanged: Wave.amplitude = value
    }
    SpinBox {
        id: spbAmplitude
        value: multiplier * Wave.amplitude
        to: multiplier * sldAmplitude.to
        stepSize: multiplier * sldAmplitude.stepSize
        editable: true

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            id: val
            bottom: Math.min(spbAmplitude.from, spbAmplitude.to)
            top:  Math.max(spbAmplitude.from, spbAmplitude.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Wave.amplitude = value / multiplier
    }

    Label {
        id: lblFrequency
        text: qsTr("Frequency")
    }
    Slider {
        id: sldFrequency
        value: Wave.frequency
        stepSize: 0.01
        to: 100
        Layout.fillWidth: true

        onValueChanged: Wave.frequency = value
    }
    SpinBox {
        id: spbFrequency
        value: multiplier * Wave.frequency
        to: multiplier * sldFrequency.to
        stepSize: multiplier * sldFrequency.stepSize
        editable: true

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbFrequency.from, spbFrequency.to)
            top:  Math.max(spbFrequency.from, spbFrequency.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Wave.frequency = value / multiplier
    }

    Label {
        id: lblPhase
        text: qsTr("Phase")
    }
    Slider {
        id: sldPhase
        value: Wave.phase
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true

        onValueChanged: Wave.phase = value
    }
    SpinBox {
        id: spbPhase
        value: multiplier * Wave.phase
        to: multiplier * sldPhase.to
        stepSize: multiplier * sldPhase.stepSize
        editable: true

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbPhase.from, spbPhase.to)
            top:  Math.max(spbPhase.from, spbPhase.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Wave.phase = value / multiplier
    }

    Label {
        text: qsTr("Background color")
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
        }
        AK.ColorButton {
            currentColor: fromRgba(Wave.background)
            title: qsTr("Choose the background color")

            onCurrentColorChanged: Wave.background = toRgba(currentColor)
        }
    }
}
