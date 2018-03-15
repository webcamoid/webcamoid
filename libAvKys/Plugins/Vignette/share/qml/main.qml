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
import AkQmlControls 1.0

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
        target: Vignette

        onAspectChanged: {
            sldAspect.value = aspect
            spbAspect.rvalue = aspect
        }

        onScaleChanged: {
            sldScale.value = scale
            spbScale.rvalue = scale
        }

        onSoftnessChanged: {
            sldSoftness.value = softness
            spbSoftness.rvalue = softness
        }
    }

    Label {
        text: qsTr("Color")
    }
    AkColorButton {
        currentColor: fromRgba(Vignette.color)
        title: qsTr("Choose the vignette color")
        showAlphaChannel: true

        onCurrentColorChanged: Vignette.color = toRgba(currentColor)
    }
    Label {
    }

    Label {
        id: lblAspect
        text: qsTr("Aspect")
    }
    Slider {
        id: sldAspect
        value: Vignette.aspect
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true

        onValueChanged: Vignette.aspect = value
    }
    AkSpinBox {
        id: spbAspect
        decimals: 2
        rvalue: Vignette.aspect
        maximumValue: sldAspect.to
        step: sldAspect.stepSize

        onRvalueChanged: Vignette.aspect = rvalue
    }

    Label {
        id: lblScale
        text: qsTr("Scale")
    }
    Slider {
        id: sldScale
        value: Vignette.scale
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true

        onValueChanged: Vignette.scale = value
    }
    AkSpinBox {
        id: spbScale
        decimals: 2
        rvalue: Vignette.scale
        maximumValue: sldScale.to
        step: sldScale.stepSize

        onRvalueChanged: Vignette.scale = rvalue
    }

    Label {
        id: lblSoftness
        text: qsTr("Softness")
    }
    Slider {
        id: sldSoftness
        value: Vignette.softness
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true

        onValueChanged: Vignette.softness = value
    }
    AkSpinBox {
        id: spbSoftness
        decimals: 2
        rvalue: Vignette.softness
        maximumValue: sldSoftness.to
        step: sldSoftness.stepSize

        onRvalueChanged: Vignette.softness = rvalue
    }
}
