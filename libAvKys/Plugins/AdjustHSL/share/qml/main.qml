/* Webcamoid, camera capture application.
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
import AkControls as AK

ColumnLayout {
    id: configs

    Label {
        id: txtHue
        text: qsTr("Hue")
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldHue
        value: AdjustHSL.hue
        stepSize: 1
        from: -359
        to: 359
        stickyPoints: [0]
        Layout.fillWidth: true
        Accessible.name: txtHue.text

        onValueChanged: AdjustHSL.hue = value
    }
    Label {
        id: txtSaturation
        text: qsTr("Saturation")
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldSaturation
        value: AdjustHSL.saturation
        stepSize: 1
        from: -255
        to: 255
        stickyPoints: [0]
        Layout.fillWidth: true
        Accessible.name: txtSaturation.text

        onValueChanged: AdjustHSL.saturation = value
    }
    Label {
        id: txtLuminance
        text: qsTr("Luminance")
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldLuminance
        value: AdjustHSL.luminance
        stepSize: 1
        from: -255
        to: 255
        stickyPoints: [0]
        Layout.fillWidth: true
        Accessible.name: txtLuminance.text

        onValueChanged: AdjustHSL.luminance = value
    }
}
