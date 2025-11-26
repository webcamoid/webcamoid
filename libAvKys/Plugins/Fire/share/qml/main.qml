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
import FireElement
import AkControls as AK

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

    // Fire mode.
    AK.LabeledComboBox {
        id: cbxMode
        label: qsTr("Mode")
        textRole: "text"
        currentIndex: modeIndex(Fire.mode)
        Layout.fillWidth: true
        Accessible.description: label

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
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldCool
        value: Fire.cool
        stepSize: 1
        from: -255
        to: 255
        stickyPoints: [0]
        Layout.fillWidth: true
        Accessible.name: txtCooling.text

        onValueChanged: Fire.cool = value
    }

    // Dissolving factor.
    Label {
        id: txtDissolve
        text: qsTr("Dissolve")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        text: Fire.dissolve
        placeholderText: qsTr("Dissolve")
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
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
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        text: Fire.blur
        placeholderText: qsTr("Blur")
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /\d+/
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
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        text: Fire.zoom
        placeholderText: qsTr("Zoom")
        selectByMouse: true
        validator: RegularExpressionValidator {
            regularExpression: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
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
        font.bold: true
        Layout.fillWidth: true
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

    // Luma threshold.
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
        value: Fire.lumaThreshold
        stepSize: 1
        to: 255
        Layout.fillWidth: true
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
        font.bold: true
        Layout.fillWidth: true
    }
    AK.StickySlider {
        id: sldAlphaDiff
        value: Fire.alphaDiff
        stepSize: 1
        from: -255
        to: 255
        stickyPoints: [0]
        Layout.fillWidth: true
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
        font.bold: true
        Layout.fillWidth: true
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

    // Number of colors.
    Label {
        id: txtNumberOfColors
        text: qsTr("Number of colors")
        font.bold: true
        Layout.fillWidth: true
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
}
