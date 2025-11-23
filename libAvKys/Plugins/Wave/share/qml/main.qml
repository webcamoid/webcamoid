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
import Ak
import AkControls as AK

ColumnLayout {
    Label {
        text: qsTr("X-Axis")
        topPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        bottomPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        wrapMode: Text.WordWrap
        elide: Text.ElideNone
        font: AkTheme.fontSettings.h6
        Layout.fillWidth: true
    }
    Label {
        id: lblAmplitudeX
        text: qsTr("Amplitude (X)")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldAmplitudeX
        value: Wave.amplitudeX
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblAmplitudeX.text

        onValueChanged: Wave.amplitudeX = value
    }
    Label {
        id: lblFrequencyX
        text: qsTr("Frequency (X)")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldFrequencyX
        value: Wave.frequencyX
        stepSize: 0.01
        to: 100
        Layout.fillWidth: true
        Accessible.name: lblFrequencyX.text

        onValueChanged: Wave.frequencyX = value
    }
    Label {
        id: lblPhaseX
        text: qsTr("Phase (X)")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldPhaseX
        value: Wave.phaseX
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblPhaseX.text

        onValueChanged: Wave.phaseX = value
    }
    Label {
        text: qsTr("Y-Axis")
        topPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        bottomPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        wrapMode: Text.WordWrap
        elide: Text.ElideNone
        font: AkTheme.fontSettings.h6
        Layout.fillWidth: true
    }
    Label {
        id: lblAmplitudeY
        text: qsTr("Amplitude (Y)")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldAmplitudeY
        value: Wave.amplitudeY
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblAmplitudeY.text

        onValueChanged: Wave.amplitudeY = value
    }
    Label {
        id: lblFrequencyY
        text: qsTr("Frequency (Y)")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldFrequencyY
        value: Wave.frequencyY
        stepSize: 0.01
        to: 100
        Layout.fillWidth: true
        Accessible.name: lblFrequencyY.text

        onValueChanged: Wave.frequencyY = value
    }
    Label {
        id: lblPhaseY
        text: qsTr("Phase (Y)")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldPhaseY
        value: Wave.phaseY
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblPhaseY.text

        onValueChanged: Wave.phaseY = value
    }
}
