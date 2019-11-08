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
import QtQuick.Layouts 1.3
import AkQmlControls 1.0

GridLayout {
    id: configs
    columns: 3

    Connections {
        target: Denoise

        onRadiusChanged: {
            sldRadius.value = radius
            spbRadius.rvalue = radius
        }
        onSigmaChanged: {
            sldSigma.value = sigma
            spbSigma.rvalue = sigma
        }
    }

    Label {
        text: qsTr("Radius")
    }
    Slider {
        id: sldRadius
        value: Denoise.radius
        stepSize: 1
        to: 10
        Layout.fillWidth: true

        onValueChanged: Denoise.radius = value
    }
    AkSpinBox {
        id: spbRadius
        rvalue: Denoise.radius
        maximumValue: sldRadius.to
        step: sldRadius.stepSize

        onRvalueChanged: Denoise.radius = rvalue
    }

    TextField {
        text: Denoise.factor
        placeholderText: qsTr("Factor")
        validator: RegExpValidator {
            regExp: /-?\d+/
        }
        Layout.columnSpan: 3
        Layout.fillWidth: true

        onTextChanged: Denoise.factor = Number(text)
    }

    TextField {
        text: Denoise.mu
        placeholderText: qsTr("Mu")
        validator: RegExpValidator {
            regExp: /-?\d+/
        }
        Layout.columnSpan: 3
        Layout.fillWidth: true

        onTextChanged: Denoise.mu = Number(text)
    }

    Label {
        text: qsTr("Sigma")
    }
    Slider {
        id: sldSigma
        value: Denoise.sigma
        stepSize: 0.1
        to: 10
        from: 0.1
        Layout.fillWidth: true

        onValueChanged: Denoise.sigma = value
    }
    AkSpinBox {
        id: spbSigma
        rvalue: Denoise.sigma
        decimals: 1
        step: sldSigma.stepSize
        minimumValue: sldSigma.from
        maximumValue: sldSigma.to

        onRvalueChanged: Denoise.sigma = rvalue
    }
}
