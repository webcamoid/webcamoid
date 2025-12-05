/* Webcamoid, camera capture application.
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AkControls as AK

ColumnLayout {
    Label {
        id: lblVSync
        text: qsTr("Vertical sync")
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldVSync
        value: AnalogTV.vsync
        stepSize: 0.01
        from: -1
        to: 1
        stickyPoints: [0]
        Layout.fillWidth: true
        Accessible.name: lblVSync.text

        onValueChanged: AnalogTV.vsync = value
    }
    Label {
        id: lblXOffset
        text: qsTr("Horizontal offset")
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldXOffset
        value: AnalogTV.xOffset
        stepSize: 1
        from: -1024
        to: 1024
        stickyPoints: [0]
        Layout.fillWidth: true
        Accessible.name: lblXOffset.text

        onValueChanged: AnalogTV.xOffset = value
    }
    Label {
        id: lblHSyncFactor
        text: qsTr("Horizontal sync factor")
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldHSyncFactor
        value: AnalogTV.hsyncFactor
        stepSize: 0.01
        from: -20
        to: 20
        stickyPoints: [0]
        Layout.fillWidth: true
        Accessible.name: lblHSyncFactor.text

        onValueChanged: AnalogTV.hsyncFactor = value
    }
    Label {
        id: lblHSyncSmoothness
        text: qsTr("Horizontal sync smoothness")
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldHSyncSmoothness
        value: AnalogTV.hsyncSmoothness
        stepSize: 1
        from: 0
        to: 1024
        Layout.fillWidth: true
        Accessible.name: lblHSyncSmoothness.text

        onValueChanged: AnalogTV.hsyncSmoothness = value
    }
    Label {
        id: lblHueFactor
        text: qsTr("Hue dephasing factor")
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldHueFactor
        value: AnalogTV.hueFactor
        stepSize: 0.01
        from: -2
        to: 2
        stickyPoints: [0]
        Layout.fillWidth: true
        Accessible.name: lblHueFactor.text

        onValueChanged: AnalogTV.hueFactor = value
    }
    Label {
        id: lblNoise
        text: qsTr("Noise")
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldNoise
        value: AnalogTV.noise
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblNoise.text

        onValueChanged: AnalogTV.noise = value
    }
}
