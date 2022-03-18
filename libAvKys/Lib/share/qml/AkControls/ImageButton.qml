/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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
import Ak 1.0

AbstractButton {
    id: control
    font.bold: true
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: 0
    spacing: 0
    hoverEnabled: true

    property string image: ""
    property alias status: buttonImage.status
    property alias fillMode: buttonImage.fillMode
    property alias cache: buttonImage.cache
    property bool highlighted: false
    readonly property int animationTime: 200
    readonly property color activeButton: AkTheme.palette.active.button
    readonly property color activeDark: AkTheme.palette.active.dark
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color activeHighlightedText: AkTheme.palette.active.highlightedText
    readonly property color activeLight: AkTheme.palette.active.light
    readonly property color activeMid: AkTheme.palette.active.mid
    readonly property color activeWindow: AkTheme.palette.active.window
    readonly property color disabledButton: AkTheme.palette.disabled.button
    readonly property color disabledButtonText: AkTheme.palette.disabled.buttonText
    readonly property color disabledDark: AkTheme.palette.disabled.dark

    contentItem: Item {
        id: buttonContent
        implicitWidth: back.implicitWidth
        implicitHeight: back.implicitHeight

        Rectangle {
            id: buttonImageHighlight
            color: "white"
            opacity: 0
            anchors.fill: parent
        }
        Rectangle {
            id: buttonRectangle
            anchors.fill: parent
            border.width:
                control.highlighted || control.checkable?
                    AkUnit.create(2 * AkTheme.controlScale, "dp").pixels:
                    AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
            border.color:
                control.checkable && control.checked?
                    control.activeMid:
                control.highlighted?
                    control.activeDark:
                    "transparent"
            color: "transparent"
        }
    }
    background: Item {
        id: back
        implicitWidth: AkUnit.create(32 * AkTheme.controlScale, "dp").pixels
        implicitHeight: AkUnit.create(32 * AkTheme.controlScale, "dp").pixels

        AkColorizedImage {
            id: buttonImage
            source: control.icon.source
            color: "gray"
            colorize: !control.enabled
            visible: status == Image.Ready
            asynchronous: true
            fillMode: AkColorizedImage.PreserveAspectFit
            mipmap: true
            smooth: true
            anchors.fill: parent
        }
    }

    states: [
        State {
            name: "Hovered"
            when: control.enabled
                  && control.hovered
                  && !(control.activeFocus || control.visualFocus)
                  && !control.pressed

            PropertyChanges {
                target: buttonImageHighlight
                opacity: 0.1
            }
        },
        State {
            name: "Focused"
            when: control.enabled
                  && (control.activeFocus || control.visualFocus)
                  && !control.pressed

            PropertyChanges {
                target: buttonImageHighlight
                opacity: 0.2
            }
            PropertyChanges {
                target: buttonRectangle
                border.width:
                    AkUnit.create(2 * AkTheme.controlScale, "dp").pixels
                border.color:
                    control.checkable && !control.checked?
                        AkTheme.constShade(control.activeHighlight, -0.2):
                        control.activeHighlight
            }
        },
        State {
            name: "Pressed"
            when: control.enabled
                  && control.pressed

            PropertyChanges {
                target: buttonImageHighlight
                opacity: 0.3
            }
            PropertyChanges {
                target: buttonRectangle
                border.width:
                    AkUnit.create(2 * AkTheme.controlScale, "dp").pixels
                border.color:
                    control.checkable && !control.checked?
                        AkTheme.constShade(control.activeHighlight, -0.2):
                        control.activeHighlight
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: buttonRectangle
            properties: "color,border.color,border.width"
            duration: control.animationTime
        }
        PropertyAnimation {
            target: buttonImageHighlight
            properties: "opacity"
            duration: control.animationTime
        }
    }
}
