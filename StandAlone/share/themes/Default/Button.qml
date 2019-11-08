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

T.Button {
    id: button
    font.bold: true
    icon.width: Ak.newUnit(18 * ThemeSettings.constrolScale, "dp").pixels
    icon.height: Ak.newUnit(18 * ThemeSettings.constrolScale, "dp").pixels
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    spacing: Ak.newUnit(8 * ThemeSettings.constrolScale, "dp").pixels

    readonly property int radius: Ak.newUnit(6 * ThemeSettings.constrolScale,
                                             "dp").pixels
    readonly property int animationTime: 200

    function pressIndicatorRadius()
    {
        let diffX = button.width / 2
        let diffY = button.height / 2
        let r2 = diffX * diffX + diffY * diffY

        return Math.sqrt(r2)
    }

    contentItem: Item {
        id: buttonContent
        implicitWidth: iconLabel.implicitWidth
                       + Ak.newUnit(18 * ThemeSettings.constrolScale,
                                    "dp").pixels
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
            color: button.highlighted?
                       ThemeSettings.colorText:
                       Qt.lighter(ThemeSettings.colorPrimary, 1.5)
        }
    }
    background: Item {
        id: back
        implicitWidth: Ak.newUnit(64 * ThemeSettings.constrolScale,
                                  "dp").pixels
        implicitHeight: Ak.newUnit(36 * ThemeSettings.constrolScale,
                                   "dp").pixels

        // Shadow
        Rectangle {
            id: buttonShadowRect
            anchors.fill: parent
            radius: button.radius
            color: Qt.hsla(0, 0, 0, 1)
            visible: false
        }
        DropShadow {
            id: buttonShadow
            anchors.fill: parent
            cached: true
            horizontalOffset: button.radius / 2
            verticalOffset: button.radius / 2
            radius: button.radius
            samples: 2 * radius + 1
            color: Qt.lighter(ThemeSettings.colorBack, 0.01)
            source: buttonShadowRect
            visible: button.highlighted && button.enabled
        }

        // Rectagle below the indicator
        Rectangle {
            id: buttonRectangleBelow
            anchors.fill: parent
            radius: button.radius
            color: ThemeSettings.colorPrimary
            visible: false
        }

        // Press indicator
        Rectangle{
            id: buttonPressIndicatorMask
            anchors.fill: parent
            radius: button.radius
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
                anchors.verticalCenter: buttonPressIndicatorItem.verticalCenter
                anchors.horizontalCenter: buttonPressIndicatorItem.horizontalCenter
                width: 2 * radius
                height: 2 * radius
                color: ThemeSettings.lighterAlpha(ThemeSettings.colorPrimary,
                                                  1.5,
                                                  0.3)
                opacity: 0
            }
        }

        // Rectangle
        Rectangle {
            id: buttonRectangle
            anchors.fill: parent
            radius: button.radius
            border.color: button.highlighted || button.flat?
                              Qt.hsla(0, 0, 0, 0):
                              ThemeSettings.colorBack
            border.width: Ak.newUnit(1 * ThemeSettings.constrolScale,
                                     "dp").pixels
            color: button.highlighted?
                       ThemeSettings.colorPrimary:
                       Qt.hsla(0, 0, 0, 0)
        }

        // Checked indicator
        Rectangle {
            id: buttonCheckableIndicator
            height: button.radius
            color: button.checked?
                       Qt.lighter(ThemeSettings.colorPrimary, 1.5):
                       Qt.lighter(ThemeSettings.colorPrimary, 0.5)
            anchors.right: back.right
            anchors.bottom: back.bottom
            anchors.left: back.left
            visible: button.checkable
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !button.enabled
                  && !button.highlighted
                  && !button.flat
                  && !button.hovered
                  && !button.visualFocus
                  && !button.pressed

            PropertyChanges {
                target: iconLabel
                color: ThemeSettings.colorBack
            }
            PropertyChanges {
                target: buttonCheckableIndicator
                color: Qt.lighter(ThemeSettings.colorBack, 0.3)
            }
        },
        State {
            name: "Hovered"
            when: button.enabled
                  && !button.highlighted
                  && !button.flat
                  && button.hovered
                  && !button.visualFocus
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangle
                color: ThemeSettings.lighterAlpha(ThemeSettings.colorPrimary,
                                                  1.5,
                                                  0.2)
            }
        },
        State {
            name: "Focused"
            when: button.enabled
                  && !button.highlighted
                  && !button.flat
                  && (button.hovered || button.visualFocus)
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangle
                color: ThemeSettings.lighterAlpha(ThemeSettings.colorPrimary,
                                                  1.5,
                                                  0.3)
            }
        },
        State {
            name: "Pressed"
            when: button.enabled
                  && !button.highlighted
                  && !button.flat
                  && button.pressed

            PropertyChanges {
                target: buttonPress
                radius: button.pressIndicatorRadius()
                opacity: 1
            }
        },
        State {
            name: "FlatDisabled"
            when: !button.enabled
                  && !button.highlighted
                  && button.flat
                  && !button.hovered
                  && !button.visualFocus
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangle
                border.color: Qt.hsla(0, 0, 0, 0)
            }
            PropertyChanges {
                target: iconLabel
                color: ThemeSettings.colorBack
            }
            PropertyChanges {
                target: buttonCheckableIndicator
                color: Qt.lighter(ThemeSettings.colorBack, 0.3)
            }
        },
        State {
            name: "FlatHovered"
            when: button.enabled
                  && !button.highlighted
                  && button.flat
                  && button.hovered
                  && !button.visualFocus
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangle
                color: ThemeSettings.lighterAlpha(ThemeSettings.colorPrimary,
                                                  1.5,
                                                  0.2)
                border.color: Qt.hsla(0, 0, 0, 0)
            }
        },
        State {
            name: "FlatFocused"
            when: button.enabled
                  && !button.highlighted
                  && button.flat
                  && (button.hovered || button.visualFocus)
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangle
                color: ThemeSettings.lighterAlpha(ThemeSettings.colorPrimary,
                                                  1.5,
                                                  0.3)
                border.color: Qt.hsla(0, 0, 0, 0)
            }
        },
        State {
            name: "FlatPressed"
            when: button.enabled
                  && !button.highlighted
                  && button.flat
                  && button.pressed

            PropertyChanges {
                target: buttonRectangle
                border.color: Qt.hsla(0, 0, 0, 0)
            }
            PropertyChanges {
                target: buttonPress
                radius: button.pressIndicatorRadius()
                opacity: 1
            }
        },
        State {
            name: "HighlightedDisabled"
            when: !button.enabled
                  && button.highlighted
                  && !button.hovered
                  && !button.visualFocus
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangle
                border.color: Qt.hsla(0, 0, 0, 0)
                color: ThemeSettings.colorBack

            }
            PropertyChanges {
                target: iconLabel
                color: Qt.lighter(ThemeSettings.colorBack, 0.3)
            }
            PropertyChanges {
                target: buttonCheckableIndicator
                color: Qt.lighter(ThemeSettings.colorBack, 0.3)
            }
        },
        State {
            name: "HighlightedHovered"
            when: button.enabled
                  && button.highlighted
                  && button.hovered
                  && !button.visualFocus
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangle
                border.color: Qt.hsla(0, 0, 0, 0)
                color: Qt.lighter(ThemeSettings.colorPrimary, 1.2)

            }
            PropertyChanges {
                target: iconLabel
                color: ThemeSettings.colorText
            }
        },
        State {
            name: "HighlightedFocused"
            when: button.enabled
                  && button.highlighted
                  && (button.hovered || button.visualFocus)
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangle
                border.color: Qt.hsla(0, 0, 0, 0)
                color: Qt.lighter(ThemeSettings.colorPrimary, 1.5)

            }
            PropertyChanges {
                target: iconLabel
                color: ThemeSettings.colorText
            }
        },
        State {
            name: "HighlightedPressed"
            when: button.enabled
                  && button.highlighted
                  && button.pressed

            PropertyChanges {
                target: buttonPress
                radius: button.pressIndicatorRadius()
                opacity: 1
            }
            PropertyChanges {
                target: buttonRectangle
                visible: false
            }
            PropertyChanges {
                target: buttonRectangleBelow
                visible: true
            }
            PropertyChanges {
                target: iconLabel
                color: ThemeSettings.colorText
            }
            PropertyChanges {
                target: buttonShadow
                radius: 2 * button.radius
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: buttonPress
            properties: "radius"
            duration: button.animationTime
        }
        ColorAnimation {
            target: buttonRectangle
            duration: button.animationTime
        }
    }
}
