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
import QtQuick.Controls.impl 2.12
import Ak 1.0

T.RoundButton {
    id: control
    font.bold: true
    icon.width: AkUnit.create(18 * ThemeSettings.controlScale, "dp").pixels
    icon.height: AkUnit.create(18 * ThemeSettings.controlScale, "dp").pixels
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    spacing: AkUnit.create(12 * ThemeSettings.controlScale, "dp").pixels
    hoverEnabled: true
    leftPadding:
        AkUnit.create(16 * ThemeSettings.controlScale, "dp").pixels
    rightPadding:
        AkUnit.create(16 * ThemeSettings.controlScale, "dp").pixels
    radius: AkUnit.create(28 * ThemeSettings.controlScale, "dp").pixels

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
        icon.color: control.highlighted?
                        ThemeSettings.colorHighlightedText:
                    control.flat?
                        ThemeSettings.colorHighlight:
                        ThemeSettings.colorButtonText
        text: control.text
        font: control.font
        color: control.highlighted?
                   ThemeSettings.colorHighlightedText:
               control.flat?
                   ThemeSettings.colorHighlight:
                   ThemeSettings.colorButtonText
    }

    background: Rectangle {
        id: buttonRectangle
        implicitWidth: 2 * control.radius
        implicitHeight: 2 * control.radius
        radius: control.radius
        border.color:
            control.checkable && control.checked && control.highlighted?
                ThemeSettings.colorHighlightedText:
            control.checkable && control.checked?
                ThemeSettings.colorHighlight:
            control.checkable?
                ThemeSettings.colorDark:
            control.highlighted || control.flat?
                ThemeSettings.shade(ThemeSettings.colorWindow, 0, 0):
                ThemeSettings.colorDark
        border.width:
            control.checkable?
                AkUnit.create(2 * ThemeSettings.controlScale, "dp").pixels:
                AkUnit.create(1 * ThemeSettings.controlScale, "dp").pixels
        color: control.highlighted?
                   ThemeSettings.colorHighlight:
               control.flat?
                   ThemeSettings.shade(ThemeSettings.colorWindow, 0, 0):
                   ThemeSettings.colorButton
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: buttonRectangle
                border.color: ThemeSettings.shade(ThemeSettings.colorWindow, -0.5)
            }
            PropertyChanges {
                target: iconLabel
                icon.color: ThemeSettings.shade(ThemeSettings.colorWindow, -0.5)
                color: ThemeSettings.shade(ThemeSettings.colorWindow, -0.5)
            }
        },
        State {
            name: "Hovered"
            when: control.enabled
                  && control.hovered
                  && !(control.activeFocus || control.visualFocus)
                  && !control.pressed

            PropertyChanges {
                target: buttonRectangle
                color: control.highlighted?
                           ThemeSettings.constShade(ThemeSettings.colorHighlight, 0.1):
                       control.flat?
                           ThemeSettings.shade(ThemeSettings.colorMid, 0, 0.5):
                           ThemeSettings.colorMid
            }
        },
        State {
            name: "Focused"
            when: control.enabled
                  && (control.activeFocus || control.visualFocus)
                  && !control.pressed

            PropertyChanges {
                target: buttonRectangle
                border.color:
                    control.checkable && control.checked && control.highlighted?
                        ThemeSettings.colorHighlightedText:
                    control.checkable && control.checked?
                        ThemeSettings.colorHighlight:
                    control.checkable?
                        ThemeSettings.colorDark:
                    control.highlighted || control.flat?
                        ThemeSettings.shade(ThemeSettings.colorWindow, 0, 0):
                        ThemeSettings.colorHighlight
                color: control.highlighted?
                           ThemeSettings.constShade(ThemeSettings.colorHighlight, 0.2):
                       control.flat?
                           ThemeSettings.shade(ThemeSettings.colorMid, 0, 0.5):
                           ThemeSettings.colorMid
            }
        },
        State {
            name: "Pressed"
            when: control.enabled
                  && control.pressed

            PropertyChanges {
                target: buttonRectangle
                border.color:
                    control.checkable && control.checked && control.highlighted?
                        ThemeSettings.colorHighlightedText:
                    control.checkable && control.checked?
                        ThemeSettings.colorHighlight:
                    control.checkable?
                        ThemeSettings.colorDark:
                    control.highlighted || control.flat?
                        ThemeSettings.shade(ThemeSettings.colorWindow, 0, 0):
                        ThemeSettings.colorHighlight
                color: control.highlighted?
                           ThemeSettings.constShade(ThemeSettings.colorHighlight, 0.3):
                       control.flat?
                           ThemeSettings.shade(ThemeSettings.colorDark, 0, 0.5):
                           ThemeSettings.colorDark
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: iconLabel
            properties: "color,icon.color"
            duration: control.animationTime
        }

        PropertyAnimation {
            target: buttonRectangle
            properties: "border.color,border.width,color"
            duration: control.animationTime
        }
    }
}
