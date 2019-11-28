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

import QtQuick 2.0
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Templates 2.5 as T
import QtGraphicalEffects 1.0
import QtQuick.Controls.impl 2.12
import AkQml 1.0

T.TabButton {
    id: button
    font.bold: true
    icon.width: Ak.newUnit(24 * ThemeSettings.constrolScale, "dp").pixels
    icon.height: Ak.newUnit(24 * ThemeSettings.constrolScale, "dp").pixels
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    spacing: Ak.newUnit(8 * ThemeSettings.constrolScale, "dp").pixels
    hoverEnabled: true

    readonly property int animationTime: 200

    function pressIndicatorRadius()
    {
        let diffX = button.width / 2
        let diffY = button.height / 2
        let r2 = diffX * diffX + diffY * diffY

        return Math.sqrt(r2)
    }

    function buttonHeight()
    {
        let defaultHeight =
            Ak.newUnit(48 * ThemeSettings.constrolScale, "dp").pixels

        return Math.max(defaultHeight,
                        iconLabel.height
                        + Ak.newUnit(18 * ThemeSettings.constrolScale,
                                     "dp").pixels)
    }

    contentItem: Item {
        id: buttonContent
        implicitWidth:
            iconLabel.implicitWidth
            + (button.display == AbstractButton.IconOnly?
               0: Ak.newUnit(18 * ThemeSettings.constrolScale, "dp").pixels)
        implicitHeight: iconLabel.implicitHeight

        IconLabel {
            id: iconLabel
            spacing: button.spacing
            mirrored: button.mirrored
            display: button.display
            anchors.verticalCenter: buttonContent.verticalCenter
            anchors.horizontalCenter: buttonContent.horizontalCenter

            icon: button.icon
            text: button.text
            font: button.font
            color: ThemeSettings.contrast(ThemeSettings.colorBack)
        }
    }

    background: Item {
        id: back
        implicitWidth: Ak.newUnit(90 * ThemeSettings.constrolScale, "dp").pixels
        implicitHeight: button.buttonHeight()

        // Rectagle below the indicator
        Rectangle {
            id: buttonRectangleBelow
            anchors.fill: parent
            color: Qt.hsla(0, 0, 0, 0)
        }

        // Press indicator
        Rectangle{
            id: buttonPressIndicatorMask
            anchors.fill: parent
            color: Qt.hsla(0, 0, 0, 1)
            visible: false
        }
        Item {
            id: buttonPressIndicatorItem
            anchors.fill: buttonPressIndicatorMask
            clip: true
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: buttonPressIndicatorMask
            }

            Rectangle {
                id: buttonPress
                radius: 0
                anchors.verticalCenter:
                    buttonPressIndicatorItem.verticalCenter
                anchors.horizontalCenter:
                    buttonPressIndicatorItem.horizontalCenter
                width: 2 * radius
                height: 2 * radius
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.1,
                                                0.4)
                opacity: 0
            }
        }
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
                target: buttonPress
                radius: button.pressIndicatorRadius()
                opacity: 1
            }
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
                color: ThemeSettings.colorPrimary
            }
            PropertyChanges {
                target: buttonPress
                radius: button.pressIndicatorRadius()
                opacity: 1
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
        ColorAnimation {
            target: iconLabel
            duration: button.animationTime
        }
        PropertyAnimation {
            target: buttonPress
            properties: "radius"
            duration: button.animationTime
        }
        ColorAnimation {
            target: buttonRectangleBelow
            duration: button.animationTime
        }
    }
}
