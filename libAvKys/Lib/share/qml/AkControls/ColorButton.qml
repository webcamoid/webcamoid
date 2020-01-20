/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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
import QtQuick.Window 2.12
import Qt.labs.platform 1.1 as LABS
import QtGraphicalEffects 1.0

Button {
    id: button
    text: currentColor
    layer.enabled: enabled
    layer.effect: ColorOverlay {
        cached: true
        color: Qt.rgba(currentColor.r, currentColor.g, currentColor.b, 0.5)
    }

    property color currentColor: "black"
    property string title: ""
    property bool showAlphaChannel: false
    property int modality: Qt.ApplicationModal
    property bool isOpen: false

    onClicked: colorDialog.open()

    LABS.ColorDialog {
        id: colorDialog
        title: button.title
        currentColor: button.currentColor
        options: button.showAlphaChannel? LABS.ColorDialog.ShowAlphaChannel: 0
        modality: button.modality

        onAccepted: button.currentColor = color
        onVisibleChanged: button.isOpen = visible
    }
}
