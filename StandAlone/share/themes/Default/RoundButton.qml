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
import QtQuick.Layouts 1.3
import QtQuick.Templates 2.5 as T
import QtGraphicalEffects 1.0
import QtQuick.Controls.impl 2.12
import Ak 1.0

T.RoundButton {
    id: roundButton
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

    function pressIndicatorRadius()
    {
        let diffX = roundButton.width / 2
        let diffY = roundButton.height / 2
        let r2 = diffX * diffX + diffY * diffY

        return Math.sqrt(r2)
    }

    contentItem: IconLabel {
        id: iconLabel
        spacing: roundButton.spacing
        mirrored: roundButton.mirrored
        display: roundButton.display
        icon.name: roundButton.icon.name
        icon.source: roundButton.icon.source
        icon.width: roundButton.icon.width
        icon.height: roundButton.icon.height
        icon.color: roundButton.highlighted?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                        ThemeSettings.colorPrimary
        text: roundButton.text
        font: roundButton.font
        color: roundButton.highlighted?
                   ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                   ThemeSettings.colorPrimary
    }

    background: Item {
        id: back
        implicitWidth: 2 * roundButton.radius
        implicitHeight: 2 * roundButton.radius

        // Shadow
        Rectangle {
            id: buttonShadowRect
            width: roundButton.width
            height: roundButton.height
            radius: roundButton.radius
            color: Qt.hsla(0, 0, 0, 1)
            visible: false
        }
        DropShadow {
            id: buttonShadow
            anchors.fill: parent
            cached: true
            horizontalOffset: AkUnit.create(3 * ThemeSettings.controlScale, "dp").pixels
            verticalOffset: AkUnit.create(3 * ThemeSettings.controlScale, "dp").pixels
            radius: AkUnit.create(6 * ThemeSettings.controlScale, "dp").pixels
            samples: 2 * radius + 1
            color: ThemeSettings.constShade(ThemeSettings.colorBack, -0.9)
            source: buttonShadowRect
            visible: roundButton.highlighted && roundButton.enabled
        }

        // Rectagle below the indicator
        Rectangle {
            id: buttonRectangleBelow
            anchors.fill: parent
            radius: roundButton.radius
            color: ThemeSettings.colorPrimary
            visible: false
        }

        // Press indicator
        Rectangle{
            id: buttonPressIndicatorMask
            anchors.fill: parent
            radius: roundButton.radius
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
                color: roundButton.highlighted?
                           ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                    0.3,
                                                    0.3):
                           ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                    0.1,
                                                    0.3)
                opacity: 0
            }
        }

        // Rectangle
        Rectangle {
            id: buttonRectangle
            anchors.fill: parent
            radius: roundButton.radius
            border.color:
                roundButton.checked && roundButton.highlighted?
                    ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                roundButton.checkable && roundButton.highlighted?
                    "grey":
                roundButton.checked?
                    ThemeSettings.colorPrimary:
                roundButton.checkable?
                    ThemeSettings.constShade(ThemeSettings.colorPrimary, -0.3):
                roundButton.highlighted || roundButton.flat?
                    "transparent":
                    ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            border.width:
                roundButton.checkable?
                    AkUnit.create(2 * ThemeSettings.controlScale, "dp").pixels:
                    AkUnit.create(1 * ThemeSettings.controlScale, "dp").pixels
            color: roundButton.highlighted?
                       ThemeSettings.colorPrimary:
                       "transparent"
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !roundButton.enabled
                  && !roundButton.highlighted
                  && !roundButton.flat
                  && !roundButton.hovered
                  && !roundButton.visualFocus
                  && !roundButton.pressed

            PropertyChanges {
                target: buttonRectangle
                border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            }
            PropertyChanges {
                target: iconLabel
                icon.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            }
        },
        State {
            name: "Hovered"
            when: roundButton.enabled
                  && !roundButton.highlighted
                  && !roundButton.flat
                  && roundButton.hovered
                  && !roundButton.visualFocus
                  && !roundButton.pressed

            PropertyChanges {
                target: buttonRectangle
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.1,
                                                0.2)
            }
        },
        State {
            name: "Focused"
            when: roundButton.enabled
                  && !roundButton.highlighted
                  && !roundButton.flat
                  && (roundButton.hovered || roundButton.visualFocus)
                  && !roundButton.pressed

            PropertyChanges {
                target: buttonRectangle
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.1,
                                                0.3)
            }
        },
        State {
            name: "Pressed"
            when: roundButton.enabled
                  && !roundButton.highlighted
                  && !roundButton.flat
                  && roundButton.pressed

            PropertyChanges {
                target: buttonPress
                radius: roundButton.pressIndicatorRadius()
                opacity: 1
            }
        },
        State {
            name: "FlatDisabled"
            when: !roundButton.enabled
                  && !roundButton.highlighted
                  && roundButton.flat
                  && !roundButton.hovered
                  && !roundButton.visualFocus
                  && !roundButton.pressed

            PropertyChanges {
                target: buttonRectangle
                border.color:
                    roundButton.checkable?
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.5):
                        "transparent"
            }
            PropertyChanges {
                target: iconLabel
                icon.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            }
        },
        State {
            name: "FlatHovered"
            when: roundButton.enabled
                  && !roundButton.highlighted
                  && roundButton.flat
                  && roundButton.hovered
                  && !roundButton.visualFocus
                  && !roundButton.pressed

            PropertyChanges {
                target: buttonRectangle
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.1,
                                                0.2)
                border.color:
                    roundButton.checked?
                        ThemeSettings.colorPrimary:
                    roundButton.checkable?
                        ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                 -0.3):
                        "transparent"
            }
        },
        State {
            name: "FlatFocused"
            when: roundButton.enabled
                  && !roundButton.highlighted
                  && roundButton.flat
                  && (roundButton.hovered || roundButton.visualFocus)
                  && !roundButton.pressed

            PropertyChanges {
                target: buttonRectangle
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.1,
                                                0.3)
                border.color:
                    roundButton.checked?
                        ThemeSettings.colorPrimary:
                    roundButton.checkable?
                        ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                 -0.3):
                        "transparent"
            }
        },
        State {
            name: "FlatPressed"
            when: roundButton.enabled
                  && !roundButton.highlighted
                  && roundButton.flat
                  && roundButton.pressed

            PropertyChanges {
                target: buttonRectangle
                border.color:
                    roundButton.checked?
                        ThemeSettings.colorPrimary:
                    roundButton.checkable?
                        ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                 -0.3):
                        "transparent"
            }
            PropertyChanges {
                target: buttonPress
                radius: roundButton.pressIndicatorRadius()
                opacity: 1
            }
        },
        State {
            name: "HighlightedDisabled"
            when: !roundButton.enabled
                  && roundButton.highlighted
                  && !roundButton.hovered
                  && !roundButton.visualFocus
                  && !roundButton.pressed

            PropertyChanges {
                target: buttonRectangle
                border.color: "transparent"
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)

            }
            PropertyChanges {
                target: iconLabel
                icon.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
            }
        },
        State {
            name: "HighlightedHovered"
            when: roundButton.enabled
                  && roundButton.highlighted
                  && roundButton.hovered
                  && !roundButton.visualFocus
                  && !roundButton.pressed

            PropertyChanges {
                target: buttonRectangle
                border.color:
                    roundButton.checked?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                    roundButton.checkable?
                        "grey":
                        "transparent"
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1)
            }
        },
        State {
            name: "HighlightedFocused"
            when: roundButton.enabled
                  && roundButton.highlighted
                  && (roundButton.hovered || roundButton.visualFocus)
                  && !roundButton.pressed

            PropertyChanges {
                target: buttonRectangle
                border.color:
                    roundButton.checked?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                    roundButton.checkable?
                        "grey":
                        "transparent"
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.2)

            }
        },
        State {
            name: "HighlightedPressed"
            when: roundButton.enabled
                  && roundButton.highlighted
                  && roundButton.pressed

            PropertyChanges {
                target: buttonPress
                radius: roundButton.pressIndicatorRadius()
                opacity: 1
            }
            PropertyChanges {
                target: buttonRectangle
                visible: false
            }
            PropertyChanges {
                target: buttonRectangleBelow
                border.color:
                    roundButton.checked?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                    roundButton.checkable?
                        "grey":
                        "transparent"
                border.width:
                    roundButton.checkable?
                        AkUnit.create(2 * ThemeSettings.controlScale,
                                                  "dp").pixels:
                        AkUnit.create(0 * ThemeSettings.controlScale,
                                                  "dp").pixels
                visible: true
            }
            PropertyChanges {
                target: buttonShadow
                radius: AkUnit.create(12 * ThemeSettings.controlScale, "dp").pixels
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: buttonPress
            properties: "radius"
            duration: roundButton.animationTime
        }
        ColorAnimation {
            target: buttonRectangle
            duration: roundButton.animationTime
        }
        PropertyAnimation {
            target: buttonShadow
            properties: "radius"
            duration: roundButton.animationTime
        }
    }
}
