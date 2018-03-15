/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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
    columns: 3

    Connections {
        target: Temperature

        onTemperatureChanged: {
            sldTemperature.value = temperature
            spbTemperature.rvalue = temperature
        }
    }

    Label {
        text: qsTr("Temperature")
    }
    Slider {
        id: sldTemperature
        value: Temperature.temperature
        stepSize: 1
        from: 1000
        to: 40000
        Layout.fillWidth: true

        onValueChanged: Temperature.temperature = value
    }
    AkSpinBox {
        id: spbTemperature
        rvalue: Temperature.temperature
        minimumValue: sldTemperature.from
        maximumValue: sldTemperature.to
        step: sldTemperature.stepSize

        onRvalueChanged: Temperature.temperature = rvalue
    }
}
