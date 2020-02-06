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
                        ThemeSettings.colorActiveHighlightedText:
                    control.flat?
                        ThemeSettings.colorActiveHighlight:
                        ThemeSettings.colorActiveButtonText
        text: control.text
        font: control.font
        color: control.highlighted?
                   ThemeSettings.colorActiveHighlightedText:
               control.flat?
                   ThemeSettings.colorActiveHighlight:
                   ThemeSettings.colorActiveButtonText
    }

    background: Rectangle {
        id: buttonRectangle
        implicitWidth: 2 * control.radius
        implicitHeight: 2 * control.radius
        radius: control.radius
        border.width:
            control.checkable?
                AkUnit.create(2 * ThemeSettings.controlScale, "dp").pixels:
                AkUnit.create(1 * ThemeSettings.controlScale, "dp").pixels
        border.color:
            control.checkable && control.checked && control.highlighted?
                ThemeSettings.colorActiveHighlightedText:
            control.checkable && control.checked?
                ThemeSettings.colorActiveHighlight:
            control.checkable?
                ThemeSettings.colorActiveDark:
            control.highlighted || control.flat?
                ThemeSettings.shade(ThemeSettings.colorActiveWindow, 0, 0):
                ThemeSettings.colorActiveDark
        color: control.highlighted?
                   ThemeSettings.colorActiveHighlight:
               control.flat?
                   ThemeSettings.shade(ThemeSettings.colorActiveWindow, 0, 0):
                   ThemeSettings.colorActiveButton
        gradient: Gradient {
            GradientStop {
                position: 0
                color: ThemeSettings.colorActiveWindow.hslLightness < 0.5?
                           Qt.tint(buttonRectangle.color,
                                   ThemeSettings.shade(ThemeSettings.colorActiveDark, 0, 0.25)):
                           Qt.tint(buttonRectangle.color,
                                   ThemeSettings.shade(ThemeSettings.colorActiveLight, 0, 0.25))
            }
            GradientStop {
                position: 1
                color: ThemeSettings.colorActiveWindow.hslLightness < 0.5?
                           Qt.tint(buttonRectangle.color,
                                   ThemeSettings.shade(ThemeSettings.colorActiveLight, 0, 0.25)):
                           Qt.tint(buttonRectangle.color,
                                   ThemeSettings.shade(ThemeSettings.colorActiveDark, 0, 0.25))
            }
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: iconLabel
                icon.color:
                    control.highlighted?
                        ThemeSettings.colorDisabledHighlightedText:
                    control.flat?
                        ThemeSettings.colorDisabledHighlight:
                        ThemeSettings.colorDisabledButtonText
                color:
                    control.highlighted?
                        ThemeSettings.colorDisabledHighlightedText:
                    control.flat?
                        ThemeSettings.colorDisabledHighlight:
                        ThemeSettings.colorDisabledButtonText
            }
            PropertyChanges {
                target: buttonRectangle
                border.color:
                    control.checkable && control.checked && control.highlighted?
                        ThemeSettings.colorDisabledHighlightedText:
                    control.checkable && control.checked?
                        ThemeSettings.colorDisabledHighlight:
                    control.checkable?
                        ThemeSettings.colorDisabledDark:
                    control.highlighted || control.flat?
                        ThemeSettings.shade(ThemeSettings.colorDisabledWindow, 0, 0):
                        ThemeSettings.colorDisabledDark
                color: control.highlighted?
                           ThemeSettings.colorDisabledHighlight:
                       control.flat?
                           ThemeSettings.shade(ThemeSettings.colorDisabledWindow, 0, 0):
                           ThemeSettings.colorDisabledButton
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
                           ThemeSettings.constShade(ThemeSettings.colorActiveHighlight, 0.1):
                       control.flat?
                           ThemeSettings.shade(ThemeSettings.colorActiveMid, 0, 0.5):
                           ThemeSettings.colorActiveMid
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
                        ThemeSettings.colorActiveHighlightedText:
                    control.checkable && control.checked?
                        ThemeSettings.colorActiveHighlight:
                    control.checkable?
                        ThemeSettings.colorActiveDark:
                    control.highlighted || control.flat?
                        ThemeSettings.shade(ThemeSettings.colorActiveWindow, 0, 0):
                        ThemeSettings.colorActiveHighlight
                color: control.highlighted?
                           ThemeSettings.constShade(ThemeSettings.colorActiveHighlight, 0.2):
                       control.flat?
                           ThemeSettings.shade(ThemeSettings.colorActiveMid, 0, 0.5):
                           ThemeSettings.colorActiveMid
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
                        ThemeSettings.colorActiveHighlightedText:
                    control.checkable && control.checked?
                        ThemeSettings.colorActiveHighlight:
                    control.checkable?
                        ThemeSettings.colorActiveDark:
                    control.highlighted || control.flat?
                        ThemeSettings.shade(ThemeSettings.colorActiveWindow, 0, 0):
                        ThemeSettings.colorActiveHighlight
                color: control.highlighted?
                           ThemeSettings.constShade(ThemeSettings.colorActiveHighlight, 0.3):
                       control.flat?
                           ThemeSettings.shade(ThemeSettings.colorActiveDark, 0, 0.5):
                           ThemeSettings.colorActiveDark
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
