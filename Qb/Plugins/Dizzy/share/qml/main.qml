/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1

GridLayout {
    columns: 3

    Label {
        id: lblSpeed
        text: qsTr("Speed")
    }
    Slider {
        id: sldSpeed
        value: Dizzy.speed
        stepSize: 0.01
        minimumValue: 0.01
        maximumValue: 60

        onValueChanged: Dizzy.speed = value
    }
    SpinBox {
        id: spbSpeed
        decimals: 2
        value: sldSpeed.value
        minimumValue: sldSpeed.minimumValue
        maximumValue: sldSpeed.maximumValue
        stepSize: sldSpeed.stepSize

        onValueChanged: sldSpeed.value = value
    }

    Label {
        id: lblZoomRate
        text: qsTr("Zoom rate")
    }
    Slider {
        id: sldZoomRate
        value: Dizzy.zoomRate
        stepSize: 0.001
        maximumValue: 0.25

        onValueChanged: Dizzy.zoomRate = value
    }
    SpinBox {
        id: spbZoomRate
        decimals: 3
        value: sldZoomRate.value
        maximumValue: sldZoomRate.maximumValue
        stepSize: sldZoomRate.stepSize

        onValueChanged: sldZoomRate.value = value
    }

    Label {
        id: lblStrength
        text: qsTr("Strength")
    }
    Slider {
        id: sldStrength
        value: Dizzy.strength
        stepSize: 0.01
        maximumValue: 1

        onValueChanged: Dizzy.strength = value
    }
    SpinBox {
        id: spbStrength
        decimals: 2
        value: sldStrength.value
        maximumValue: sldStrength.maximumValue
        stepSize: sldStrength.stepSize

        onValueChanged: sldStrength.value = value
    }
}
