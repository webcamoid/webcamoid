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
        target: ColorReplace

        function onRadiusChanged(radius)
        {
            sldRadius.value = radius
            spbRadius.value = radius
        }
    }

    // Color to replace.
    Label {
        id: txtOldColor
        text: qsTr("Old color")
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(ColorReplace.from)
            title: qsTr("Select the color to replace")
            modality: Qt.NonModal
            showAlphaChannel: true
            Accessible.description: txtOldColor.text

            onCurrentColorChanged: ColorReplace.from = AkUtils.toRgba(currentColor)
            onIsOpenChanged: ColorReplace.disable = isOpen
        }
    }

    // Color to replace.
    Label {
        id: txtNewColor
        text: qsTr("New color")
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
        }
        AK.ColorButton {
            currentColor: AkUtils.fromRgba(ColorReplace.to)
            title: qsTr("Select the new color")
            modality: Qt.NonModal
            showAlphaChannel: true
            Accessible.description: txtNewColor.text

            onCurrentColorChanged: ColorReplace.to = AkUtils.toRgba(currentColor)
            onIsOpenChanged: ColorReplace.disable = isOpen
        }
    }

    // Configure color selection radius.
    Label {
        id: lblRadius
        text: qsTr("Radius")
    }
    Slider {
        id: sldRadius
        value: ColorReplace.radius
        stepSize: 1
        to: 256
        Layout.fillWidth: true
        Accessible.name: lblRadius.text

        onValueChanged: ColorReplace.radius = value
    }
    SpinBox {
        id: spbRadius
        value: ColorReplace.radius
        to: sldRadius.to
        stepSize: sldRadius.stepSize
        editable: true
        Accessible.name: lblRadius.text

        onValueChanged: ColorReplace.radius = Number(value)
    }

    // Enable soft color replacing.
    Label {
        id: lblSoft
        text: qsTr("Soft")
    }
    RowLayout {
        Layout.columnSpan: 2

        Item {
            Layout.fillWidth: true
        }
        Switch {
            id: chkSoft
            checked: ColorReplace.soft
            Accessible.name: lblSoft.text

            onCheckedChanged: ColorReplace.soft = checked
        }
    }
}
