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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3

GridLayout {
    columns: 3

    Connections {
        target: Photocopy

        function onBrightness(brightness)
        {
            sldBrightness.value = brightness
            spbBrightness.value = brightness
        }

        function onContrast(contrast)
        {
            sldContrast.value = contrast
            spbContrast.value = contrast
        }
    }

    Label {
        id: txtBrightness
        text: qsTr("Brightness")
    }
    Slider {
        id: sldBrightness
        value: Photocopy.brightness
        stepSize: 1
        to: 256
        Layout.fillWidth: true
        Accessible.name: txtBrightness.text

        onValueChanged: Photocopy.brightness = value
    }
    SpinBox {
        id: spbBrightness
        value: Photocopy.brightness
        to: sldBrightness.to
        stepSize: sldBrightness.stepSize
        editable: true
        Accessible.name: txtBrightness.text

        onValueChanged: Photocopy.brightness = Number(value)
    }

    Label {
        id: txtContrast
        text: qsTr("Contrast")
    }
    Slider {
        id: sldContrast
        value: Photocopy.contrast
        stepSize: 1
        to: 256
        Layout.fillWidth: true
        Accessible.name: txtContrast.text

        onValueChanged: Photocopy.contrast = value
    }
    SpinBox {
        id: spbContrast
        value: Photocopy.contrast
        to: sldContrast.to
        stepSize: sldContrast.stepSize
        editable: true
        Accessible.name: txtContrast.text

        onValueChanged: Photocopy.contrast = Number(value)
    }
}
