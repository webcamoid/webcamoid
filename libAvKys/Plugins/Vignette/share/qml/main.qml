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

    RowLayout {
        layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight

        Label {
            id: txtColor
            text: qsTr("Color")
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(Vignette.color)
            title: qsTr("Choose the vignette color")
            showAlphaChannel: true
            Accessible.description: txtColor.text

            onCurrentColorChanged: Vignette.color = AkUtils.toRgba(currentColor)
        }
    }

    Label {
        id: lblAspect
        //: Aspect ratio
        text: qsTr("Aspect")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldAspect
        value: Vignette.aspect
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblAspect.text

        onValueChanged: Vignette.aspect = value
    }

    Label {
        id: lblScale
        text: qsTr("Scale")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldScale
        value: Vignette.scale
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblScale.text

        onValueChanged: Vignette.scale = value
    }

    Label {
        id: lblSoftness
        text: qsTr("Softness")
        font.bold: true
        Layout.fillWidth: true
    }
    Slider {
        id: sldSoftness
        value: Vignette.softness
        stepSize: 0.01
        to: 1
        Layout.fillWidth: true
        Accessible.name: lblSoftness.text

        onValueChanged: Vignette.softness = value
    }
}
