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
import QtQuick.Shapes 1.12
import Ak 1.0

T.Dial {
    id: control
    implicitWidth: Math.max(AkUnit.create(96 * AkTheme.controlScale, "dp").pixels
                            + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(AkUnit.create(96 * AkTheme.controlScale, "dp").pixels
                             + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: AkUnit.create(6 * AkTheme.controlScale, "dp").pixels
    hoverEnabled: true
    focusPolicy: Qt.WheelFocus

    readonly property int strokeWidth: backgroundRectangle.width / 8
    readonly property int animationTime: 200
    readonly property color activeDark: AkTheme.palette.active.dark
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color activeLink: AkTheme.palette.active.link
    readonly property color activeWindow: AkTheme.palette.active.window
    readonly property color disabledHighlight: AkTheme.palette.disabled.highlight
    readonly property color disabledLink: AkTheme.palette.disabled.link
    readonly property color disabledWindowText: AkTheme.palette.disabled.windowText

    contentItem: Text {
        id: dialValue
        text: Math.round(control.value * 100) / 100
        color: control.activeWindowText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        linkColor: control.enabled?
                       control.activeLink:
                       control.disabledLink
        enabled: control.enabled
    }

    background: Rectangle {
        id: backgroundRectangle
        x: (control.width - width) / 2
        y: (control.height - height) / 2
        width: Math.min(control.width, control.height)
        height: width
        radius: width / 2
        border.color: AkTheme.shade(control.activeWindow, 0, 0.5)
        color: AkTheme.shade(control.activeWindow, 0, 0.5)

        Shape {
            id: shape
            anchors.fill: parent
            anchors.rightMargin:
                AkUnit.create(2 * AkTheme.controlScale, "dp").pixels
            anchors.leftMargin: anchors.rightMargin
            anchors.bottomMargin: anchors.rightMargin
            anchors.topMargin:  anchors.rightMargin

            readonly property real aperture: 140

            ShapePath {
                id: shapePathBack
                startX: 0
                startY: 0
                fillColor: "transparent"
                strokeColor:
                    AkTheme.constShade(shapePath.strokeColor, 0, 0.25)
                strokeStyle: ShapePath.SolidLine
                strokeWidth: control.strokeWidth
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin

                PathAngleArc {
                    centerX: shape.width / 2
                    centerY: shape.height / 2
                    radiusX: (shape.width - shapePathBack.strokeWidth) / 2
                    radiusY: (shape.height - shapePathBack.strokeWidth) / 2
                    startAngle: -shape.aperture - 90
                    sweepAngle: 2 * shape.aperture
                }
            }

            ShapePath {
                id: shapePath
                startX: 0
                startY: 0
                fillColor: "transparent"
                strokeColor: control.activeHighlight
                strokeStyle: ShapePath.SolidLine
                strokeWidth: control.strokeWidth
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin

                PathAngleArc {
                    centerX: shape.width / 2
                    centerY: shape.height / 2
                    radiusX: (shape.width - shapePathBack.strokeWidth) / 2
                    radiusY: (shape.height - shapePathBack.strokeWidth) / 2
                    startAngle: -shape.aperture - 90
                    sweepAngle: shape.aperture + control.angle
                }
            }
        }
    }

    handle: Rectangle {
        id: handleRect
        x: (control.width - control.strokeWidth) / 2
        y: (control.height - control.strokeWidth) / 2
        width: control.strokeWidth
        height: control.strokeWidth
        radius: control.strokeWidth / 2
        color: shapePath.strokeColor

        transform: [
            Translate {
                y: (handleRect.height - shape.height) / 2
            },
            Rotation {
                angle: control.angle
                origin.x: handleRect.width / 2
                origin.y: handleRect.height / 2
            }
        ]
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: shapePath
                strokeColor: control.disabledHighlight
            }
            PropertyChanges {
                target: dialValue
                color: control.disabledWindowText
            }
        },
        State {
            name: "Hovered"
            when: control.hovered
                  && !(control.visualFocus || control.activeFocus)
                  && !control.pressed

            PropertyChanges {
                target: backgroundRectangle
                border.color: control.activeDark
            }
        },
        State {
            name: "Focused"
            when: (control.hovered
                   || control.visualFocus
                   || control.activeFocus)
                  && !control.pressed

            PropertyChanges {
                target: backgroundRectangle
                border.color: control.activeHighlight
                border.width:
                    AkUnit.create(2 * AkTheme.controlScale, "dp").pixels
            }
        },
        State {
            name: "Pressed"
            when: control.pressed

            PropertyChanges {
                target: backgroundRectangle
                border.color:
                    AkTheme.constShade(control.activeHighlight, 0.1)
                border.width:
                    AkUnit.create(2 * AkTheme.controlScale, "dp").pixels
            }
            PropertyChanges {
                target: shapePath
                strokeColor: AkTheme.constShade(control.activeHighlight, 0.1)
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: shapePath
            properties: "strokeColor"
            duration: control.animationTime
        }
        ColorAnimation {
            target: dialValue
            duration: control.animationTime
        }
        PropertyAnimation {
            target: backgroundRectangle
            properties: "border.color,border.width"
            duration: control.animationTime
        }
    }
}
