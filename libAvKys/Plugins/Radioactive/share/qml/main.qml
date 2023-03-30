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
import Ak 1.0
import AkControls 1.0 as AK
import RadioactiveElement 1.0

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
        target: Radioactive

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
        currentIndex: modeIndex(Radioactive.mode)
        Layout.columnSpan: 2
        Layout.fillWidth: true
        Accessible.description: txtMode.text

        model: ListModel {
            ListElement {
                text: qsTr("Hard color")
                mode: RadioactiveElement.RadiationModeHardColor
            }
            ListElement {
                text: qsTr("Hard normal")
                mode: RadioactiveElement.RadiationModeHardNormal
            }
            ListElement {
                text: qsTr("Soft color")
                mode: RadioactiveElement.RadiationModeSoftColor
            }
            ListElement {
                text: qsTr("Soft normal")
                mode: RadioactiveElement.RadiationModeSoftNormal
            }
        }

        onCurrentIndexChanged: Radioactive.mode = cbxMode.model.get(currentIndex).mode
    }
    Label {
        id: txtBlur
        text: qsTr("Blur")
    }
    Slider {
        id: sldBlur
        value: Radioactive.blur
        stepSize: 1
        to: 128
        Layout.fillWidth: true
        Accessible.name: txtBlur.text

        onValueChanged: Radioactive.blur = value
    }
    SpinBox {
        id: spbBlur
        value: Radioactive.blur
        to: sldBlur.to
        stepSize: sldBlur.stepSize
        editable: true
        Accessible.name: txtBlur.text

        onValueChanged: Radioactive.blur = Number(value)
    }
    Label {
        id: txtZoom
        text: qsTr("Zoom")
    }
    Slider {
        id: sldZoom
        value: Radioactive.zoom
        stepSize: 0.1
        from: 1.0
        to: 10.0
        Layout.fillWidth: true
        Accessible.name: txtZoom.text

        onValueChanged: Radioactive.zoom = value
    }
    SpinBox {
        id: spbZoom
        value: multiplier * Radioactive.zoom
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
        onValueModified: Radioactive.zoom = value / multiplier
    }
    Label {
        id: txtThreshold
        text: qsTr("Threshold")
    }
    Slider {
        id: sldThreshold
        value: Radioactive.threshold
        stepSize: 1
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtThreshold.text

        onValueChanged: Radioactive.threshold = value
    }
    SpinBox {
        id: spbThreshold
        value: Radioactive.threshold
        to: sldThreshold.to
        stepSize: sldThreshold.stepSize
        editable: true
        Accessible.name: txtThreshold.text

        onValueChanged: Radioactive.threshold = Number(value)
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
        value: Radioactive.lumaThreshold
        stepSize: 1
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtLumaThreshold.text

        onValueChanged: Radioactive.lumaThreshold = value
    }
    SpinBox {
        id: spbLumaThreshold
        value: Radioactive.lumaThreshold
        to: sldLumaThreshold.to
        stepSize: sldLumaThreshold.stepSize
        editable: true
        Accessible.name: txtLumaThreshold.text

        onValueChanged: Radioactive.lumaThreshold = Number(value)
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
        value: Radioactive.alphaDiff
        stepSize: 1
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtAlphaDiff.text

        onValueChanged: Radioactive.alphaDiff = value
    }
    SpinBox {
        id: spbAlphaDiff
        value: Radioactive.alphaDiff
        from: sldAlphaDiff.from
        to: sldAlphaDiff.to
        stepSize: sldAlphaDiff.stepSize
        editable: true
        Accessible.name: txtAlphaDiff.text

        onValueChanged: Radioactive.alphaDiff = Number(value)
    }
    Label {
        id: txtRadiationColor
        text: qsTr("Radiation color")
    }
    Item {
        Layout.fillWidth: true
    }
    AK.ColorButton {
        currentColor: AkUtils.fromRgba(Radioactive.radColor)
        title: qsTr("Choose a color")
        showAlphaChannel: true
        Accessible.description: txtRadiationColor.text

        onCurrentColorChanged: Radioactive.radColor = AkUtils.toRgba(currentColor)
    }
}
