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
import Ak 1.0
import "Private"

T.DelayButton {
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

    readonly property int animationTime: 200
    readonly property color activeButton: AkTheme.palette.active.button
    readonly property color activeButtonText: AkTheme.palette.active.buttonText
    readonly property color activeDark: AkTheme.palette.active.dark
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color activeLight: AkTheme.palette.active.light
    readonly property color activeMid: AkTheme.palette.active.mid
    readonly property color activeWindow: AkTheme.palette.active.window
    readonly property color disabledButton: AkTheme.palette.disabled.button
    readonly property color disabledButtonText: AkTheme.palette.disabled.buttonText
    readonly property color disabledDark: AkTheme.palette.disabled.dark

    transition: Transition {
        NumberAnimation {
            duration:
                control.delay
                * (control.pressed? 1.0 - control.progress: 0.3 * control.progress)
        }
    }

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
            iconColor: control.icon.color
            text: control.text
            font: control.font
            color: control.activeButtonText
            enabled: control.enabled
            anchors.verticalCenter: buttonContent.verticalCenter
            anchors.horizontalCenter: buttonContent.horizontalCenter
        }
    }
    background: Item {
        id: back
        implicitWidth: AkUnit.create(64 * AkTheme.controlScale, "dp").pixels
        implicitHeight: AkUnit.create(36 * AkTheme.controlScale, "dp").pixels

        // Rectangle
        Rectangle {
            id: buttonRectangle
            anchors.fill: parent
            radius: AkUnit.create(6 * AkTheme.controlScale, "dp").pixels
            border.width:
                AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
            border.color: control.activeDark
            color: control.activeButton
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
            color: control.activeDark
            anchors.bottom: back.bottom
            anchors.left: back.left
            anchors.right: back.right
        }
        Rectangle {
            width: parent.width * control.progress
            height: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
            color: control.activeHighlight
            anchors.bottom: back.bottom
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: iconLabel
                color: control.disabledButtonText
            }
            PropertyChanges {
                target: buttonCheckableIndicator
                color: control.disabledDark
            }
            PropertyChanges {
                target: buttonRectangle
                border.color: control.disabledDark
                color: control.disabledButton
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
                color: control.activeMid
            }
        },
        State {
            name: "Focused"
            when: control.enabled
                  && (control.activeFocus || control.visualFocus)
                  && !control.pressed

            PropertyChanges {
                target: buttonRectangle
                border.width: AkUnit.create(2 * AkTheme.controlScale,
                                            "dp").pixels
                border.color: control.activeHighlight
                color: control.activeMid
            }
        },
        State {
            name: "Pressed"
            when: control.enabled
                  && control.pressed

            PropertyChanges {
                target: buttonRectangle
                border.width: AkUnit.create(2 * AkTheme.controlScale,
                                            "dp").pixels
                border.color: control.activeHighlight
                color: control.activeDark
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
