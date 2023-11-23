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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ak
import AkControls as AK
import RadiactiveElement

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
        target: Radiactive

        function onBlurChanged(blur)
        {
            sldBlur.value = blur
            spbBlur.value = blur
        }

        function onZoomChanged(zoom)
        {
            sldZoom.value = zoom
            spbZoom.value = zoom * spbZoom.multiplier
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
    }

    Label {
        id: txtMode
        text: qsTr("Mode")
    }
    ComboBox {
        id: cbxMode
        textRole: "text"
        currentIndex: modeIndex(Radiactive.mode)
        Layout.columnSpan: 2
        Layout.fillWidth: true
        Accessible.description: txtMode.text

        model: ListModel {
            ListElement {
                text: qsTr("Hard color")
                mode: RadiactiveElement.RadiationModeHardColor
            }
            ListElement {
                text: qsTr("Hard normal")
                mode: RadiactiveElement.RadiationModeHardNormal
            }
            ListElement {
                text: qsTr("Soft color")
                mode: RadiactiveElement.RadiationModeSoftColor
            }
            ListElement {
                text: qsTr("Soft normal")
                mode: RadiactiveElement.RadiationModeSoftNormal
            }
        }

        onCurrentIndexChanged: Radiactive.mode = cbxMode.model.get(currentIndex).mode
    }
    Label {
        id: txtBlur
        text: qsTr("Blur")
    }
    Slider {
        id: sldBlur
        value: Radiactive.blur
        stepSize: 1
        to: 128
        Layout.fillWidth: true
        Accessible.name: txtBlur.text

        onValueChanged: Radiactive.blur = value
    }
    SpinBox {
        id: spbBlur
        value: Radiactive.blur
        to: sldBlur.to
        stepSize: sldBlur.stepSize
        editable: true
        Accessible.name: txtBlur.text

        onValueChanged: Radiactive.blur = Number(value)
    }
    Label {
        id: txtZoom
        text: qsTr("Zoom")
    }
    Slider {
        id: sldZoom
        value: Radiactive.zoom
        stepSize: 0.1
        from: 1.0
        to: 10.0
        Layout.fillWidth: true
        Accessible.name: txtZoom.text

        onValueChanged: Radiactive.zoom = value
    }
    SpinBox {
        id: spbZoom
        value: multiplier * Radiactive.zoom
        from: multiplier * sldZoom.from
        to: multiplier * sldZoom.to
        stepSize: multiplier * sldZoom.stepSize
        editable: true
        Accessible.name: txtZoom.text

        readonly property int decimals: 1
        readonly property int multiplier: Math.pow(10, decimals)

        validator: DoubleValidator {
            bottom: Math.min(spbZoom.from, spbZoom.to)
            top:  Math.max(spbZoom.from, spbZoom.to)
        }
        textFromValue: function(value, locale) {
            return Number(value / multiplier).toLocaleString(locale, 'f', decimals)
        }
        valueFromText: function(text, locale) {
            return Number.fromLocaleString(locale, text) * multiplier
        }
        onValueModified: Radiactive.zoom = value / multiplier
    }
    Label {
        id: txtThreshold
        text: qsTr("Threshold")
    }
    Slider {
        id: sldThreshold
        value: Radiactive.threshold
        stepSize: 1
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtThreshold.text

        onValueChanged: Radiactive.threshold = value
    }
    SpinBox {
        id: spbThreshold
        value: Radiactive.threshold
        to: sldThreshold.to
        stepSize: sldThreshold.stepSize
        editable: true
        Accessible.name: txtThreshold.text

        onValueChanged: Radiactive.threshold = Number(value)
    }
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
        value: Radiactive.lumaThreshold
        stepSize: 1
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtLumaThreshold.text

        onValueChanged: Radiactive.lumaThreshold = value
    }
    SpinBox {
        id: spbLumaThreshold
        value: Radiactive.lumaThreshold
        to: sldLumaThreshold.to
        stepSize: sldLumaThreshold.stepSize
        editable: true
        Accessible.name: txtLumaThreshold.text

        onValueChanged: Radiactive.lumaThreshold = Number(value)
    }
    Label {
        id: txtAlphaDiff
        /*: Alpha channel, also known as the transparency component of a pixel
            in an image.
         */
        text: qsTr("Alpha differential")
    }
    Slider {
        id: sldAlphaDiff
        value: Radiactive.alphaDiff
        stepSize: 1
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtAlphaDiff.text

        onValueChanged: Radiactive.alphaDiff = value
    }
    SpinBox {
        id: spbAlphaDiff
        value: Radiactive.alphaDiff
        from: sldAlphaDiff.from
        to: sldAlphaDiff.to
        stepSize: sldAlphaDiff.stepSize
        editable: true
        Accessible.name: txtAlphaDiff.text

        onValueChanged: Radiactive.alphaDiff = Number(value)
    }
    Label {
        id: txtRadiationColor
        text: qsTr("Radiation color")
    }
    Item {
        Layout.fillWidth: true
    }
    AK.ColorButton {
        currentColor: AkUtils.fromRgba(Radiactive.radColor)
        title: qsTr("Choose a color")
        showAlphaChannel: true
        Accessible.description: txtRadiationColor.text

        onCurrentColorChanged: Radiactive.radColor = AkUtils.toRgba(currentColor)
    }
}
