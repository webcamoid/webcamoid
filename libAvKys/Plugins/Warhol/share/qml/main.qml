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

ColumnLayout {
    id: root
    layoutDirection: rtl? Qt.RightToLeft: Qt.LeftToRight

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    Label {
        text: qsTr("Palette")
        topPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        bottomPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        wrapMode: Text.WordWrap
        elide: Text.ElideNone
        font: AkTheme.fontSettings.h6
        Layout.fillWidth: true
    }
    Label {
        id: txtFrameLen
        text: qsTr("Frame length")
        font.bold: true
        Layout.fillWidth: true
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
    Label {
        id: txtLevels
        text: qsTr("Levels")
        font.bold: true
        Layout.fillWidth: true
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
    Label {
        id: txtSaturation
        text: qsTr("Saturation")
        font.bold: true
        Layout.fillWidth: true
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
    Label {
        id: txtLuminance
        text: qsTr("Luminance")
        font.bold: true
        Layout.fillWidth: true
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
    Label {
        id: txtPaletteOffset
        text: qsTr("Hue offset")
        font.bold: true
        Layout.fillWidth: true
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

    Label {
        text: qsTr("Shadow")
        topPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        bottomPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        wrapMode: Text.WordWrap
        elide: Text.ElideNone
        font: AkTheme.fontSettings.h6
        Layout.fillWidth: true
    }
    Label {
        id: txtShadowThLow
        text: qsTr("Shadow threshold")
        font.bold: true
        Layout.fillWidth: true
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
    RowLayout {
        layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight

        Label {
            id: txtShadowColor
            text: qsTr("Shadow color")
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
