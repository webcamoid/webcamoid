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

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1

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

    function invert(color) {
        return Qt.rgba(1.0 - color.r, 1.0 - color.g, 1.0 - color.b, 1)
    }

    Label {
        text: qsTr("Color")
    }
    Button {
        Layout.preferredWidth: 32
        Layout.preferredHeight: 32

        style: ButtonStyle {
            background: Rectangle {
                color: fromRgba(Vignette.color)
                border.color: invert(color)
                border.width: 1
            }
        }

        onClicked: colorDialog.open()
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
        maximumValue: 1

        onValueChanged: Vignette.aspect = value
    }
    SpinBox {
        id: spbAspect
        decimals: 2
        value: sldAspect.value
        maximumValue: sldAspect.maximumValue
        stepSize: sldAspect.stepSize

        onValueChanged: sldAspect.value = value
    }

    Label {
        id: lblScale
        text: qsTr("Scale")
    }
    Slider {
        id: sldScale
        value: Vignette.scale
        stepSize: 0.01
        maximumValue: 1

        onValueChanged: Vignette.scale = value
    }
    SpinBox {
        id: spbScale
        decimals: 2
        value: sldScale.value
        maximumValue: sldScale.maximumValue
        stepSize: sldScale.stepSize

        onValueChanged: sldScale.value = value
    }

    Label {
        id: lblSoftness
        text: qsTr("Softness")
    }
    Slider {
        id: sldSoftness
        value: Vignette.softness
        stepSize: 0.01
        maximumValue: 1

        onValueChanged: Vignette.softness = value
    }
    SpinBox {
        id: spbSoftness
        decimals: 2
        value: sldSoftness.value
        maximumValue: sldSoftness.maximumValue
        stepSize: sldSoftness.stepSize

        onValueChanged: sldSoftness.value = value
    }

    ColorDialog {
        id: colorDialog
        title: qsTr("Choose the vignette color")
        currentColor: fromRgba(Vignette.color)
        showAlphaChannel: true

        onAccepted: Vignette.color = toRgba(color)
    }
}
