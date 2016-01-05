/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
        text: qsTr("Degrees")
    }
    Slider {
        id: sldDegrees
        value: Swirl.degrees
        stepSize: 1
        minimumValue: -360
        maximumValue: 360

        onValueChanged: Swirl.degrees = value
    }
    SpinBox {
        value: sldDegrees.value
        maximumValue: sldDegrees.maximumValue
        stepSize: sldDegrees.stepSize

        onValueChanged: sldDegrees.value = value
    }
}
