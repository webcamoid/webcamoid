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

T.Button {
    id: control
    font.bold: true
    icon.width: AkUnit.create(18 * ThemeSettings.controlScale, "dp").pixels
    icon.height: AkUnit.create(18 * ThemeSettings.controlScale, "dp").pixels
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    spacing: AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
    hoverEnabled: true

    readonly property int radius:
        AkUnit.create(6 * ThemeSettings.controlScale, "dp").pixels
    readonly property int animationTime: 200

    contentItem: Item {
        id: buttonContent
        implicitWidth: iconLabel.implicitWidth
                       + AkUnit.create(18 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight: iconLabel.implicitHeight

        IconLabel {
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
            anchors.verticalCenter: buttonContent.verticalCenter
            anchors.horizontalCenter: buttonContent.horizontalCenter
        }
    }
    background: Item {
        id: back
        implicitWidth:
            AkUnit.create(64 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight:
            AkUnit.create(36 * ThemeSettings.controlScale, "dp").pixels

        // Rectangle
        Rectangle {
            id: buttonRectangle
            anchors.fill: parent
            radius: control.radius
            border.color:
                control.highlighted || control.flat?
                    ThemeSettings.shade(ThemeSettings.colorWindow, 0, 0):
                    ThemeSettings.colorDark
            border.width:
                AkUnit.create(1 * ThemeSettings.controlScale, "dp").pixels
            color: control.highlighted?
                       ThemeSettings.colorHighlight:
                   control.flat?
                       ThemeSettings.shade(ThemeSettings.colorWindow, 0, 0):
                       ThemeSettings.colorButton
        }

        // Checked indicator
        Rectangle {
            id: buttonCheckableIndicator
            height: control.radius
            color: control.checked && control.highlighted?
                       ThemeSettings.colorHighlightedText:
                   control.highlighted?
                       ThemeSettings.colorHighlight:
                       ThemeSettings.colorDark
            anchors.right: back.right
            anchors.bottom: back.bottom
            anchors.left: back.left
            visible: control.checkable
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: iconLabel
                icon.color: ThemeSettings.shade(ThemeSettings.colorWindow, -0.5)
                color: ThemeSettings.shade(ThemeSettings.colorWindow, -0.5)
            }
            PropertyChanges {
                target: buttonCheckableIndicator
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
                border.width:
                    AkUnit.create(2 * ThemeSettings.controlScale, "dp").pixels
                border.color:
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
                border.width:
                    AkUnit.create(2 * ThemeSettings.controlScale, "dp").pixels
                border.color:
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
            target: buttonRectangle
            properties: "color,border.color,border.width"
            duration: control.animationTime
        }
    }
}
