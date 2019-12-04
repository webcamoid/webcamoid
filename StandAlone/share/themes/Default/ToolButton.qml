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

T.ToolButton {
    id: button
    font.bold: true
    icon.width:
        button.display == AbstractButton.IconOnly
        || button.highlighted?
            Ak.newUnit(24 * ThemeSettings.constrolScale, "dp").pixels:
            Ak.newUnit(18 * ThemeSettings.constrolScale, "dp").pixels
    icon.height:
        button.display == AbstractButton.IconOnly
        || button.highlighted?
            Ak.newUnit(24 * ThemeSettings.constrolScale, "dp").pixels:
            Ak.newUnit(18 * ThemeSettings.constrolScale, "dp").pixels
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    spacing: Ak.newUnit(8 * ThemeSettings.constrolScale, "dp").pixels
    hoverEnabled: true

    readonly property int radius:
        Ak.newUnit(6 * ThemeSettings.constrolScale, "dp").pixels
    readonly property int animationTime: 200

    function pressIndicatorRadius()
    {
        let diffX = button.width / 2
        let diffY = button.height / 2
        let r2 = diffX * diffX + diffY * diffY

        return Math.sqrt(r2)
    }

    function buttonWidth()
    {
        if (button.display == AbstractButton.IconOnly) {
            if (button.highlighted)
                return Ak.newUnit(40 * ThemeSettings.constrolScale, "dp").pixels

            return Ak.newUnit(48 * ThemeSettings.constrolScale, "dp").pixels
        }

        return Ak.newUnit(64 * ThemeSettings.constrolScale, "dp").pixels
    }

    function buttonHeight()
    {
        if (button.display == AbstractButton.IconOnly) {
            if (button.highlighted)
                return Ak.newUnit(40 * ThemeSettings.constrolScale, "dp").pixels

            return Ak.newUnit(48 * ThemeSettings.constrolScale, "dp").pixels
        }

        let defaultHeight =
            button.highlighted?
                Ak.newUnit(48 * ThemeSettings.constrolScale, "dp").pixels:
                Ak.newUnit(36 * ThemeSettings.constrolScale, "dp").pixels

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
            icon.name: button.icon.name
            icon.source: button.icon.source
            icon.width: button.icon.width
            icon.height: button.icon.height
            icon.color: button.highlighted?
                            ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                            ThemeSettings.contrast(ThemeSettings.colorBack)
            text: button.text
            font: button.font
            color: button.highlighted?
                       ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                       ThemeSettings.contrast(ThemeSettings.colorBack)
        }
    }
    background: Item {
        id: back
        implicitWidth: button.buttonWidth()
        implicitHeight: button.buttonHeight()

        Item {
            id: buttonContainer
            anchors.bottom: parent.bottom
            anchors.bottomMargin:
                button.highlighted?
                    0: Ak.newUnit(1 * ThemeSettings.constrolScale, "dp").pixels
            anchors.left: parent.left
            anchors.leftMargin: anchors.bottomMargin
            anchors.right: parent.right
            anchors.rightMargin: anchors.bottomMargin
            anchors.top: parent.top
            anchors.topMargin: anchors.bottomMargin

            // Shadow
            Rectangle {
                id: buttonShadowRect
                anchors.fill: parent
                radius: height / 2
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
                color: ThemeSettings.constShade(ThemeSettings.colorBack, -0.9)
                source: buttonShadowRect
                visible: button.highlighted && button.enabled
            }

            // Rectagle below the indicator
            Rectangle {
                id: buttonRectangleBelow
                anchors.fill: parent
                radius: button.highlighted?
                            height / 2:
                            0
                color: button.highlighted?
                           ThemeSettings.colorPrimary:
                           ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
                visible: !button.highlighted
            }

            // Focus
            Rectangle {
                id: buttonCheckIndicatorBelow
                anchors.fill: parent
                anchors.bottomMargin:
                    Ak.newUnit(6 * ThemeSettings.constrolScale, "dp").pixels
                anchors.leftMargin: anchors.bottomMargin
                anchors.rightMargin: anchors.bottomMargin
                anchors.topMargin: anchors.bottomMargin
                radius: height / 2
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
                visible: false
            }

            // Press indicator
            Rectangle{
                id: buttonPressIndicatorMask
                anchors.fill: parent
                radius: button.highlighted?
                            height / 2:
                            0
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
                    color: button.highlighted?
                               ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                        1.5,
                                                        0.3):
                               ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
                    opacity: 0
                }
            }

            // Rectangle
            Rectangle {
                id: buttonRectangle
                anchors.fill: parent
                radius: button.highlighted?
                            height / 2:
                            button.radius
                border.color: ThemeSettings.colorBack
                border.width:
                    Ak.newUnit(1 * ThemeSettings.constrolScale, "dp").pixels
                color: Qt.hsla(0, 0, 0, 0)
                visible: button.highlighted
            }
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !button.enabled
                  && !button.highlighted
                  && !button.hovered
                  && !button.visualFocus
                  && !button.pressed

            PropertyChanges {
                target: iconLabel
                opacity: 0.5
            }
        },
        State {
            name: "Hovered"
            when: button.enabled
                  && !button.highlighted
                  && button.hovered
                  && !button.checked
                  && !button.visualFocus
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangleBelow
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
            }
        },
        State {
            name: "Focused"
            when: button.enabled
                  && !button.highlighted
                  && !button.hovered
                  && !button.checked
                  && button.visualFocus
                  && !button.pressed

            PropertyChanges {
                target: buttonCheckIndicatorBelow
                visible: true
            }
        },
        State {
            name: "FocusedHovered"
            when: button.enabled
                  && !button.highlighted
                  && button.hovered
                  && !button.checked
                  && button.visualFocus
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangleBelow
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
            }
            PropertyChanges {
                target: buttonCheckIndicatorBelow
                visible: true
            }
        },
        State {
            name: "Checked"
            when: button.enabled
                  && !button.highlighted
                  && button.checked
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangleBelow
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
            }
        },
        State {
            name: "Pressed"
            when: button.enabled
                  && !button.highlighted
                  && button.pressed

            PropertyChanges {
                target: buttonPress
                radius: button.pressIndicatorRadius()
                opacity: 1
            }
        },
        State {
            name: "Highlighted"
            when: button.enabled
                  && button.highlighted
                  && !button.hovered
                  && !button.checked
                  && !button.visualFocus
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangleBelow
                visible: false
            }
            PropertyChanges {
                target: buttonRectangle
                border.color: Qt.hsla(0, 0, 0, 0)
                color: ThemeSettings.colorPrimary
                visible: true
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
            when: button.enabled
                  && button.highlighted
                  && button.hovered
                  && !button.checked
                  && !button.visualFocus
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangle
                border.color: Qt.hsla(0, 0, 0, 0)
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1)
            }
        },
        State {
            name: "HighlightedFocused"
            when: button.enabled
                  && button.highlighted
                  && !button.hovered
                  && !button.checked
                  && button.visualFocus
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangle
                border.color: Qt.hsla(0, 0, 0, 0)
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.2)
            }
        },
        State {
            name: "HighlightedFocusedHovered"
            when: button.enabled
                  && button.highlighted
                  && button.hovered
                  && !button.checked
                  && button.visualFocus
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangle
                border.color: Qt.hsla(0, 0, 0, 0)
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.2)
            }
        },
        State {
            name: "HighlightedChecked"
            when: button.enabled
                  && button.highlighted
                  && button.checked
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangle
                border.color: Qt.hsla(0, 0, 0, 0)
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.2)
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
