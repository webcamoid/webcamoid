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
    columns: 3

    Connections {
        target: Dizzy

        onSpeedChanged: {
            sldSpeed.value = speed
            spbSpeed.rvalue = speed
        }

        onZoomRateChanged: {
            sldZoomRate.value = zoomRate
            spbZoomRate.rvalue = zoomRate
        }

        onStrengthChanged: {
            sldStrength.value = strength
            spbStrength.rvalue = strength
        }
    }

    Label {
        id: lblSpeed
        text: qsTr("Speed")
    }
    Slider {
        id: sldSpeed
        value: Dizzy.speed
        stepSize: 0.01
        from: 0.01
        to: 60
        Layout.fillWidth: true

        onValueChanged: Dizzy.speed = value
    }
    AkSpinBox {
        id: spbSpeed
        decimals: 2
        rvalue: Dizzy.speed
        minimumValue: sldSpeed.from
        maximumValue: sldSpeed.to
        step: sldSpeed.stepSize

        onRvalueChanged: Dizzy.speed = rvalue
    }

    Label {
        id: lblZoomRate
        text: qsTr("Zoom rate")
    }
    Slider {
        id: sldZoomRate
        value: Dizzy.zoomRate
        stepSize: 0.001
        to: 0.25
        Layout.fillWidth: true

        onValueChanged: Dizzy.zoomRate = value
    }
    AkSpinBox {
        id: spbZoomRate
        decimals: 3
        rvalue: Dizzy.zoomRate
        maximumValue: sldZoomRate.to
        step: sldZoomRate.stepSize

        onRvalueChanged: Dizzy.zoomRate = rvalue
    }

    Label {
        id: lblStrength
        text: qsTr("Strength")
    }
    Slider {
        id: sldStrength
        value: Dizzy.strength
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true

        onValueChanged: Dizzy.strength = value
    }
    AkSpinBox {
        id: spbStrength
        decimals: 2
        rvalue: Dizzy.strength
        maximumValue: sldStrength.to
        step: sldStrength.stepSize

        onRvalueChanged: Dizzy.strength = rvalue
    }
}
