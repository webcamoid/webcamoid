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
import QtQuick.Controls.impl 2.12
import Ak 1.0

T.MenuBarItem {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)
    rightPadding: AkUnit.create(16 * ThemeSettings.controlScale, "dp").pixels
    spacing: AkUnit.create(20 * ThemeSettings.controlScale, "dp").pixels
    icon.width: AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels
    icon.height: AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels
    clip: true
    hoverEnabled: true

    readonly property int animationTime: 200

    contentItem: IconLabel {
        id: iconLabel
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        icon.name: control.icon.name
        icon.source: control.icon.source
        icon.width: control.icon.width
        icon.height: control.icon.height
        icon.color: ThemeSettings.colorHighlightedText
        text: control.text
        font: control.font
        color: ThemeSettings.colorHighlightedText
        alignment: Qt.AlignLeft
    }

    background: Rectangle {
        id: background
        implicitWidth: AkUnit.create(64 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight: AkUnit.create(48 * ThemeSettings.controlScale, "dp").pixels
        color: control.highlighted?
                   ThemeSettings.constShade(ThemeSettings.colorHighlight, 0.1):
                   ThemeSettings.constShade(ThemeSettings.colorHighlight, 0, 0)
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: iconLabel
                opacity: 0.5
            }
            PropertyChanges {
                target: background
                color: control.highlighted?
                           ThemeSettings.shade(ThemeSettings.colorWindow, -0.3):
                           ThemeSettings.constShade(ThemeSettings.colorHighlight, 0, 0)
            }
        },
        State {
            name: "Hovered"
            when: control.enabled
                  && control.hovered
                  && !control.visualFocus
                  && !control.pressed

            PropertyChanges {
                target: background
                color:
                    control.highlighted?
                        ThemeSettings.constShade(ThemeSettings.colorHighlight, 0.2):
                        ThemeSettings.constShade(ThemeSettings.colorHighlight, 0.1)
            }
        },
        State {
            name: "Pressed"
            when: control.enabled
                  && control.pressed

            PropertyChanges {
                target: background
                color:
                    control.highlighted?
                        ThemeSettings.constShade(ThemeSettings.colorHighlight, 0.3):
                        ThemeSettings.constShade(ThemeSettings.colorHighlight, 0.2)
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: iconLabel
            properties: "opacity"
            duration: control.animationTime
        }
        ColorAnimation {
            target: background
            duration: control.animationTime
        }
    }
}
