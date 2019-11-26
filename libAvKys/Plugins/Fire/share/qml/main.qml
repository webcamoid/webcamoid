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

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

GridLayout {
    columns: 3

    function modeIndex(mode)
    {
        var index = -1

        for (var i = 0; i < cbxMode.model.count; i++)
            if (cbxMode.model.get(i).mode === mode) {
                index = i
                break
            }

        return index
    }

    Connections {
        target: Fire

        onCoolChanged: {
            sldCool.value = cool
            spbCool.value = cool
        }
        onThresholdChanged: {
            sldThreshold.value = threshold
            spbThreshold.value = threshold
        }
        onLumaThresholdChanged: {
            sldLumaThreshold.value = lumaThreshold
            spbLumaThreshold.value = lumaThreshold
        }
        onAlphaDiffChanged: {
            sldAlphaDiff.value = alphaDiff
            spbAlphaDiff.value = alphaDiff
        }
        onAlphaVariationChanged: {
            sldAlphaVariation.value = alphaVariation
            spbAlphaVariation.value = alphaVariation
        }
        onNColorsChanged: {
            sldNColors.value = nColors
            spbNColors.value = nColors
        }
    }

    // Fire mode.
    Label {
        text: qsTr("Mode")
    }
    ComboBox {
        id: cbxMode
        textRole: "text"
        currentIndex: modeIndex(Fire.mode)
        Layout.columnSpan: 2
        Layout.fillWidth: true

        model: ListModel {
            ListElement {
                text: qsTr("Soft")
                mode: "soft"
            }
            ListElement {
                text: qsTr("Hard")
                mode: "hard"
            }
        }

        onCurrentIndexChanged: Fire.mode = cbxMode.model.get(currentIndex).mode
    }

    // Cooling factor.
    Label {
        text: qsTr("Cooling")
    }
    Slider {
        id: sldCool
        value: Fire.cool
        stepSize: 1
        from: -255
        to: 255
        Layout.fillWidth: true

        onValueChanged: Fire.cool = value
    }
    SpinBox {
        id: spbCool
        value: Fire.cool
        from: sldCool.from
        to: sldCool.to
        stepSize: sldCool.stepSize
        editable: true

        onValueChanged: Fire.cool = value
    }

    // Dissolving factor.
    Label {
        text: qsTr("Dissolve")
    }
    TextField {
        text: Fire.dissolve
        placeholderText: qsTr("Dissolve")
        validator: RegExpValidator {
            regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
        }
        Layout.columnSpan: 2
        Layout.fillWidth: true

        onTextChanged: Fire.dissolve = Number(text)
    }

    // Blur.
    Label {
        text: qsTr("Blur")
    }
    TextField {
        text: Fire.blur
        placeholderText: qsTr("Blur")
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.columnSpan: 2
        Layout.fillWidth: true

        onTextChanged: Fire.blur = Number(text)
    }

    // Zoom.
    Label {
        text: qsTr("Zoom")
    }
    TextField {
        text: Fire.zoom
        placeholderText: qsTr("Zoom")
        validator: RegExpValidator {
            regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
        }
        Layout.columnSpan: 2
        Layout.fillWidth: true

        onTextChanged: Fire.zoom = Number(text)
    }

    // Threshold.
    Label {
        text: qsTr("Threshold")
    }
    Slider {
        id: sldThreshold
        value: Fire.threshold
        stepSize: 1
        to: 255
        Layout.fillWidth: true

        onValueChanged: Fire.threshold = value
    }
    SpinBox {
        id: spbThreshold
        value: Fire.threshold
        to: sldThreshold.to
        stepSize: sldThreshold.stepSize
        editable: true

        onValueChanged: Fire.threshold = value
    }

    // Luma threshold.
    Label {
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

        onValueChanged: Fire.lumaThreshold = value
    }
    SpinBox {
        id: spbLumaThreshold
        value: Fire.lumaThreshold
        to: sldLumaThreshold.to
        stepSize: sldLumaThreshold.stepSize
        editable: true

        onValueChanged: Fire.lumaThreshold = value
    }

    // Alpha diff.
    Label {
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

        onValueChanged: Fire.alphaDiff = value
    }
    SpinBox {
        id: spbAlphaDiff
        value: Fire.alphaDiff
        from: sldAlphaDiff.from
        to: sldAlphaDiff.to
        stepSize: sldAlphaDiff.stepSize
        editable: true

        onValueChanged: Fire.alphaDiff = value
    }

    // Alpha variation.
    Label {
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

        onValueChanged: Fire.alphaVariation = value
    }
    SpinBox {
        id: spbAlphaVariation
        value: Fire.alphaVariation
        to: sldAlphaVariation.to
        stepSize: sldAlphaVariation.stepSize
        editable: true

        onValueChanged: Fire.alphaVariation = value
    }

    // N° of colors.
    Label {
        text: qsTr("N° of colors")
    }
    Slider {
        id: sldNColors
        value: Fire.nColors
        stepSize: 1
        to: 256
        Layout.fillWidth: true

        onValueChanged: Fire.nColors = value
    }
    SpinBox {
        id: spbNColors
        value: Fire.nColors
        to: sldNColors.to
        stepSize: sldNColors.stepSize
        editable: true

        onValueChanged: Fire.nColors = value
    }
}
