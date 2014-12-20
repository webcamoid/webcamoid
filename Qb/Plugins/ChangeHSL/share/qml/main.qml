/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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

    // Configure hue.
    Label {
        id: lblHue
        text: qsTr("Hue")
    }
    Slider {
        id: sldHue
        value: ChangeHSL.hsl[0]
        stepSize: 0.01
        minimumValue: -maximumValue
        maximumValue: 4

        onValueChanged: {
            var hsl = ChangeHSL.hsl
            hsl[0] = value
            ChangeHSL.hsl = hsl
        }
    }
    SpinBox {
        id: spbHue
        decimals: 2
        value: sldHue.value
        minimumValue: sldHue.minimumValue
        maximumValue: sldHue.maximumValue
        stepSize: sldHue.stepSize

        onValueChanged: sldHue.value = value
    }

    // Configure saturation.
    Label {
        id: lblSaturation
        text: qsTr("Saturation")
    }
    Slider {
        id: sldSaturation
        value: ChangeHSL.hsl[1]
        stepSize: 0.01
        minimumValue: -maximumValue
        maximumValue: 4

        onValueChanged: {
            var hsl = ChangeHSL.hsl
            hsl[1] = value
            ChangeHSL.hsl = hsl
        }
    }
    SpinBox {
        id: spbSaturation
        decimals: 2
        value: sldSaturation.value
        minimumValue: sldSaturation.minimumValue
        maximumValue: sldSaturation.maximumValue
        stepSize: sldSaturation.stepSize

        onValueChanged: sldSaturation.value = value
    }

    // Configure luminance.
    Label {
        id: lblLuminance
        text: qsTr("Luminance")
    }
    Slider {
        id: sldLuminance
        value: ChangeHSL.hsl[2]
        stepSize: 0.01
        minimumValue: -maximumValue
        maximumValue: 4

        onValueChanged: {
            var hsl = ChangeHSL.hsl
            hsl[2] = value
            ChangeHSL.hsl = hsl
        }
    }
    SpinBox {
        id: spbLuminance
        decimals: 2
        value: sldLuminance.value
        minimumValue: sldLuminance.minimumValue
        maximumValue: sldLuminance.maximumValue
        stepSize: sldLuminance.stepSize

        onValueChanged: sldLuminance.value = value
    }
}
