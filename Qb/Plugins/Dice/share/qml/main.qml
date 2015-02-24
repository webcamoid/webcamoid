/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

GridLayout {
    columns: 3

    Label {
        id: lblCubeBits
        text: qsTr("Size")
    }
    Slider {
        id: sldCubeBits
        value: Dice.cubeBits
        stepSize: 1
        maximumValue: 5

        onValueChanged: Dice.cubeBits = value
    }
    SpinBox {
        id: spbcubeBits
        value: sldCubeBits.value
        maximumValue: sldCubeBits.maximumValue
        stepSize: sldCubeBits.stepSize

        onValueChanged: sldCubeBits.value = value
    }
}
