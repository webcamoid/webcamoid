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
import AkQmlControls 1.0

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
            spbCool.rvalue = cool
        }
        onThresholdChanged: {
            sldThreshold.value = threshold
            spbThreshold.rvalue = threshold
        }
        onLumaThresholdChanged: {
            sldLumaThreshold.value = lumaThreshold
            spbLumaThreshold.rvalue = lumaThreshold
        }
        onAlphaDiffChanged: {
            sldAlphaDiff.value = alphaDiff
            spbAlphaDiff.rvalue = alphaDiff
        }
        onAlphaVariationChanged: {
            sldAlphaVariation.value = alphaVariation
            spbAlphaVariation.rvalue = alphaVariation
        }
        onNColorsChanged: {
            sldNColors.value = nColors
            spbNColors.rvalue = nColors
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
    AkSpinBox {
        id: spbCool
        rvalue: Fire.cool
        minimumValue: sldCool.from
        maximumValue: sldCool.to
        step: sldCool.stepSize

        onRvalueChanged: Fire.cool = rvalue
    }

    // Disolving factor.
    Label {
        text: qsTr("Disolve")
    }
    TextField {
        text: Fire.disolve
        validator: RegExpValidator {
            regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
        }
        Layout.columnSpan: 2
        Layout.fillWidth: true

        onTextChanged: Fire.disolve = text
    }

    // Blur.
    Label {
        text: qsTr("Blur")
    }
    TextField {
        text: Fire.blur
        validator: RegExpValidator {
            regExp: /\d+/
        }
        Layout.columnSpan: 2
        Layout.fillWidth: true

        onTextChanged: Fire.blur = text
    }

    // Zoom.
    Label {
        text: qsTr("Zoom")
    }
    TextField {
        text: Fire.zoom
        validator: RegExpValidator {
            regExp: /-?(\d+\.\d+|\d+\.|\.\d+|\d+)/
        }
        Layout.columnSpan: 2
        Layout.fillWidth: true

        onTextChanged: Fire.zoom = text
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
    AkSpinBox {
        id: spbThreshold
        rvalue: Fire.threshold
        maximumValue: sldThreshold.to
        step: sldThreshold.stepSize

        onRvalueChanged: Fire.threshold = rvalue
    }

    // Luma threshold.
    Label {
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
    AkSpinBox {
        id: spbLumaThreshold
        rvalue: Fire.lumaThreshold
        maximumValue: sldLumaThreshold.to
        step: sldLumaThreshold.stepSize

        onRvalueChanged: Fire.lumaThreshold = rvalue
    }

    // Alpha diff.
    Label {
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
    AkSpinBox {
        id: spbAlphaDiff
        rvalue: Fire.alphaDiff
        minimumValue: sldAlphaDiff.from
        maximumValue: sldAlphaDiff.to
        step: sldAlphaDiff.stepSize

        onRvalueChanged: Fire.alphaDiff = rvalue
    }

    // Alpha variation.
    Label {
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
    AkSpinBox {
        id: spbAlphaVariation
        rvalue: Fire.alphaVariation
        maximumValue: sldAlphaVariation.to
        step: sldAlphaVariation.stepSize

        onRvalueChanged: Fire.alphaVariation = rvalue
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
    AkSpinBox {
        id: spbNColors
        rvalue: Fire.nColors
        maximumValue: sldNColors.to
        step: sldNColors.stepSize

        onRvalueChanged: Fire.nColors = rvalue
    }
}
