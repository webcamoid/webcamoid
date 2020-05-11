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
import "Private"

T.Button {
    id: control
    font.bold: true
    icon.width: AkUnit.create(18 * AkTheme.controlScale, "dp").pixels
    icon.height: AkUnit.create(18 * AkTheme.controlScale, "dp").pixels
    icon.color: activeButtonText
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
    spacing: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
    hoverEnabled: true

    readonly property int radius:
        AkUnit.create(6 * AkTheme.controlScale, "dp").pixels
    readonly property int animationTime: 200
    readonly property color activeButton: AkTheme.palette.active.button
    readonly property color activeButtonText: AkTheme.palette.active.buttonText
    readonly property color activeDark: AkTheme.palette.active.dark
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color activeHighlightedText: AkTheme.palette.active.highlightedText
    readonly property color activeLight: AkTheme.palette.active.light
    readonly property color activeMid: AkTheme.palette.active.mid
    readonly property color activeWindow: AkTheme.palette.active.window
    readonly property color disabledButton: AkTheme.palette.disabled.button
    readonly property color disabledButtonText: AkTheme.palette.disabled.buttonText
    readonly property color disabledDark: AkTheme.palette.disabled.dark
    readonly property color disabledHighlight: AkTheme.palette.disabled.highlight
    readonly property color disabledHighlightedText: AkTheme.palette.disabled.highlightedText
    readonly property color disabledWindow: AkTheme.palette.disabled.window

    contentItem: Item {
        id: buttonContent
        implicitWidth:
            iconLabel.implicitWidth
            + AkUnit.create(18 * AkTheme.controlScale, "dp").pixels
        implicitHeight: iconLabel.implicitHeight

        IconLabel {
            id: iconLabel
            spacing: control.spacing
            mirrored: control.mirrored
            display: control.display
            iconName: control.icon.name
            iconSource: control.icon.source
            iconWidth: control.icon.width
            iconHeight: control.icon.height
            iconColor:
                control.highlighted?
                    control.activeHighlightedText:
                control.flat?
                    control.activeHighlight:
                    control.icon.color
            text: control.text
            font: control.font
            color:
                control.highlighted?
                    control.activeHighlightedText:
                control.flat?
                    control.activeHighlight:
                    control.activeButtonText
            enabled: control.enabled
            anchors.verticalCenter: buttonContent.verticalCenter
            anchors.horizontalCenter: buttonContent.horizontalCenter
        }
    }
    background: Item {
        id: back
        implicitWidth:
            AkUnit.create(64 * AkTheme.controlScale, "dp").pixels
        implicitHeight:
            AkUnit.create(36 * AkTheme.controlScale, "dp").pixels

        // Rectangle
        Rectangle {
            id: buttonRectangle
            anchors.fill: parent
            radius: AkUnit.create(6 * AkTheme.controlScale, "dp").pixels
            border.width:
                AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
            border.color:
                control.highlighted || control.flat?
                    AkTheme.shade(control.activeWindow, 0, 0):
                    control.activeDark
            color: control.highlighted?
                       control.activeHighlight:
                   control.flat?
                       AkTheme.shade(control.activeWindow, 0, 0):
                       control.activeButton

            gradient: Gradient {
                GradientStop {
                    position: 0
                    color: control.activeWindow.hslLightness < 0.5?
                               Qt.tint(buttonRectangle.color,
                                       AkTheme.shade(control.activeDark, 0, 0.25)):
                               Qt.tint(buttonRectangle.color,
                                       AkTheme.shade(control.activeLight, 0, 0.25))
                }
                GradientStop {
                    position: 1
                    color: control.activeWindow.hslLightness < 0.5?
                               Qt.tint(buttonRectangle.color,
                                       AkTheme.shade(control.activeLight, 0, 0.25)):
                               Qt.tint(buttonRectangle.color,
                                       AkTheme.shade(control.activeDark, 0, 0.25))
                }
            }
        }

        // Checked indicator
        Rectangle {
            id: buttonCheckableIndicator
            height: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
            color:
                control.checkable && control.checked && control.highlighted?
                    control.activeHighlightedText:
                control.checkable && control.checked?
                    control.activeHighlight:
                control.checkable?
                    control.activeDark:
                control.highlighted || control.flat?
                    AkTheme.shade(control.activeWindow, 0, 0):
                    control.activeDark
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
                color: control.highlighted?
                           control.disabledHighlightedText:
                       control.flat?
                           control.disabledHighlight:
                           control.disabledButtonText
            }
            PropertyChanges {
                target: buttonCheckableIndicator
                color:
                    control.checkable && control.checked && control.highlighted?
                        control.disabledHighlightedText:
                    control.checkable && control.checked?
                        control.disabledHighlight:
                    control.checkable?
                        control.disabledDark:
                    control.highlighted || control.flat?
                        AkTheme.shade(control.disabledWindow, 0, 0):
                        control.disabledDark
            }
            PropertyChanges {
                target: buttonRectangle
                border.color:
                    control.highlighted || control.flat?
                        AkTheme.shade(control.disabledWindow, 0, 0):
                        control.disabledDark
                color: control.highlighted?
                           control.disabledHighlight:
                       control.flat?
                           AkTheme.shade(control.disabledWindow, 0, 0):
                           control.disabledButton
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
                           AkTheme.constShade(control.activeHighlight, 0.1):
                       control.flat?
                           AkTheme.shade(control.activeMid, 0, 0.5):
                           control.activeMid
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
                    AkUnit.create(2 * AkTheme.controlScale, "dp").pixels
                border.color:
                    control.highlighted || control.flat?
                        AkTheme.shade(control.activeWindow, 0, 0):
                        control.activeHighlight
                color: control.highlighted?
                           AkTheme.constShade(control.activeHighlight, 0.2):
                       control.flat?
                           AkTheme.shade(control.activeMid, 0, 0.5):
                           control.activeMid
            }
        },
        State {
            name: "Pressed"
            when: control.enabled
                  && control.pressed

            PropertyChanges {
                target: buttonRectangle
                border.width:
                    AkUnit.create(2 * AkTheme.controlScale, "dp").pixels
                border.color:
                    control.highlighted || control.flat?
                        AkTheme.shade(control.activeWindow, 0, 0):
                        control.activeHighlight
                color: control.highlighted?
                           AkTheme.constShade(control.activeHighlight, 0.3):
                       control.flat?
                           AkTheme.shade(control.activeDark, 0, 0.5):
                           control.activeDark
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
