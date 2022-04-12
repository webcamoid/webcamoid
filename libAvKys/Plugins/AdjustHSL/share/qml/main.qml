/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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
    id: configs
    columns: 3

    Connections {
        target: AdjustHSL

        function onHueChanged(hue)
        {
            sldHue.value = hue
            spbHue.value = hue
        }

        function onSaturationChanged(saturation)
        {
            sldSaturation.value = saturation
            spbSaturation.value = saturation
        }

        function onLuminanceChanged(luminance)
        {
            sldLuminance.value = luminance
            spbLuminance.value = luminance
        }
    }

    Label {
        id: txtHue
        text: qsTr("Hue")
    }
    Slider {
        id: sldHue
        value: AdjustHSL.hue
        stepSize: 1
        from: -359
        to: 359
        Layout.fillWidth: true
        Accessible.name: txtHue.text

        onValueChanged: AdjustHSL.hue = value
    }
    SpinBox {
        id: spbHue
        value: AdjustHSL.hue
        from: sldHue.from
        to: sldHue.to
        stepSize: sldHue.stepSize
        editable: true
        Accessible.name: txtHue.text

        onValueChanged: AdjustHSL.hue = Number(value)
    }
    Label {
        id: txtSaturation
        text: qsTr("Saturation")
    }
    Slider {
        id: sldSaturation
        value: AdjustHSL.saturation
        stepSize: 1
        from: -255
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtSaturation.text

        onValueChanged: AdjustHSL.saturation = value
    }
    SpinBox {
        id: spbSaturation
        value: AdjustHSL.saturation
        from: sldSaturation.from
        to: sldSaturation.to
        stepSize: sldSaturation.stepSize
        editable: true
        Accessible.name: txtSaturation.text

        onValueChanged: AdjustHSL.saturation = Number(value)
    }
    Label {
        id: txtLuminance
        text: qsTr("Luminance")
    }
    Slider {
        id: sldLuminance
        value: AdjustHSL.luminance
        stepSize: 1
        from: -255
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtLuminance.text

        onValueChanged: AdjustHSL.luminance = value
    }
    SpinBox {
        id: spbLuminance
        value: AdjustHSL.luminance
        from: sldLuminance.from
        to: sldLuminance.to
        stepSize: sldLuminance.stepSize
        editable: true
        Accessible.name: txtLuminance.text

        onValueChanged: AdjustHSL.luminance = Number(value)
    }
}
