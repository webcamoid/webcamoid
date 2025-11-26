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
import Ak
import AkControls as AK
import RadioactiveElement

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

    AK.LabeledComboBox {
        id: cbxMode
        label: qsTr("Mode")
        textRole: "text"
        currentIndex: modeIndex(Radioactive.mode)
        Layout.fillWidth: true
        Accessible.description: label

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
        font.bold: true
        Layout.fillWidth: true
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
    Label {
        id: txtZoom
        text: qsTr("Zoom")
        font.bold: true
        Layout.fillWidth: true
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
    Label {
        id: txtThreshold
        text: qsTr("Threshold")
        font.bold: true
        Layout.fillWidth: true
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
        value: Radioactive.lumaThreshold
        stepSize: 1
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtLumaThreshold.text

        onValueChanged: Radioactive.lumaThreshold = value
    }
    Label {
        id: txtAlphaDiff
        /*: Alpha channel, also known as the transparency component of a pixel
            in an image.
         */
        text: qsTr("Alpha differential")
        font.bold: true
        Layout.fillWidth: true
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
    RowLayout {
        Label {
            id: txtRadiationColor
            text: qsTr("Radiation color")
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(Radioactive.radColor)
            title: qsTr("Choose a color")
            showAlphaChannel: true
            Accessible.description: txtRadiationColor.text

            onCurrentColorChanged: Radioactive.radColor = AkUtils.toRgba(currentColor)
        }
    }
}
