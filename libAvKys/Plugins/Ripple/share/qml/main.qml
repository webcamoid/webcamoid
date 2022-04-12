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
    columns: 2

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

    Label {
        id: txtMode
        text: qsTr("Mode")
    }
    ComboBox {
        id: cbxMode
        textRole: "text"
        currentIndex: modeIndex(Ripple.mode)
        Layout.fillWidth: true
        Accessible.description: txtMode.text

        model: ListModel {
            ListElement {
                text: qsTr("Motion detect")
                mode: "motionDetect"
            }
            ListElement {
                text: qsTr("Rain")
                mode: "rain"
            }
        }

        onCurrentIndexChanged: Ripple.mode = cbxMode.model.get(currentIndex).mode
    }
    Label {
        id: txtAmplitude
        text: qsTr("Amplitude")
    }
    TextField {
        text: Ripple.amplitude
        placeholderText: qsTr("Amplitude")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtAmplitude.text

        onTextChanged: Ripple.amplitude = Number(text)
    }
    Label {
        id: txtDecay
        text: qsTr("Decay")
    }
    TextField {
        text: Ripple.decay
        placeholderText: qsTr("Decay")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtDecay.text

        onTextChanged: Ripple.decay = Number(text)
    }
    Label {
        id: txtThreshold
        text: qsTr("Threshold")
    }
    TextField {
        text: Ripple.threshold
        placeholderText: qsTr("Threshold")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtThreshold.text

        onTextChanged: Ripple.threshold = Number(text)
    }
    Label {
        id: txtLuma
        /*: Minimum luminance/light/white level/intensity in a gray or black and
            white picture.

            https://en.wikipedia.org/wiki/Luma_(video)
         */
        text: qsTr("Luma threshold")
    }
    TextField {
        text: Ripple.lumaThreshold
        placeholderText: txtLuma.text
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.fillWidth: true
        Accessible.name: txtLuma.text

        onTextChanged: Ripple.lumaThreshold = Number(text)
    }
}
