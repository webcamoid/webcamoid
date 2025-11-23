/* Webcamoid, camera capture application.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
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

RowLayout {
    property alias icon: button.icon
    property alias font: label.font
    property alias labelText: label.text
    property alias placeholderText: label.placeholderText
    property alias buttonText: button.text

    signal buttonClicked()

    TextField {
        id: label
        selectByMouse: true
        Accessible.name: placeholderText
        Layout.fillWidth: true
    }
    Button {
        id: button
        icon.source: parent.iconSource
        flat: true
        display: AbstractButton.IconOnly
        implicitWidth: implicitHeight
        ToolTip.visible: hovered
        ToolTip.text: text
        Accessible.name: text
        Accessible.description: text

        onClicked: parent.buttonClicked()
    }
}
