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

T.Dial {
    id: control
    implicitWidth: Math.max(Ak.newUnit(96 * ThemeSettings.constrolScale, "dp").pixels
                            + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(Ak.newUnit(96 * ThemeSettings.constrolScale, "dp").pixels
                             + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: Ak.newUnit(6 * ThemeSettings.constrolScale, "dp").pixels
    hoverEnabled: true
    focusPolicy: Qt.WheelFocus

    readonly property int animationTime: 200

    background: Item {
        id: backgrounItem
        width: Math.min(control.width, control.height)
        height: width
        x: (control.width - width) / 2
        y: (control.height - height) / 2

        Rectangle {
            id: highlight
            width: 0
            height: width
            color: ThemeSettings.colorPrimary
            radius: width / 2
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            opacity: 0
        }

        Repeater {
            id: repeater
            model: nBalls(ballRadiusFactor, 0.75)

            property real ballRadiusFactor: 0.1

            function nBalls(factor, k=1)
            {
                let r = factor / (1 - factor)
                let n = Math.PI / Math.asin(r)

                return Math.floor(k * n)
            }

            Item {
                id: ball
                x: (backgrounItem.width - width) / 2
                y: (backgrounItem.height - height) / 2
                width: repeater.ballRadiusFactor * backgrounItem.width
                height: width

                property real aperture: 140

                transform: [
                    Translate {
                        y: (ball.height - Math.min(backgrounItem.width, backgrounItem.height)) / 2
                    },
                    Rotation {
                        angle: ball.aperture
                               * (2 * index - repeater.count + 1)
                               / (repeater.count - 1)
                        origin.x: width / 2
                        origin.y: height / 2
                    }
                ]

                Rectangle {
                    radius: parent.width
                            * (index * (repeater.count - 2)
                               + repeater.count - 1)
                            / (2 * Math.pow(repeater.count - 1, 2))
                    width: 2 * radius
                    height: 2 * radius
                    color: knob.border.color
                    opacity: (index * (repeater.count - 2)
                              + repeater.count - 1)
                             / Math.pow(repeater.count - 1, 2)
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }

        // Shadow
        Rectangle {
            id: buttonShadowRect
            width: knob.width
            height: knob.height
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            radius: width / 2
            color: Qt.hsla(0, 0, 0, 1)
            visible: false
        }
        DropShadow {
            id: buttonShadow
            width: knob.width
            height: knob.height
            cached: true
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalOffset: radius / 2
            verticalOffset: radius / 2
            radius: Ak.newUnit(6 * ThemeSettings.constrolScale, "dp").pixels
            samples: 2 * radius + 1
            color: ThemeSettings.constShade(ThemeSettings.colorBack, -0.9)
            source: buttonShadowRect
        }

        // Rectagle below the indicator
        Rectangle {
            id: rectangleBelow
            width: knob.width
            height: knob.height
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            radius: knob.radius
            color: ThemeSettings.shade(ThemeSettings.colorBack, -0.2)
        }

        // Press indicator
        Rectangle {
            id: buttonPress
            radius: width / 2
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: 0
            height: width
            color: ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
        }

        Rectangle {
            id: knob
            color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            radius: width / 2
            border.color: ThemeSettings.colorPrimary
            border.width: handleRect.radius
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: 0.8 * parent.width * (1 - 2 * repeater.ballRadiusFactor)
            height: width
        }
    }

    handle: Rectangle {
        id: handleRect
        x: (control.width - control.handle.width) / 2
        y: (control.height - control.handle.height) / 2
        width: height / 2
        height: 0.3 * knob.width
        radius: height / 4
        color: knob.border.color

        transform: [
            Translate {
                y: (control.handle.height - knob.height) / 2
            },
            Rotation {
                angle: control.angle
                origin.x: control.handle.width / 2
                origin.y: control.handle.height / 2
            }
        ]
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: knob
                border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            }
        },
        State {
            name: "Hovered"
            when: control.hovered
                  && !(control.visualFocus || control.activeFocus)
                  && !control.pressed

            PropertyChanges {
                target: knob
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.2)
                border.color:
                    ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1)
            }
        },
        State {
            name: "Focused"
            when: (control.hovered
                   || control.visualFocus
                   || control.activeFocus)
                  && !control.pressed

            PropertyChanges {
                target: knob
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.2)
                border.color:
                    ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1)
            }
            PropertyChanges {
                target: highlight
                width: backgrounItem.width
                opacity: 0.5
            }
        },
        State {
            name: "Pressed"
            when: control.pressed

            PropertyChanges {
                target: highlight
                width: backgrounItem.width
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1)
                opacity: 0.5
            }
            PropertyChanges {
                target: buttonPress
                width: knob.width
            }
            PropertyChanges {
                target: knob
                color: "transparent"
                border.color:
                    ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.2)
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: knob
            properties: "color,border"
            duration: control.animationTime
        }
        PropertyAnimation {
            target: highlight
            properties: "color,opacity,width"
            duration: control.animationTime
        }
        PropertyAnimation {
            target: buttonPress
            properties: "width"
            duration: control.animationTime
        }
    }
}
