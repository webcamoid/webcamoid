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

    // Configure the number of scratches to show.
    Label {
        id: lblNScratches
        text: qsTr("Number of scratches")
    }
    Slider {
        id: sldNScratches
        value: Aging.nScratches
        stepSize: 1
        maximumValue: 1024

        onValueChanged: Aging.nScratches = value
    }
    SpinBox {
        id: spbNScratches
        value: sldNScratches.value
        maximumValue: sldNScratches.maximumValue
        stepSize: sldNScratches.stepSize

        onValueChanged: sldNScratches.value = value
    }

    // Configure the number of scratch lines.
    Label {
        id: lblScratchLines
        text: qsTr("Number of scratching lines")
    }
    Slider {
        id: sldScratchLines
        value: Aging.scratchLines
        stepSize: 1
        maximumValue: 256

        onValueChanged: Aging.scratchLines = value
    }
    SpinBox {
        id: spbScratchLines
        value: sldScratchLines.value
        maximumValue: sldScratchLines.maximumValue
        stepSize: sldScratchLines.stepSize

        onValueChanged: sldScratchLines.value = value
    }

    // Aging mode.
    Label {
        id: lblAgingMode
        text: qsTr("Aging mode")
    }
    ComboBox {
        id: sldAgingMode
        currentIndex: Aging.agingMode
        model: [0, 1]

        onCurrentIndexChanged: Aging.agingMode = currentIndex
    }
}
