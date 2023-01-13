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
import FireElement 1.0

GridLayout {
    columns: 3

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

    Connections {
        target: Fire

        function onCoolChanged(cool)
        {
            sldCool.value = cool
            spbCool.value = cool
        }

        function onThresholdChanged(threshold)
        {
            sldThreshold.value = threshold
            spbThreshold.value = threshold
        }

        function onLumaThresholdChanged(lumaThreshold)
        {
            sldLumaThreshold.value = lumaThreshold
            spbLumaThreshold.value = lumaThreshold
        }

        function onAlphaDiffChanged(alphaDiff)
        {
            sldAlphaDiff.value = alphaDiff
            spbAlphaDiff.value = alphaDiff
        }

        function onAlphaVariationChanged(alphaVariation)
        {
            sldAlphaVariation.value = alphaVariation
            spbAlphaVariation.value = alphaVariation
        }

        function onNColorsChanged(nColors)
        {
            sldNColors.value = nColors
            spbNColors.value = nColors
        }
    }

    // Fire mode.
    Label {
        id: txtMode
        text: qsTr("Mode")
    }
    ComboBox {
        id: cbxMode
        textRole: "text"
        currentIndex: modeIndex(Fire.mode)
        Layout.columnSpan: 2
        Layout.fillWidth: true
        Accessible.description: txtMode.text

        model: ListModel {
            ListElement {
                text: qsTr("Soft")
                mode: FireElement.FireModeSoft
            }
            ListElement {
                text: qsTr("Hard")
                mode: FireElement.FireModeHard
            }
        }

        onCurrentIndexChanged: Fire.mode = cbxMode.model.get(currentIndex).mode
    }

    // Cooling factor.
    Label {
        id: txtCooling
        text: qsTr("Cooling")
    }
    Slider {
        id: sldCool
        value: Fire.cool
        stepSize: 1
        from: -255
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtCooling.text

        onValueChanged: Fire.cool = value
    }
    SpinBox {
        id: spbCool
        value: Fire.cool
        from: sldCool.from
        to: sldCool.to
        stepSize: sldCool.stepSize
        editable: true
        Accessible.name: txtCooling.text

        onValueChanged: Fire.cool = value
    }

    // Dissolving factor.
    Label {
        id: txtDissolve
        text: qsTr("Dissolve")
    }
    TextField {
        text: Fire.dissolve
        placeholderText: qsTr("Dissolve")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
        }
        Layout.columnSpan: 2
        Layout.fillWidth: true
        Accessible.name: txtDissolve.text

        onTextChanged: Fire.dissolve = Number(text)
    }

    // Blur.
    Label {
        id: txtBlur
        text: qsTr("Blur")
    }
    TextField {
        text: Fire.blur
        placeholderText: qsTr("Blur")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.columnSpan: 2
        Layout.fillWidth: true
        Accessible.name: txtBlur.text

        onTextChanged: Fire.blur = Number(text)
    }

    // Zoom.
    Label {
        id: txtZoom
        text: qsTr("Zoom")
    }
    TextField {
        text: Fire.zoom
        placeholderText: qsTr("Zoom")
        selectByMouse: true
        validator: RegExpValidator {
            regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
        }
        Layout.columnSpan: 2
        Layout.fillWidth: true
        Accessible.name: txtZoom.text

        onTextChanged: Fire.zoom = Number(text)
    }

    // Threshold.
    Label {
        id: txtThreshold
        text: qsTr("Threshold")
    }
    Slider {
        id: sldThreshold
        value: Fire.threshold
        stepSize: 1
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtThreshold.text

        onValueChanged: Fire.threshold = value
    }
    SpinBox {
        id: spbThreshold
        value: Fire.threshold
        to: sldThreshold.to
        stepSize: sldThreshold.stepSize
        editable: true
        Accessible.name: txtThreshold.text

        onValueChanged: Fire.threshold = value
    }

    // Luma threshold.
    Label {
        id: txtLumaThreshold
        /*: Minimum luminance/light/white level/intensity in a gray or black and
            white picture.

            https://en.wikipedia.org/wiki/Luma_(video)
         */
        text: qsTr("Luma threshold")
    }
    Slider {
        id: sldLumaThreshold
        value: Fire.lumaThreshold
        stepSize: 1
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtLumaThreshold.text

        onValueChanged: Fire.lumaThreshold = value
    }
    SpinBox {
        id: spbLumaThreshold
        value: Fire.lumaThreshold
        to: sldLumaThreshold.to
        stepSize: sldLumaThreshold.stepSize
        editable: true
        Accessible.name: txtLumaThreshold.text

        onValueChanged: Fire.lumaThreshold = value
    }

    // Alpha diff.
    Label {
        id: txtAlphaDiff
        /*: Alpha channel, also known as the transparency component of a pixel
            in an image.
         */
        text: qsTr("Alpha diff")
    }
    Slider {
        id: sldAlphaDiff
        value: Fire.alphaDiff
        stepSize: 1
        from: -255
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtAlphaDiff.text

        onValueChanged: Fire.alphaDiff = value
    }
    SpinBox {
        id: spbAlphaDiff
        value: Fire.alphaDiff
        from: sldAlphaDiff.from
        to: sldAlphaDiff.to
        stepSize: sldAlphaDiff.stepSize
        editable: true
        Accessible.name: txtAlphaDiff.text

        onValueChanged: Fire.alphaDiff = value
    }

    // Alpha variation.
    Label {
        id: txtAlphaVariation
        /*: Alpha channel, also known as the transparency component of a pixel
            in an image.
         */
        text: qsTr("Alpha variation")
    }
    Slider {
        id: sldAlphaVariation
        value: Fire.alphaVariation
        stepSize: 1
        to: 256
        Layout.fillWidth: true
        Accessible.name: txtAlphaVariation.text

        onValueChanged: Fire.alphaVariation = value
    }
    SpinBox {
        id: spbAlphaVariation
        value: Fire.alphaVariation
        to: sldAlphaVariation.to
        stepSize: sldAlphaVariation.stepSize
        editable: true
        Accessible.name: txtAlphaVariation.text

        onValueChanged: Fire.alphaVariation = value
    }

    // Number of colors.
    Label {
        id: txtNumberOfColors
        text: qsTr("Number of colors")
    }
    Slider {
        id: sldNColors
        value: Fire.nColors
        stepSize: 1
        to: 256
        Layout.fillWidth: true
        Accessible.name: txtNumberOfColors.text

        onValueChanged: Fire.nColors = value
    }
    SpinBox {
        id: spbNColors
        value: Fire.nColors
        to: sldNColors.to
        stepSize: sldNColors.stepSize
        editable: true
        Accessible.name: txtNumberOfColors.text

        onValueChanged: Fire.nColors = value
    }
}
