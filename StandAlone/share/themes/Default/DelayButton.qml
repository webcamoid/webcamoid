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

import QtQuick 2.0
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Templates 2.5 as T
import QtGraphicalEffects 1.0
import QtQuick.Controls.impl 2.12
import AkQml 1.0

T.DelayButton {
    id: button
    font.bold: true
    icon.width: Ak.newUnit(18 * ThemeSettings.controlScale, "dp").pixels
    icon.height: Ak.newUnit(18 * ThemeSettings.controlScale, "dp").pixels
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    spacing: Ak.newUnit(8 * ThemeSettings.controlScale, "dp").pixels
    hoverEnabled: true

    readonly property int radius:
        Ak.newUnit(6 * ThemeSettings.controlScale, "dp").pixels
    readonly property int animationTime: 200

    function pressIndicatorRadius()
    {
        let diffX = button.width / 2
        let diffY = button.height / 2
        let r2 = diffX * diffX + diffY * diffY

        return Math.sqrt(r2)
    }

    transition: Transition {
        NumberAnimation {
            duration:
                button.delay
                * (button.pressed? 1.0 - button.progress: 0.3 * button.progress)
        }
    }

    contentItem: Item {
        id: buttonContent
        implicitWidth:
            iconLabel.implicitWidth
            + Ak.newUnit(18 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight: iconLabel.implicitHeight

        IconLabel {
            id: iconLabel
            spacing: button.spacing
            mirrored: button.mirrored
            display: button.display
            icon.name: button.icon.name
            icon.source: button.icon.source
            icon.width: button.icon.width
            icon.height: button.icon.height
            icon.color: ThemeSettings.colorPrimary
            text: button.text
            font: button.font
            color: ThemeSettings.colorPrimary
            anchors.verticalCenter: buttonContent.verticalCenter
            anchors.horizontalCenter: buttonContent.horizontalCenter
        }
    }
    background: Item {
        id: back
        implicitWidth: Ak.newUnit(64 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight: Ak.newUnit(36 * ThemeSettings.controlScale, "dp").pixels

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
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.1,
                                                0.3)
                opacity: 0
            }
        }

        // Rectangle
        Rectangle {
            id: buttonRectangle
            anchors.fill: parent
            radius: button.radius
            border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            border.width:
                Ak.newUnit(1 * ThemeSettings.controlScale, "dp").pixels
            color: Qt.hsla(0, 0, 0, 0)
        }

        // Checked indicator
        Rectangle {
            id: buttonCheckableIndicator
            height: button.radius
            color:
                ThemeSettings.constShade(ThemeSettings.colorPrimary, -0.3)
            anchors.bottom: back.bottom
            anchors.left: back.left
            anchors.right: back.right
        }
        Rectangle {
            width: parent.width * button.progress
            height: button.radius
            color: ThemeSettings.colorPrimary
            anchors.bottom: back.bottom
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !button.enabled
                  && !button.hovered
                  && !button.visualFocus
                  && !button.pressed

            PropertyChanges {
                target: iconLabel
                icon.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            }
            PropertyChanges {
                target: buttonCheckableIndicator
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            }
        },
        State {
            name: "Hovered"
            when: button.enabled
                  && button.hovered
                  && !button.visualFocus
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangle
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.1,
                                                0.2)
            }
        },
        State {
            name: "Focused"
            when: button.enabled
                  && (button.hovered || button.visualFocus)
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangle
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.1,
                                                0.3)
            }
        },
        State {
            name: "Pressed"
            when: button.enabled
                  && button.pressed

            PropertyChanges {
                target: buttonPress
                radius: button.pressIndicatorRadius()
                opacity: 1
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
