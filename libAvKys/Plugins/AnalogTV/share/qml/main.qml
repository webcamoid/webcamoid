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
        target: AnalogTV

        function onVsyncChanged(vsync)
        {
            sldVSync.value = vsync
            spbVSync.value = vsync * spbVSync.multiplier
        }

        function onXOffsetChanged(xOffset)
        {
            sldXOffset.value = xOffset
            spbXOffset.value = xOffset
        }

        function onHsyncFactorChanged(hsyncFactor)
        {
            sldHSyncFactor.value = hsyncFactor
            spbHSyncFactor.value = hsyncFactor * spbHSyncFactor.multiplier
        }

        function onHsyncSmoothnessChanged(hsyncSmoothness)
        {
            sldHSyncSmoothness.value = hsyncSmoothness
            spbHSyncSmoothness.value = hsyncSmoothness
        }

        function onHueFactorChanged(hueFactor)
        {
            sldHueFactor.value = hueFactor
            spbHueFactor.value = hueFactor * spbHueFactor.multiplier
        }

        function onNoiseChanged(noise)
        {
            sldNoise.value = noise
            spbNoise.value = noise * spbNoise.multiplier
        }
    }

    Label {
        id: lblVSync
        text: qsTr("Vertical sync")
    }
    Slider {
        id: sldVSync
        value: AnalogTV.vsync
        stepSize: 0.01
        from: -1
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblVSync.text

        onValueChanged: AnalogTV.vsync = value
    }
    SpinBox {
        id: spbVSync
        value: multiplier * AnalogTV.vsync
        from: multiplier * sldVSync.from
        to: multiplier * sldVSync.to
        stepSize: multiplier * sldVSync.stepSize
        editable: true
        Accessible.name: lblVSync.text

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbVSync.from, spbVSync.to)
            top:  Math.max(spbVSync.from, spbVSync.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: AnalogTV.vsync = value / multiplier
    }
    Label {
        id: lblXOffset
        text: qsTr("Horizontal offset")
    }
    Slider {
        id: sldXOffset
        value: AnalogTV.xOffset
        stepSize: 1
        from: -1024
        to: 1024
        Layout.fillWidth: true
        Accessible.name: lblXOffset.text

        onValueChanged: AnalogTV.xOffset = value
    }
    SpinBox {
        id: spbXOffset
        value: AnalogTV.xOffset
        from: sldXOffset.from
        to: sldXOffset.to
        stepSize: sldXOffset.stepSize
        editable: true
        Accessible.name: lblXOffset.text

        onValueChanged: AnalogTV.xOffset = Number(value)
    }
    Label {
        id: lblHSyncFactor
        text: qsTr("Horizontcal sync factor")
    }
    Slider {
        id: sldHSyncFactor
        value: AnalogTV.hsyncFactor
        stepSize: 0.01
        from: -20
        to: 20
        Layout.fillWidth: true
        Accessible.name: lblHSyncFactor.text

        onValueChanged: AnalogTV.hsyncFactor = value
    }
    SpinBox {
        id: spbHSyncFactor
        value: multiplier * AnalogTV.hsyncFactor
        from: multiplier * sldHSyncFactor.from
        to: multiplier * sldHSyncFactor.to
        stepSize: multiplier * sldHSyncFactor.stepSize
        editable: true
        Accessible.name: lblHSyncFactor.text

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbHSyncFactor.from, spbHSyncFactor.to)
            top:  Math.max(spbHSyncFactor.from, spbHSyncFactor.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: AnalogTV.hsyncFactor = value / multiplier
    }
    Label {
        id: lblHSyncSmoothness
        text: qsTr("Horizontcal sync smoothness")
    }
    Slider {
        id: sldHSyncSmoothness
        value: AnalogTV.hsyncSmoothness
        stepSize: 1
        from: 0
        to: 1024
        Layout.fillWidth: true
        Accessible.name: lblHSyncSmoothness.text

        onValueChanged: AnalogTV.hsyncSmoothness = value
    }
    SpinBox {
        id: spbHSyncSmoothness
        value: AnalogTV.hsyncSmoothness
        from: sldHSyncSmoothness.from
        to: sldHSyncSmoothness.to
        stepSize: sldHSyncSmoothness.stepSize
        editable: true
        Accessible.name: lblHSyncSmoothness.text

        onValueChanged: AnalogTV.hsyncSmoothness = Number(value)
    }
    Label {
        id: lblHueFactor
        text: qsTr("Hue dephasing factor")
    }
    Slider {
        id: sldHueFactor
        value: AnalogTV.hueFactor
        stepSize: 0.01
        from: -2
        to: 2
        Layout.fillWidth: true
        Accessible.name: lblHueFactor.text

        onValueChanged: AnalogTV.hueFactor = value
    }
    SpinBox {
        id: spbHueFactor
        value: multiplier * AnalogTV.hueFactor
        from: multiplier * sldHueFactor.from
        to: multiplier * sldHueFactor.to
        stepSize: multiplier * sldHueFactor.stepSize
        editable: true
        Accessible.name: lblHueFactor.text

        readonly property int decimals: 2
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbHueFactor.from, spbHueFactor.to)
            top:  Math.max(spbHueFactor.from, spbHueFactor.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: AnalogTV.hueFactor = value / multiplier
    }
    Label {
        id: lblNoise
        text: qsTr("Noise")
    }
    Slider {
        id: sldNoise
        value: AnalogTV.noise
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblNoise.text

        onValueChanged: AnalogTV.noise = value
    }
    SpinBox {
        id: spbNoise
        value: multiplier * AnalogTV.noise
        to: multiplier * sldNoise.to
        stepSize: multiplier * sldNoise.stepSize
        editable: true
        Accessible.name: lblNoise.text

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
        onValueModified: AnalogTV.noise = value / multiplier
    }
}
