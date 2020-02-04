/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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
import QtQuick.Templates 2.5 as T
import Ak 1.0

T.ToolTip {
    id: toolTip
    x: parent? (parent.width - implicitWidth) / 2: 0
    y: -implicitHeight - AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)
    padding: AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
    horizontalPadding: padding
                       + AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
    closePolicy: T.Popup.CloseOnEscape
                 | T.Popup.CloseOnPressOutsideParent
                 | T.Popup.CloseOnReleaseOutsideParent

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            easing.type: Easing.OutQuad
            duration: 150
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            easing.type: Easing.InQuad
            duration: 75
        }
    }

    contentItem: Text {
        text: toolTip.text
        font: toolTip.font
        color: ThemeSettings.colorWindow
    }

    background: Rectangle {
        implicitHeight: AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels
        color: ThemeSettings.colorWindowText
        radius: AkUnit.create(4 * ThemeSettings.controlScale, "dp").pixels
    }
}
