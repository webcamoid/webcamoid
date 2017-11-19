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
import "qrc:/Ak/share/qml/AkQmlControls"

GridLayout {
    columns: 3

    Connections {
        target: Scroll

        onSpeedChanged: {
            sldSpeed.value = speed
            spbSpeed.rvalue = speed
        }

        onNoiseChanged: {
            sldNoise.value = noise
            spbNoise.rvalue = noise
        }
    }

    Label {
        id: lblSpeed
        text: qsTr("Vertical Sync")
    }
    Slider {
        id: sldSpeed
        value: Scroll.speed
        stepSize: 0.01
        from: -1
        to: 1
        Layout.fillWidth: true

        onValueChanged: Scroll.speed = value
    }
    AkSpinBox {
        id: spbSpeed
        decimals: 2
        rvalue: Scroll.speed
        minimumValue: sldSpeed.from
        maximumValue: sldSpeed.to
        step: sldSpeed.stepSize

        onRvalueChanged: Scroll.speed = rvalue
    }

    Label {
        id: lblNoise
        text: qsTr("Noise")
    }
    Slider {
        id: sldNoise
        value: Scroll.noise
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true

        onValueChanged: Scroll.noise = value
    }
    AkSpinBox {
        id: spbNoise
        decimals: 2
        rvalue: Scroll.noise
        maximumValue: sldNoise.to
        step: sldNoise.stepSize

        onRvalueChanged: Scroll.noise = rvalue
    }
}
