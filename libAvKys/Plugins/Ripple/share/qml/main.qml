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
import RippleElement

ColumnLayout {
    function modeIndex(mode)
    {
        var index = -1

        for (var i = 0; i < cbxMode.model.count; i++)
            if (cbxMode.model.get(i).mode == mode) {
                index = i
                break
            }

        return index
    }

    AK.LabeledComboBox {
        id: cbxMode
        label: qsTr("Mode")
        textRole: "text"
        currentIndex: modeIndex(Ripple.mode)
        Layout.fillWidth: true
        Accessible.description: label

        model: ListModel {
            ListElement {
                text: qsTr("Motion detect")
                mode: RippleElement.RippleModeMotionDetect
            }
            ListElement {
                text: qsTr("Rain")
                mode: RippleElement.RippleModeRain
            }
        }

        onCurrentIndexChanged: Ripple.mode = cbxMode.model.get(currentIndex).mode
    }
    Label {
        text: qsTr("General parameters")
        topPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        bottomPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        wrapMode: Text.WordWrap
        elide: Text.ElideNone
        font: AkTheme.fontSettings.h6
        Layout.fillWidth: true
    }
    Label {
        id: txtAmplitude
        text: qsTr("Amplitude")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldAmplitude
        value: Ripple.amplitude
        stepSize: 1
        to: 1024
        Layout.fillWidth: true
        Accessible.name: txtAmplitude.text

        onValueChanged: Ripple.amplitude = value
    }
    Label {
        id: txtDecay
        text: qsTr("Decay")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldDecay
        value: Ripple.decay
        stepSize: 1
        to: 8
        Layout.fillWidth: true
        Accessible.name: txtDecay.text

        onValueChanged: Ripple.decay = value
    }
    Label {
        text: qsTr("Motion detection parameters")
        topPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        bottomPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        wrapMode: Text.WordWrap
        elide: Text.ElideNone
        font: AkTheme.fontSettings.h6
        Layout.fillWidth: true
    }
    Label {
        id: txtThreshold
        text: qsTr("Threshold")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldThreshold
        value: Ripple.threshold
        stepSize: 1
        to: 256
        Layout.fillWidth: true
        Accessible.name: txtThreshold.text

        onValueChanged: Ripple.threshold = value
    }
    Label {
        id: txtLumaThreshold
        /*: Minimum luminance/light/white level/intensity in a gray or black and
            white picture.

            https://en.wikipedia.org/wiki/Luma_(video)
         */
        text: qsTr("Luma threshold")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldLumaThreshold
        value: Ripple.lumaThreshold
        stepSize: 1
        to: 256
        Layout.fillWidth: true
        Accessible.name: txtLumaThreshold.text

        onValueChanged: Ripple.lumaThreshold = value
    }
    Label {
        text: qsTr("Rain parameters")
        topPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        bottomPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        wrapMode: Text.WordWrap
        elide: Text.ElideNone
        font: AkTheme.fontSettings.h6
        Layout.fillWidth: true
    }
    Label {
        id: txtMinDropSize
        text: qsTr("Minimum drop size")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldMinDropSize
        value: Ripple.minDropSize
        stepSize: 1
        to: 1024
        Layout.fillWidth: true
        Accessible.name: txtMinDropSize.text

        onValueChanged: Ripple.minDropSize = value
    }
    Label {
        id: txtMaxDropSize
        text: qsTr("Maximum drop size")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldMaxDropSize
        value: Ripple.maxDropSize
        stepSize: 1
        to: 1024
        Layout.fillWidth: true
        Accessible.name: txtMaxDropSize.text

        onValueChanged: Ripple.maxDropSize = value
    }
    Label {
        id: lblDropSigma
        text: qsTr("Drop thickness")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldDropSigma
        value: Ripple.dropSigma
        stepSize: 0.01
        from: 0
        to: 10
        Layout.fillWidth: true
        Accessible.name: lblDropSigma.text

        onValueChanged: Ripple.dropSigma = value
    }
    Label {
        id: lblDropProbability
        text: qsTr("Drop frequency")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldDropProbability
        value: Ripple.dropProbability
        stepSize: 0.01
        from: 0
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblDropProbability.text

        onValueChanged: Ripple.dropProbability = value
    }
}
