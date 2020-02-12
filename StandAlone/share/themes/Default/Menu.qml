/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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
import QtQuick.Window 2.12
import Ak 1.0
import "Private"

T.Menu {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)
    margins: 0
    padding: 0
    transformOrigin:
        !cascade?
            Item.Top:
        mirrored?
            Item.TopRight:
            Item.TopLeft
    delegate: MenuItem { }

    // Fade in
    enter: Transition {
        NumberAnimation {
            property: "scale"
            from: 0.9
            to: 1.0
            easing.type: Easing.OutQuint
            duration: 220
        }
        NumberAnimation { property: "opacity"
            from: 0.0
            to: 1.0
            easing.type: Easing.OutCubic
            duration: 150
        }
    }

    // Fade out
    exit: Transition {
        NumberAnimation {
            property: "scale"
            from: 1.0
            to: 0.9
            easing.type: Easing.OutQuint
            duration: 220
        }
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            easing.type: Easing.OutCubic
            duration: 150
        }
    }

    contentItem: ListView {
        implicitHeight: contentHeight
        model: control.contentModel
        interactive: Window.window?
                         contentHeight > Window.window.height:
                         false
        clip: true
        currentIndex: control.currentIndex
        ScrollIndicator.vertical: ScrollIndicator {}
    }

    background: Rectangle {
        implicitWidth: AkUnit.create(128 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight: AkUnit.create(48 * ThemeSettings.controlScale, "dp").pixels
        color: enabled?
                   ThemeSettings.colorActiveWindow:
                   ThemeSettings.colorDisabledWindow
        border.color: enabled?
                          ThemeSettings.colorActiveDark:
                          ThemeSettings.colorDisabledDark
        radius: AkUnit.create(4 * ThemeSettings.controlScale, "dp").pixels
    }

    T.Overlay.modal: Rectangle {
        color: control.enabled?
                   ThemeSettings.shade(ThemeSettings.colorActiveDark, 0, 0.75):
                   ThemeSettings.shade(ThemeSettings.colorDisabledDark, 0, 0.75)

        Behavior on opacity { NumberAnimation { duration: 150 } }
    }

    T.Overlay.modeless: Rectangle {
        color: control.enabled?
                   ThemeSettings.shade(ThemeSettings.colorActiveDark, 0, 0.75):
                   ThemeSettings.shade(ThemeSettings.colorDisabledDark, 0, 0.75)

        Behavior on opacity { NumberAnimation { duration: 150 } }
    }
}
