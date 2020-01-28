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

T.TabButton {
    id: button
    font.bold: true
    icon.width:
        button.display == AbstractButton.IconOnly
        || button.highlighted?
            0.8 * Math.min(width, height):
            0.375 * Math.min(width, height)
    icon.height:
        button.display == AbstractButton.IconOnly
        || button.highlighted?
            0.8 * Math.min(width, height):
            0.375 * Math.min(width, height)
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    spacing: AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
    hoverEnabled: true

    readonly property int animationTime: 200

    function buttonHeight()
    {
        let defaultHeight =
            AkUnit.create(48 * ThemeSettings.controlScale, "dp").pixels

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
            icon.color: ThemeSettings.colorText
            text: button.text
            font: button.font
            color: ThemeSettings.colorText
        }
    }

    background: Rectangle {
        id: buttonRectangleBelow
        implicitWidth: AkUnit.create(90 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight: button.buttonHeight()
        color: ThemeSettings.shade(ThemeSettings.colorBack, 0.0, 0.0)
    }

    states: [
        State {
            name: "Disabled"
            when: !button.enabled

            PropertyChanges {
                target: iconLabel
                opacity: 0.5
            }
        },
        State {
            name: "Hovered"
            when: button.enabled
                  && !button.checked
                  && (button.hovered
                      || button.visualFocus
                      || button.activeFocus)
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangleBelow
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.0,
                                                0.1)
            }
        },
        State {
            name: "Pressed"
            when: button.enabled
                  && !button.checked
                  && button.pressed

            PropertyChanges {
                target: buttonRectangleBelow
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.0,
                                                0.2)
            }
        },
        State {
            name: "Checked"
            when: button.enabled
                  && button.checked
                  && !(button.hovered
                       || button.visualFocus
                       || button.activeFocus)
                  && !button.pressed

            PropertyChanges {
                target: iconLabel
                icon.color: ThemeSettings.colorPrimary
                color: ThemeSettings.colorPrimary
            }
        },
        State {
            name: "CheckedHovered"
            when: button.enabled
                  && button.checked
                  && (button.hovered
                      || button.visualFocus
                      || button.activeFocus)
                  && !button.pressed

            PropertyChanges {
                target: iconLabel
                icon.color: ThemeSettings.colorPrimary
                color: ThemeSettings.colorPrimary
            }
            PropertyChanges {
                target: buttonRectangleBelow
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.0,
                                                0.3)
            }
        },
        State {
            name: "CheckedPressed"
            when: button.enabled
                  && button.checked
                  && button.pressed

            PropertyChanges {
                target: iconLabel
                icon.color: ThemeSettings.colorPrimary
                color: ThemeSettings.colorPrimary
            }
            PropertyChanges {
                target: buttonRectangleBelow
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.0,
                                                0.4)
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: iconLabel
            properties: "color,icon.color"
            duration: button.animationTime
        }
        ColorAnimation {
            target: buttonRectangleBelow
            duration: button.animationTime
        }
    }
}
