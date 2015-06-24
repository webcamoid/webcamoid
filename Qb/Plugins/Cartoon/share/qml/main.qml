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

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

GridLayout {
    columns: 3

    // Configure threshold.
    Label {
        id: lblThreshold
        text: qsTr("Threshold")
    }
    Slider {
        id: sldThreshold
        value: Cartoon.threshold
        stepSize: 1
        maximumValue: 256

        onValueChanged: Cartoon.threshold = value
    }
    SpinBox {
        id: spbThreshold
        value: sldThreshold.value
        maximumValue: sldThreshold.maximumValue
        stepSize: sldThreshold.stepSize

        onValueChanged: sldThreshold.value = value
    }

    // Configure distance.
    Label {
        id: lblDistance
        text: qsTr("Distance")
    }
    Slider {
        id: sldDistance
        value: Cartoon.diffSpace
        stepSize: 1
        maximumValue: 1024

        onValueChanged: Cartoon.diffSpace = value
    }
    SpinBox {
        id: spbDistance
        value: sldDistance.value
        maximumValue: sldDistance.maximumValue
        stepSize: sldDistance.stepSize

        onValueChanged: sldDistance.value = value
    }
}
