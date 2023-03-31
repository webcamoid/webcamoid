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
        target: Saturated

        function onFactor(factor)
        {
            sldFactor.value = factor
            spbFactor.value = factor
        }
    }

    Label {
        id: txtFactor
        text: qsTr("Factor")
    }
    Slider {
        id: sldFactor
        value: Saturated.factor
        stepSize: 1
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtFactor.text

        onValueChanged: Saturated.factor = value
    }
    SpinBox {
        id: spbFactor
        value: Saturated.factor
        to: sldFactor.to
        stepSize: sldFactor.stepSize
        editable: true
        Accessible.name: txtFactor.text

        onValueChanged: Saturated.factor = Number(value)
    }
}
