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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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
        text: qsTr("Hue")
    }
    Slider {
        id: sldHue
        value: AdjustHSL.hue
        stepSize: 1
        from: -359
        to: 359
        Layout.fillWidth: true

        onValueChanged: AdjustHSL.hue = value
    }
    SpinBox {
        id: spbHue
        value: AdjustHSL.hue
        from: sldHue.from
        to: sldHue.to
        stepSize: sldHue.stepSize
        editable: true

        onValueChanged: AdjustHSL.hue = Number(value)
    }
    Label {
        text: qsTr("Saturation")
    }
    Slider {
        id: sldSaturation
        value: AdjustHSL.saturation
        stepSize: 1
        from: -255
        to: 255
        Layout.fillWidth: true

        onValueChanged: AdjustHSL.saturation = value
    }
    SpinBox {
        id: spbSaturation
        value: AdjustHSL.saturation
        from: sldSaturation.from
        to: sldSaturation.to
        stepSize: sldSaturation.stepSize
        editable: true

        onValueChanged: AdjustHSL.saturation = Number(value)
    }
    Label {
        text: qsTr("Luminance")
    }
    Slider {
        id: sldLuminance
        value: AdjustHSL.luminance
        stepSize: 1
        from: -255
        to: 255
        Layout.fillWidth: true

        onValueChanged: AdjustHSL.luminance = value
    }
    SpinBox {
        id: spbLuminance
        value: AdjustHSL.luminance
        from: sldLuminance.from
        to: sldLuminance.to
        stepSize: sldLuminance.stepSize
        editable: true

        onValueChanged: AdjustHSL.luminance = Number(value)
    }
}
