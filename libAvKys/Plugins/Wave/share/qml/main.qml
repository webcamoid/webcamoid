/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3
import "qrc:/Ak/share/qml/AkQmlControls"

GridLayout {
    columns: 3

    function fromRgba(rgba)
    {
        var a = ((rgba >> 24) & 0xff) / 255.0
        var r = ((rgba >> 16) & 0xff) / 255.0
        var g = ((rgba >> 8) & 0xff) / 255.0
        var b = (rgba & 0xff) / 255.0

        return Qt.rgba(r, g, b, a)
    }

    function toRgba(color)
    {
        var a = Math.round(255 * color.a) << 24
        var r = Math.round(255 * color.r) << 16
        var g = Math.round(255 * color.g) << 8
        var b = Math.round(255 * color.b)

        return a | r | g | b
    }

    Connections {
        target: Wave

        onAmplitudeChanged: {
            sldAmplitude.value = amplitude
            spbAmplitude.rvalue = amplitude
        }

        onFrequencyChanged: {
            sldFrequency.value = frequency
            spbFrequency.rvalue = frequency
        }

        onPhaseChanged: {
            sldPhase.value = phase
            spbPhase.rvalue = phase
        }
    }

    Label {
        id: lblAmplitude
        text: qsTr("Amplitude")
    }
    Slider {
        id: sldAmplitude
        value: Wave.amplitude
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true

        onValueChanged: Wave.amplitude = value
    }
    AkSpinBox {
        id: spbAmplitude
        decimals: 2
        rvalue: Wave.amplitude
        maximumValue: sldAmplitude.to
        step: sldAmplitude.stepSize

        onRvalueChanged: Wave.amplitude = rvalue
    }

    Label {
        id: lblFrequency
        text: qsTr("Frequency")
    }
    Slider {
        id: sldFrequency
        value: Wave.frequency
        stepSize: 0.01
        to: 100
        Layout.fillWidth: true

        onValueChanged: Wave.frequency = value
    }
    AkSpinBox {
        id: spbFrequency
        decimals: 2
        rvalue: Wave.frequency
        maximumValue: sldFrequency.to
        step: sldFrequency.stepSize

        onRvalueChanged: Wave.frequency = rvalue
    }

    Label {
        id: lblPhase
        text: qsTr("Phase")
    }
    Slider {
        id: sldPhase
        value: Wave.phase
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true

        onValueChanged: Wave.phase = value
    }
    AkSpinBox {
        id: spbPhase
        decimals: 2
        rvalue: Wave.phase
        maximumValue: sldPhase.to
        step: sldPhase.stepSize

        onRvalueChanged: Wave.phase = rvalue
    }

    Label {
        text: qsTr("Background color")
    }
    AkColorButton {
        currentColor: fromRgba(Wave.background)
        title: qsTr("Choose the background color")

        onCurrentColorChanged: Wave.background = toRgba(currentColor)
    }
    Label {
    }
}
