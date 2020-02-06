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

T.ToolButton {
    id: button
    font.bold: true
    icon.width:
        button.display == AbstractButton.IconOnly?
            0.8 * Math.min(width, height):
            0.375 * Math.min(width, height)
    icon.height:
        button.display == AbstractButton.IconOnly?
            0.8 * Math.min(width, height):
            0.375 * Math.min(width, height)
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
    spacing: AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
    hoverEnabled: true

    readonly property int animationTime: 200

    function buttonWidth()
    {
        if (button.display == AbstractButton.IconOnly)
            return AkUnit.create(48 * ThemeSettings.controlScale, "dp").pixels

        return AkUnit.create(64 * ThemeSettings.controlScale, "dp").pixels
    }

    function buttonHeight()
    {
        if (button.display == AbstractButton.IconOnly)
            return AkUnit.create(48 * ThemeSettings.controlScale, "dp").pixels

        let defaultHeight =
            button.highlighted?
                AkUnit.create(48 * ThemeSettings.controlScale, "dp").pixels:
                AkUnit.create(36 * ThemeSettings.controlScale, "dp").pixels

        return Math.max(defaultHeight,
                        iconLabel.height
                        + AkUnit.create(18 * ThemeSettings.controlScale,
                                     "dp").pixels)
    }

    contentItem: Item {
        id: buttonContent
        implicitWidth:
            iconLabel.implicitWidth
            + (button.display == AbstractButton.IconOnly?
               0: AkUnit.create(18 * ThemeSettings.controlScale, "dp").pixels)
        implicitHeight: iconLabel.implicitHeight

        IconLabel {
            id: iconLabel
            spacing: button.spacing
            mirrored: button.mirrored
            display: button.display
            anchors.verticalCenter: buttonContent.verticalCenter
            anchors.horizontalCenter: buttonContent.horizontalCenter
            icon.name: button.icon.name
            icon.source: button.icon.source
            icon.width: button.icon.width
            icon.height: button.icon.height
            icon.color: ThemeSettings.colorActiveHighlightedText
            text: button.text
            font: button.font
            color: ThemeSettings.colorActiveHighlightedText
        }
    }
    background: Rectangle {
        id: buttonRectangleBelow
        implicitWidth: button.buttonWidth()
        implicitHeight: button.buttonHeight()
        color: button.highlighted?
                   ThemeSettings.constShade(ThemeSettings.colorActiveHighlight, 0.1):
                   ThemeSettings.constShade(ThemeSettings.colorActiveHighlight, 0, 0)
    }

    states: [
        State {
            name: "Disabled"
            when: !button.enabled

            PropertyChanges {
                target: iconLabel
                icon.color: ThemeSettings.colorDisabledHighlightedText
                color: ThemeSettings.colorDisabledHighlightedText
            }
            PropertyChanges {
                target: buttonRectangleBelow
                color: button.highlighted?
                           ThemeSettings.constShade(ThemeSettings.colorDisabledHighlight, 0.1):
                           ThemeSettings.constShade(ThemeSettings.colorDisabledHighlight, 0, 0)
            }
        },
        State {
            name: "Hovered"
            when: button.enabled
                  && (button.hovered || button.visualFocus)
                  && !button.checked
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangleBelow
                color:
                    button.highlighted?
                        ThemeSettings.constShade(ThemeSettings.colorActiveHighlight, 0.2):
                        ThemeSettings.constShade(ThemeSettings.colorActiveHighlight, 0.1)
            }
        },
        State {
            name: "Checked"
            when: button.enabled
                  && button.checked
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangleBelow
                color:
                    ThemeSettings.constShade(ThemeSettings.colorActiveHighlight, 0.2)
            }
        },
        State {
            name: "Pressed"
            when: button.pressed

            PropertyChanges {
                target: buttonRectangleBelow
                color:
                    ThemeSettings.constShade(ThemeSettings.colorActiveHighlight, 0.3)
            }
        }
    ]

    transitions: Transition {
        ColorAnimation {
            target: buttonRectangleBelow
            duration: button.animationTime
        }
    }
}
