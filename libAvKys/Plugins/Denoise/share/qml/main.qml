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
    id: configs
    columns: 3

    Connections {
        target: Denoise

        function onRadiusChanged(radius)
        {
            sldRadius.value = radius
            spbRadius.value = radius
        }

        function onSigmaChanged(sigma)
        {
            sldSigma.value = sigma
            spbSigma.value = sigma * spbSigma.multiplier
        }
    }

    Label {
        id: txtRadius
        text: qsTr("Radius")
    }
    Slider {
        id: sldRadius
        value: Denoise.radius
        stepSize: 1
        to: 10
        Layout.fillWidth: true
        Accessible.name: txtRadius.text

        onValueChanged: Denoise.radius = value
    }
    SpinBox {
        id: spbRadius
        value: Denoise.radius
        to: sldRadius.to
        stepSize: sldRadius.stepSize
        editable: true
        Accessible.name: txtRadius.text

        onValueChanged: Denoise.radius = Number(value)
    }

    Label {
        id: txtFactor
        text: qsTr("Factor")
    }
    TextField {
        text: Denoise.factor
        placeholderText: qsTr("Factor")
        validator: RegExpValidator {
            regExp: /-?\d+/
        }
        Layout.columnSpan: 2
        Layout.fillWidth: true
        Accessible.name: txtFactor.text

        onTextChanged: Denoise.factor = Number(text)
    }

    Label {
        id: txtMu
        /*: Mu factor (µ letter from greek), represents the average of a group
            of values.

            https://en.wikipedia.org/wiki/Arithmetic_mean
         */
        text: qsTr("Mu")
    }
    TextField {
        text: Denoise.mu
        placeholderText: txtMu.text
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /-?\d+/
        }
        Layout.columnSpan: 2
        Layout.fillWidth: true
        Accessible.name: txtMu.text

        onTextChanged: Denoise.mu = Number(text)
    }

    Label {
        id: txtSigma
        /*: Sigma factor (σ letter from greek), represents the standard
            deviation of a group of values.

            https://en.wikipedia.org/wiki/Standard_deviation
         */
        text: qsTr("Sigma")
    }
    Slider {
        id: sldSigma
        value: Denoise.sigma
        stepSize: 0.1
        from: 0.1
        to: 10
        Layout.fillWidth: true
        Accessible.name: txtSigma.text

        onValueChanged: Denoise.sigma = value
    }
    SpinBox {
        id: spbSigma
        value: multiplier * Denoise.sigma
        from: multiplier * sldSigma.from
        to: multiplier * sldSigma.to
        stepSize: multiplier * sldSigma.stepSize
        editable: true
        Accessible.name: txtSigma.text

        readonly property int decimals: 1
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbSigma.from, spbSigma.to)
            top:  Math.max(spbSigma.from, spbSigma.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Denoise.sigma = value / multiplier
    }
}
