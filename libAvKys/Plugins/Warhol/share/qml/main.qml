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

GridLayout {
    columns: 3

    Connections {
        target: Warhol

        function onFrameLenChanged(frameLen)
        {
            sldFrameLen.value = frameLen
            spbFrameLen.value = frameLen
        }

        function onLevelsChanged(levels)
        {
            sldLevels.value = levels
            spbLevels.value = levels
        }

        function onSaturationChanged(saturation)
        {
            sldSaturation.value = saturation
            spbSaturation.value = saturation
        }

        function onLuminanceChanged(luminance)
        {
            sldLuminance.value = luminance
            spbLuminance.value = luminance
        }

        function onPaletteOffsetChanged(paletteOffset)
        {
            sldPaletteOffset.value = paletteOffset
            spbPaletteOffset.value = paletteOffset
        }

        function onShadowThLowChanged(shadowThLow)
        {
            sldShadowTh.first.value = shadowThLow
            spbShadowThLow.value = shadowThLow
        }

        function onShadowThHiChanged(shadowThHi)
        {
            sldShadowTh.second.value = shadowThHi
            spbShadowThHi.value = shadowThHi
        }
    }

    Label {
        text: qsTr("<b>Palette</b>")
        topPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        bottomPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        wrapMode: Text.Wrap
        Layout.columnSpan: 3
        Layout.fillWidth: true
    }
    Label {
        id: txtFrameLen
        text: qsTr("Frame length")
    }
    Slider {
        id: sldFrameLen
        value: Warhol.frameLen
        stepSize: 1
        from: 1
        to: 4
        Layout.fillWidth: true
        Accessible.name: txtFrameLen.text

        onValueChanged: Warhol.frameLen = value
    }
    SpinBox {
        id: spbFrameLen
        value: Warhol.frameLen
        from: sldFrameLen.from
        to: sldFrameLen.to
        stepSize: sldFrameLen.stepSize
        editable: true
        Accessible.name: txtFrameLen.text

        onValueChanged: Warhol.frameLen = Number(value)
    }
    Label {
        id: txtLevels
        text: qsTr("Levels")
    }
    Slider {
        id: sldLevels
        value: Warhol.levels
        stepSize: 1
        from: 2
        to: 4
        Layout.fillWidth: true
        Accessible.name: txtLevels.text

        onValueChanged: Warhol.levels = value
    }
    SpinBox {
        id: spbLevels
        value: Warhol.levels
        from: sldLevels.from
        to: sldLevels.to
        stepSize: sldLevels.stepSize
        editable: true
        Accessible.name: txtLevels.text

        onValueChanged: Warhol.levels = Number(value)
    }
    Label {
        id: txtSaturation
        text: qsTr("Saturation")
    }
    Slider {
        id: sldSaturation
        value: Warhol.saturation
        stepSize: 1
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtSaturation.text

        onValueChanged: Warhol.saturation = value
    }
    SpinBox {
        id: spbSaturation
        value: Warhol.saturation
        from: sldSaturation.from
        to: sldSaturation.to
        stepSize: sldSaturation.stepSize
        editable: true
        Accessible.name: txtSaturation.text

        onValueChanged: Warhol.saturation = Number(value)
    }
    Label {
        id: txtLuminance
        text: qsTr("Luminance")
    }
    Slider {
        id: sldLuminance
        value: Warhol.luminance
        stepSize: 1
        to: 255
        Layout.fillWidth: true
        Accessible.name: txtLuminance.text

        onValueChanged: Warhol.luminance = value
    }
    SpinBox {
        id: spbLuminance
        value: Warhol.luminance
        from: sldLuminance.from
        to: sldLuminance.to
        stepSize: sldLuminance.stepSize
        editable: true
        Accessible.name: txtLuminance.text

        onValueChanged: Warhol.luminance = Number(value)
    }
    Label {
        id: txtPaletteOffset
        text: qsTr("Hue offset")
    }
    Slider {
        id: sldPaletteOffset
        value: Warhol.paletteOffset
        stepSize: 1
        to: 360
        Layout.fillWidth: true
        Accessible.name: txtPaletteOffset.text

        onValueChanged: Warhol.paletteOffset = value
    }
    SpinBox {
        id: spbPaletteOffset
        value: Warhol.paletteOffset
        from: sldPaletteOffset.from
        to: sldPaletteOffset.to
        stepSize: sldPaletteOffset.stepSize
        editable: true
        Accessible.name: txtPaletteOffset.text

        onValueChanged: Warhol.paletteOffset = Number(value)
    }

    Label {
        text: qsTr("<b>Shadow</b>")
        topPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        bottomPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        wrapMode: Text.Wrap
        Layout.columnSpan: 3
        Layout.fillWidth: true
    }
    Label {
        id: txtShadowThLow
        text: qsTr("Shadow threshold")
    }
    RowLayout {
        Layout.columnSpan: 2

        SpinBox {
            id: spbShadowThLow
            value: Warhol.shadowThLow
            to: sldShadowTh.to
            stepSize: sldShadowTh.stepSize
            editable: true
            Accessible.name: qsTr("Shadow threshold low")

            onValueChanged: Warhol.shadowThLow = Number(value)
        }
        RangeSlider {
            id: sldShadowTh
            first.value: Warhol.shadowThLow
            second.value: Warhol.shadowThHi
            stepSize: 1
            to: 255
            Layout.fillWidth: true
            Accessible.name: txtShadowThLow.text

            first.onValueChanged: Warhol.shadowThLow = first.value
            second.onValueChanged: Warhol.shadowThHi = second.value
        }
        SpinBox {
            id: spbShadowThHi
            value: Warhol.shadowThHi
            to: sldShadowTh.to
            stepSize: sldShadowTh.stepSize
            editable: true
            Accessible.name: qsTr("Shadow threshold hi")

            onValueChanged: Warhol.shadowThHi = Number(value)
        }
    }
    Label {
        id: txtShadowColor
        text: qsTr("Shadow color")
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(Warhol.shadowColor)
            title: qsTr("Choose the color of the shadow")
            showAlphaChannel: true
            Accessible.description: txtShadowColor.text

            onCurrentColorChanged: Warhol.shadowColor = AkUtils.toRgba(currentColor)
        }
    }
}
