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
import RippleElement 1.0

GridLayout {
    columns: 3

    function modeIndex(mode)
    {
        var index = -1

        for (var i = 0; i < cbxMode.model.count; i++)
            if (cbxMode.model.get(i).mode == mode) {
                index = i
                break
            }

        return index
    }

    Connections {
        target: Ripple

        function onAmplitudeChanged(amplitude)
        {
            sldAmplitude.value = amplitude
            spbAmplitude.value = amplitude
        }

        function onDecayChanged(decay)
        {
            sldDecay.value = decay
            spbDecay.value = decay
        }

        function onThresholdChanged(threshold)
        {
            sldThreshold.value = threshold
            spbThreshold.value = threshold
        }

        function onLumaThresholdChanged(lumaThreshold)
        {
            sldLumaThreshold.value = lumaThreshold
            spbLumaThreshold.value = lumaThreshold
        }

        function onMinDropSizeChanged(minDropSize)
        {
            sldMinDropSize.value = minDropSize
            spbMinDropSize.value = minDropSize
        }

        function onMaxDropSizeChanged(maxDropSize)
        {
            sldMaxDropSize.value = maxDropSize
            spbMaxDropSize.value = maxDropSize
        }

        function onDropSigmaChanged(dropSigma)
        {
            sldDropSigma.value = dropSigma
            spbDropSigma.value = dropSigma * spbDropSigma.multiplier
        }

        function onDropProbabilityChanged(dropProbability)
        {
            sldDropProbability.value = dropProbability
            spbDropProbability.value = dropProbability * spbDropProbability.multiplier
        }
    }

    Label {
        id: txtMode
        text: qsTr("Mode")
    }
    ComboBox {
        id: cbxMode
        textRole: "text"
        currentIndex: modeIndex(Ripple.mode)
        Layout.columnSpan: 2
        Layout.fillWidth: true
        Accessible.description: txtMode.text

        model: ListModel {
            ListElement {
                text: qsTr("Motion detect")
                mode: RippleElement.RippleModeMotionDetect
            }
            ListElement {
                text: qsTr("Rain")
                mode: RippleElement.RippleModeRain
            }
        }

        onCurrentIndexChanged: Ripple.mode = cbxMode.model.get(currentIndex).mode
    }
    Label {
        text: qsTr("<b>General parameters</b>")
        topPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        bottomPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        wrapMode: Text.Wrap
        Layout.columnSpan: 3
        Layout.fillWidth: true
    }
    Label {
        id: txtAmplitude
        text: qsTr("Amplitude")
    }
    Slider {
        id: sldAmplitude
        value: Ripple.amplitude
        stepSize: 1
        to: 1024
        Layout.fillWidth: true
        Accessible.name: txtAmplitude.text

        onValueChanged: Ripple.amplitude = value
    }
    SpinBox {
        id: spbAmplitude
        value: Ripple.amplitude
        to: sldAmplitude.to
        stepSize: sldAmplitude.stepSize
        editable: true
        Accessible.name: txtAmplitude.text

        onValueChanged: Ripple.amplitude = Number(value)
    }
    Label {
        id: txtDecay
        text: qsTr("Decay")
    }
    Slider {
        id: sldDecay
        value: Ripple.decay
        stepSize: 1
        to: 8
        Layout.fillWidth: true
        Accessible.name: txtDecay.text

        onValueChanged: Ripple.decay = value
    }
    SpinBox {
        id: spbDecay
        value: Ripple.decay
        to: sldDecay.to
        stepSize: sldDecay.stepSize
        editable: true
        Accessible.name: txtDecay.text

        onValueChanged: Ripple.decay = Number(value)
    }
    Label {
        text: qsTr("<b>Motion detection parameters</b>")
        topPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        bottomPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        wrapMode: Text.Wrap
        Layout.columnSpan: 3
        Layout.fillWidth: true
    }
    Label {
        id: txtThreshold
        text: qsTr("Threshold")
    }
    Slider {
        id: sldThreshold
        value: Ripple.threshold
        stepSize: 1
        to: 256
        Layout.fillWidth: true
        Accessible.name: txtThreshold.text

        onValueChanged: Ripple.threshold = value
    }
    SpinBox {
        id: spbThreshold
        value: Ripple.threshold
        to: sldThreshold.to
        stepSize: sldThreshold.stepSize
        editable: true
        Accessible.name: txtThreshold.text

        onValueChanged: Ripple.threshold = Number(value)
    }
    Label {
        id: txtLumaThreshold
        /*: Minimum luminance/light/white level/intensity in a gray or black and
            white picture.

            https://en.wikipedia.org/wiki/Luma_(video)
         */
        text: qsTr("Luma threshold")
    }
    Slider {
        id: sldLumaThreshold
        value: Ripple.lumaThreshold
        stepSize: 1
        to: 256
        Layout.fillWidth: true
        Accessible.name: txtLumaThreshold.text

        onValueChanged: Ripple.lumaThreshold = value
    }
    SpinBox {
        id: spbLumaThreshold
        value: Ripple.lumaThreshold
        to: sldLumaThreshold.to
        stepSize: sldLumaThreshold.stepSize
        editable: true
        Accessible.name: txtLumaThreshold.text

        onValueChanged: Ripple.lumaThreshold = Number(value)
    }
    Label {
        text: qsTr("<b>Rain parameters</b>")
        topPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        bottomPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        wrapMode: Text.Wrap
        Layout.columnSpan: 3
        Layout.fillWidth: true
    }
    Label {
        id: txtMinDropSize

        text: qsTr("Minimum drop size")
    }
    Slider {
        id: sldMinDropSize
        value: Ripple.minDropSize
        stepSize: 1
        to: 1024
        Layout.fillWidth: true
        Accessible.name: txtMinDropSize.text

        onValueChanged: Ripple.minDropSize = value
    }
    SpinBox {
        id: spbMinDropSize
        value: Ripple.minDropSize
        to: sldMinDropSize.to
        stepSize: sldMinDropSize.stepSize
        editable: true
        Accessible.name: txtMinDropSize.text

        onValueChanged: Ripple.minDropSize = Number(value)
    }
    Label {
        id: txtMaxDropSize

        text: qsTr("Maximum drop size")
    }
    Slider {
        id: sldMaxDropSize
        value: Ripple.maxDropSize
        stepSize: 1
        to: 1024
        Layout.fillWidth: true
        Accessible.name: txtMaxDropSize.text

        onValueChanged: Ripple.maxDropSize = value
    }
    SpinBox {
        id: spbMaxDropSize
        value: Ripple.maxDropSize
        to: sldMaxDropSize.to
        stepSize: sldMaxDropSize.stepSize
        editable: true
        Accessible.name: txtMaxDropSize.text

        onValueChanged: Ripple.maxDropSize = Number(value)
    }
    Label {
        id: lblDropSigma
        text: qsTr("Drop thickness")
    }
    Slider {
        id: sldDropSigma
        value: Ripple.dropSigma
        stepSize: 0.01
        from: 0
        to: 10
        Layout.fillWidth: true
        Accessible.name: lblDropSigma.text

        onValueChanged: Ripple.dropSigma = value
    }
    SpinBox {
        id: spbDropSigma
        value: multiplier * Ripple.dropSigma
        from: multiplier * sldDropSigma.from
        to: multiplier * sldDropSigma.to
        stepSize: multiplier * sldDropSigma.stepSize
        editable: true
        Accessible.name: lblDropSigma.text

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbDropSigma.from, spbDropSigma.to)
            top:  Math.max(spbDropSigma.from, spbDropSigma.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Ripple.dropSigma = value / multiplier
    }
    Label {
        id: lblDropProbability
        text: qsTr("Drop frequency")
    }
    Slider {
        id: sldDropProbability
        value: Ripple.dropProbability
        stepSize: 0.01
        from: 0
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblDropProbability.text

        onValueChanged: Ripple.dropProbability = value
    }
    SpinBox {
        id: spbDropProbability
        value: multiplier * Ripple.dropProbability
        from: multiplier * sldDropProbability.from
        to: multiplier * sldDropProbability.to
        stepSize: multiplier * sldDropProbability.stepSize
        editable: true
        Accessible.name: lblDropProbability.text

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbDropProbability.from, spbDropProbability.to)
            top:  Math.max(spbDropProbability.from, spbDropProbability.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Ripple.dropProbability = value / multiplier
    }
}
