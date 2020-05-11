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
import QtQuick.Layouts 1.3
import QtQuick.Templates 2.5 as T
import Ak 1.0

T.RangeSlider {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            first.implicitHandleWidth + leftPadding + rightPadding,
                            second.implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             first.implicitHandleHeight + topPadding + bottomPadding,
                             second.implicitHandleHeight + topPadding + bottomPadding)
    padding: thumbRadius
    hoverEnabled: true
    focusPolicy: Qt.WheelFocus

    readonly property int defaultWidth:
        AkUnit.create(128 * AkTheme.controlScale, "dp").pixels
    readonly property int defaultHeight:
        AkUnit.create(20 * AkTheme.controlScale, "dp").pixels
    readonly property int trackWidth:
        AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
    readonly property color tickMarkColorLeft:
        control.enabled && control.horizontal?
            AkTheme.constShade(activeHighlight, -0.1):
        control.enabled && control.vertical?
            AkTheme.constShade(activeHighlight, 0, 0.5):
        !control.enabled && control.horizontal?
            AkTheme.shade(activeWindow, -0.4):
            AkTheme.shade(activeWindow, 0, 0.5)
    readonly property color tickMarkColorRight:
        control.enabled && control.horizontal?
            AkTheme.constShade(activeHighlight, 0, 0.5):
        control.enabled && control.vertical?
            AkTheme.constShade(activeHighlight, -0.1):
        !control.enabled && control.horizontal?
            AkTheme.shade(activeWindow, 0, 0.5):
            AkTheme.shade(activeWindow, -0.4)
    property color thumbColor: activeHighlight
    readonly property real thumbRadius:
        Math.min(background.implicitWidth, background.implicitHeight) / 2
    property int highlightWidth: 0
    property real highlightOpacity: 0
    readonly property int animationTime: 200
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color activeWindow: AkTheme.palette.active.window
    readonly property color disabledWindow: AkTheme.palette.disabled.window

    background: Item {
        id: background
        x: control.leftPadding
        y: control.topPadding
        implicitWidth: control.horizontal?
                           control.defaultWidth:
                           control.defaultHeight
        implicitHeight: control.horizontal?
                            control.defaultHeight:
                            control.defaultWidth
        width: control.availableWidth
        height: control.availableHeight
        clip: true

        Rectangle {
            id: horizontalBackTrack
            height: horizontalLeftTrack.height
            radius: Math.min(width, height) / 2
            anchors.left: background.left
            anchors.right: background.right
            anchors.verticalCenter: background.verticalCenter
            color: AkTheme.constShade(control.activeHighlight, 0, 0.5)
            visible: control.horizontal
        }
        Rectangle {
            id: horizontalLeftTrack
            width: control.first.visualPosition * background.width
            height: control.trackWidth
            radius: Math.min(width, height) / 2
            anchors.left: background.left
            anchors.verticalCenter: background.verticalCenter
            color: control.activeHighlight
            visible: control.horizontal
        }
        Rectangle {
            id: horizontalRightTrack
            width: (1 - control.second.visualPosition) * background.width
            height: horizontalLeftTrack.height
            radius: Math.min(width, height) / 2
            anchors.right: background.right
            anchors.verticalCenter: background.verticalCenter
            color: control.activeHighlight
            visible: control.horizontal
        }
        Rectangle {
            id: verticalBackTrack
            width: control.trackWidth
            radius: Math.min(width, height) / 2
            anchors.top: background.top
            anchors.bottom: background.bottom
            anchors.horizontalCenter: background.horizontalCenter
            color: AkTheme.constShade(control.activeHighlight, 0, 0.5)
            visible: control.vertical
        }
        Rectangle {
            id: verticalTopTrack
            width: verticalBottomTrack.width
            height: control.second.visualPosition * background.height
            radius: Math.min(width, height) / 2
            anchors.top: background.top
            anchors.horizontalCenter: background.horizontalCenter
            color: control.activeHighlight
            visible: control.vertical
        }
        Rectangle {
            id: verticalBottomTrack
            width: control.trackWidth
            height: (1 - control.first.visualPosition) * background.height
            radius: Math.min(width, height) / 2
            anchors.bottom: background.bottom
            anchors.horizontalCenter: background.horizontalCenter
            color: control.activeHighlight
            visible: control.vertical
        }
        GridLayout {
            columns: control.horizontal? repeater.model + 1: 1
            rows: control.horizontal? 1: repeater.model + 1
            x: control.horizontal?
                   control.thumbRadius:
                   (background.width - control.trackWidth) / 2
            y: control.horizontal?
                   (background.height - control.trackWidth) / 2:
                   control.thumbRadius
            width: control.horizontal?
                       control.availableWidth - 2 * control.thumbRadius:
                       control.trackWidth
            height: control.horizontal?
                        control.trackWidth:
                        control.availableHeight - 2 * control.thumbRadius
            visible: control.snapMode != Slider.NoSnap
                     && repeater.model > 1
                     && Math.max(control.horizontal? control.availableWidth: 0,
                                 control.horizontal? 0: control.availableHeight)
                        > 2 * control.thumbRadius
                          + control.trackWidth
                          * (repeater.model + 1)

            Rectangle {
                width: control.trackWidth
                height: control.trackWidth
                radius: Math.min(width, height) / 2
                color: control.tickMarkColorLeft
            }
            Repeater {
                id: repeater
                model: control.stepSize && control.snapMode != Slider.NoSnap?
                           Math.max(0, Math.round((control.to - control.from)
                                                  / control.stepSize)):
                           0

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Rectangle {
                        x: control.horizontal?
                               parent.width - control.trackWidth:
                               (parent.width - control.trackWidth) / 2
                        y: control.horizontal?
                               (parent.height - control.trackWidth) / 2:
                               parent.height - control.trackWidth
                        width: control.trackWidth
                        height: control.trackWidth
                        radius: Math.min(width, height) / 2
                        color: index < control.visualPosition * repeater.model?
                                   control.tickMarkColorLeft:
                                   control.tickMarkColorRight
                    }
                }
            }
        }
    }

    Component {
        id: handleComponent

        Item {
            implicitWidth: Math.min(background.implicitWidth,
                                    background.implicitHeight)
            implicitHeight: implicitWidth

            Rectangle {
                id: highlight
                height: width
                color: control.thumbColor
                radius: width / 2
                anchors.verticalCenter: sliderThumbRect.verticalCenter
                anchors.horizontalCenter: sliderThumbRect.horizontalCenter
                width: control.highlightWidth
                opacity: control.highlightOpacity
            }
            Rectangle {
                id: sliderThumbRect
                color: control.thumbColor
                radius: Math.min(width, height) / 2
                anchors.fill: parent
            }
        }
    }

    first.handle: Loader {
        x: control.leftPadding + (control.horizontal ?
               control.first.visualPosition * (control.availableWidth - width):
               (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal?
               (control.availableHeight - height) / 2:
               control.first.visualPosition * (control.availableHeight - height))
        sourceComponent: handleComponent
    }
    second.handle: Loader {
        x: control.leftPadding + (control.horizontal ?
               control.second.visualPosition * (control.availableWidth - width):
               (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ?
               (control.availableHeight - height) / 2:
               control.second.visualPosition * (control.availableHeight - height))
        sourceComponent: handleComponent
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: control
                thumbColor: AkTheme.shade(control.disabledWindow, -0.5)
            }
            PropertyChanges {
                target: horizontalBackTrack
                color: AkTheme.shade(control.disabledWindow, -0.3)
            }
            PropertyChanges {
                target: horizontalLeftTrack
                color: AkTheme.shade(control.disabledWindow, -0.5)
            }
            PropertyChanges {
                target: horizontalRightTrack
                color: AkTheme.shade(control.disabledWindow, -0.5)
            }
            PropertyChanges {
                target: verticalBackTrack
                color: AkTheme.shade(control.disabledWindow, -0.3)
            }
            PropertyChanges {
                target: verticalTopTrack
                color: AkTheme.shade(control.disabledWindow, -0.5)
            }
            PropertyChanges {
                target: verticalBottomTrack
                color: AkTheme.shade(control.disabledWindow, -0.5)
            }
        },
        State {
            name: "HoverFocus"
            when: control.enabled
                  && (control.hovered || control.activeFocus)
                  && !(control.first.pressed || control.second.pressed)

            PropertyChanges {
                target: control
                thumbColor: AkTheme.constShade(control.activeHighlight, 0.1)
                highlightWidth: 4 * control.thumbRadius
                highlightOpacity: 0.75
            }
        },
        State {
            name: "Pressed"
            when: control.enabled
                  && (control.first.pressed || control.second.pressed)

            PropertyChanges {
                target: control
                thumbColor: AkTheme.constShade(control.activeHighlight, 0.2)
                highlightWidth: 4 * control.thumbRadius
                highlightOpacity: 0.75
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: control
            properties: "thumbColor,highlightWidth,highlightOpacity"
            duration: control.animationTime
        }
    }
}
