/* Webcamoid, camera capture application.
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: configs

    Label {
        id: txtRadius
        text: qsTr("Radius")
        font.bold: true
        Layout.fillWidth: true
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

    Label {
        id: txtFactor
        text: qsTr("Factor")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        text: Denoise.factor
        placeholderText: qsTr("Factor")
        validator: RegularExpressionValidator {
            regularExpression: /-?\d+/
        }
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
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        text: Denoise.mu
        placeholderText: txtMu.text
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /-?\d+/
        }
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
        font.bold: true
        Layout.fillWidth: true
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
}
