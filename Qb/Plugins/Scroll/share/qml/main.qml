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
        text: qsTr("Vertical Sync")
    }
    Slider {
        id: sldSpeed
        value: Scroll.speed
        stepSize: 0.01
        minimumValue: -1
        maximumValue: 1

        onValueChanged: Scroll.speed = value
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
        id: lblNoise
        text: qsTr("Noise")
    }
    Slider {
        id: sldNoise
        value: Scroll.noise
        stepSize: 0.01
        maximumValue: 1

        onValueChanged: Scroll.noise = value
    }
    SpinBox {
        id: spbNoise
        decimals: 2
        value: sldNoise.value
        maximumValue: sldNoise.maximumValue
        stepSize: sldNoise.stepSize

        onValueChanged: sldNoise.value = value
    }
}
