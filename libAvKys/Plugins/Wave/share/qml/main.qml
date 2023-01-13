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
        target: Wave

        function onAmplitudeXChanged(amplitudeX)
        {
            sldAmplitudeX.value = amplitudeX
            spbAmplitudeX.value = amplitudeX * spbAmplitudeX.multiplier
        }

        function onFrequencyXChanged(frequencyX)
        {
            sldFrequencyX.value = frequencyX
            spbFrequencyX.value = frequencyX * spbFrequencyX.multiplier
        }

        function onPhaseXChanged(phaseX)
        {
            sldPhaseX.value = phaseX
            spbPhaseX.value = spbPhaseX.multiplier * phaseX
        }

        function onAmplitudeYChanged(amplitudeY)
        {
            sldAmplitudeY.value = amplitudeY
            spbAmplitudeY.value = amplitudeY * spbAmplitudeY.multiplier
        }

        function onFrequencyYChanged(frequencyY)
        {
            sldFrequencyY.value = frequencyY
            spbFrequencyY.value = frequencyY * spbFrequencyY.multiplier
        }

        function onPhaseYChanged(phaseY)
        {
            sldPhaseY.value = phaseY
            spbPhaseY.value = spbPhaseY.multiplier * phaseY
        }
    }

    Label {
        text: qsTr("<b>X-Axis</b>")
        topPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        bottomPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        wrapMode: Text.Wrap
        Layout.columnSpan: 3
        Layout.fillWidth: true
    }
    Label {
        id: lblAmplitudeX
        text: qsTr("Amplitude (X)")
    }
    Slider {
        id: sldAmplitudeX
        value: Wave.amplitudeX
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblAmplitudeX.text

        onValueChanged: Wave.amplitudeX = value
    }
    SpinBox {
        id: spbAmplitudeX
        value: multiplier * Wave.amplitudeX
        to: multiplier * sldAmplitudeX.to
        stepSize: multiplier * sldAmplitudeX.stepSize
        editable: true
        Accessible.name: lblAmplitudeX.text

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbAmplitudeX.from, spbAmplitudeX.to)
            top:  Math.max(spbAmplitudeX.from, spbAmplitudeX.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Wave.amplitudeX = value / multiplier
    }
    Label {
        id: lblFrequencyX
        text: qsTr("Frequency (X)")
    }
    Slider {
        id: sldFrequencyX
        value: Wave.frequencyX
        stepSize: 0.01
        to: 100
        Layout.fillWidth: true
        Accessible.name: lblFrequencyX.text

        onValueChanged: Wave.frequencyX = value
    }
    SpinBox {
        id: spbFrequencyX
        value: multiplier * Wave.frequencyX
        to: multiplier * sldFrequencyX.to
        stepSize: multiplier * sldFrequencyX.stepSize
        editable: true
        Accessible.name: lblFrequencyX.text

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbFrequencyX.from, spbFrequencyX.to)
            top:  Math.max(spbFrequencyX.from, spbFrequencyX.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Wave.frequencyX = value / multiplier
    }
    Label {
        id: lblPhaseX
        text: qsTr("Phase (X)")
    }
    Slider {
        id: sldPhaseX
        value: Wave.phaseX
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblPhaseX.text

        onValueChanged: Wave.phaseX = value
    }
    SpinBox {
        id: spbPhaseX
        value: multiplier * Wave.phaseX
        to: multiplier * sldPhaseX.to
        stepSize: multiplier * sldPhaseX.stepSize
        editable: true
        Accessible.name: lblPhaseX.text

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbPhaseX.from, spbPhaseX.to)
            top:  Math.max(spbPhaseX.from, spbPhaseX.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Wave.phaseX = value / multiplier
    }
    Label {
        text: qsTr("<b>Y-Axis</b>")
        topPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        bottomPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        wrapMode: Text.Wrap
        Layout.columnSpan: 3
        Layout.fillWidth: true
    }
    Label {
        id: lblAmplitudeY
        text: qsTr("Amplitude (Y)")
    }
    Slider {
        id: sldAmplitudeY
        value: Wave.amplitudeY
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblAmplitudeY.text

        onValueChanged: Wave.amplitudeY = value
    }
    SpinBox {
        id: spbAmplitudeY
        value: multiplier * Wave.amplitudeY
        to: multiplier * sldAmplitudeY.to
        stepSize: multiplier * sldAmplitudeY.stepSize
        editable: true
        Accessible.name: lblAmplitudeY.text

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbAmplitudeY.from, spbAmplitudeY.to)
            top:  Math.max(spbAmplitudeY.from, spbAmplitudeY.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Wave.amplitudeY = value / multiplier
    }
    Label {
        id: lblFrequencyY
        text: qsTr("Frequency (Y)")
    }
    Slider {
        id: sldFrequencyY
        value: Wave.frequencyY
        stepSize: 0.01
        to: 100
        Layout.fillWidth: true
        Accessible.name: lblFrequencyY.text

        onValueChanged: Wave.frequencyY = value
    }
    SpinBox {
        id: spbFrequencyY
        value: multiplier * Wave.frequencyY
        to: multiplier * sldFrequencyY.to
        stepSize: multiplier * sldFrequencyY.stepSize
        editable: true
        Accessible.name: lblFrequencyY.text

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbFrequencyY.from, spbFrequencyY.to)
            top:  Math.max(spbFrequencyY.from, spbFrequencyY.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Wave.frequencyY = value / multiplier
    }
    Label {
        id: lblPhaseY
        text: qsTr("Phase (Y)")
    }
    Slider {
        id: sldPhaseY
        value: Wave.phaseY
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblPhaseY.text

        onValueChanged: Wave.phaseY = value
    }
    SpinBox {
        id: spbPhaseY
        value: multiplier * Wave.phaseY
        to: multiplier * sldPhaseY.to
        stepSize: multiplier * sldPhaseY.stepSize
        editable: true
        Accessible.name: lblPhaseY.text

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbPhaseY.from, spbPhaseY.to)
            top:  Math.max(spbPhaseY.from, spbPhaseY.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Wave.phaseY = value / multiplier
    }
}
