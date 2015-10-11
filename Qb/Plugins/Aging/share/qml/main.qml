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

    // Configure the number of scratches to show.
    Label {
        id: lblNScratches
        text: qsTr("Number of scratches")
    }
    Slider {
        id: sldNScratches
        value: Aging.nScratches
        stepSize: 1
        maximumValue: 255

        onValueChanged: Aging.nScratches = value
    }
    SpinBox {
        id: spbNScratches
        value: sldNScratches.value
        maximumValue: sldNScratches.maximumValue
        stepSize: sldNScratches.stepSize

        onValueChanged: sldNScratches.value = value
    }

    // Aging mode.
    Label {
        text: qsTr("Add dust")
    }
    CheckBox {
        id: chkAddDust
        checked: Aging.addDust

        onCheckedChanged: Aging.addDust = checked
    }
}
