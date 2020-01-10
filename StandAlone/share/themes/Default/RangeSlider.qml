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

T.RangeSlider {
    id: slider
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
        Ak.newUnit(128 * ThemeSettings.controlScale, "dp").pixels
    readonly property int defaultHeight:
        Ak.newUnit(20 * ThemeSettings.controlScale, "dp").pixels
    readonly property int trackWidth:
        Ak.newUnit(4 * ThemeSettings.controlScale, "dp").pixels
    readonly property color tickMarkColorLeft:
        slider.enabled && slider.horizontal?
            ThemeSettings.constShade(ThemeSettings.colorPrimary, -0.1):
        slider.enabled && slider.vertical?
            ThemeSettings.constShade(ThemeSettings.colorPrimary, 0, 0.5):
        !slider.enabled && slider.horizontal?
            ThemeSettings.shade(ThemeSettings.colorBack, -0.4):
            ThemeSettings.shade(ThemeSettings.colorBack, 0, 0.5)
    readonly property color tickMarkColorRight:
        slider.enabled && slider.horizontal?
            ThemeSettings.constShade(ThemeSettings.colorPrimary, 0, 0.5):
        slider.enabled && slider.vertical?
            ThemeSettings.constShade(ThemeSettings.colorPrimary, -0.1):
        !slider.enabled && slider.horizontal?
            ThemeSettings.shade(ThemeSettings.colorBack, 0, 0.5):
            ThemeSettings.shade(ThemeSettings.colorBack, -0.4)
    property color thumbColor: ThemeSettings.colorPrimary
    readonly property real thumbRadius:
        Math.min(background.implicitWidth, background.implicitHeight) / 2
    property int highlightWidth: 0
    property real highlightOpacity: 0
    readonly property int animationTime: 200

    background: Item {
        id: background
        x: slider.leftPadding
        y: slider.topPadding
        implicitWidth: slider.horizontal?
                           slider.defaultWidth:
                           slider.defaultHeight
        implicitHeight: slider.horizontal?
                            slider.defaultHeight:
                            slider.defaultWidth
        width: slider.availableWidth
        height: slider.availableHeight
        clip: true

        Rectangle {
            id: horizontalBackTrack
            height: horizontalLeftTrack.height
            radius: Math.min(width, height) / 2
            anchors.left: background.left
            anchors.right: background.right
            anchors.verticalCenter: background.verticalCenter
            color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0, 0.4)
            visible: slider.horizontal
        }
        Rectangle {
            id: horizontalLeftTrack
            width: slider.first.visualPosition * background.width
            height: slider.trackWidth
            radius: Math.min(width, height) / 2
            anchors.left: background.left
            anchors.verticalCenter: background.verticalCenter
            color: ThemeSettings.colorPrimary
            visible: slider.horizontal
        }
        Rectangle {
            id: horizontalRightTrack
            width: (1 - slider.second.visualPosition) * background.width
            height: horizontalLeftTrack.height
            radius: Math.min(width, height) / 2
            anchors.right: background.right
            anchors.verticalCenter: background.verticalCenter
            color: ThemeSettings.colorPrimary
            visible: slider.horizontal
        }
        Rectangle {
            id: verticalBackTrack
            width: slider.trackWidth
            radius: Math.min(width, height) / 2
            anchors.top: background.top
            anchors.bottom: background.bottom
            anchors.horizontalCenter: background.horizontalCenter
            color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0, 0.4)
            visible: slider.vertical
        }
        Rectangle {
            id: verticalTopTrack
            width: verticalBottomTrack.width
            height: slider.second.visualPosition * background.height
            radius: Math.min(width, height) / 2
            anchors.top: background.top
            anchors.horizontalCenter: background.horizontalCenter
            color: ThemeSettings.colorPrimary
            visible: slider.vertical
        }
        Rectangle {
            id: verticalBottomTrack
            width: slider.trackWidth
            height: (1 - slider.first.visualPosition) * background.height
            radius: Math.min(width, height) / 2
            anchors.bottom: background.bottom
            anchors.horizontalCenter: background.horizontalCenter
            color: ThemeSettings.colorPrimary
            visible: slider.vertical
        }
        GridLayout {
            columns: slider.horizontal? repeater.model + 1: 1
            rows: slider.horizontal? 1: repeater.model + 1
            x: slider.horizontal?
                   slider.thumbRadius:
                   (background.width - slider.trackWidth) / 2
            y: slider.horizontal?
                   (background.height - slider.trackWidth) / 2:
                   slider.thumbRadius
            width: slider.horizontal?
                       slider.availableWidth - 2 * slider.thumbRadius:
                       slider.trackWidth
            height: slider.horizontal?
                        slider.trackWidth:
                        slider.availableHeight - 2 * slider.thumbRadius
            visible: slider.snapMode != Slider.NoSnap
                     && repeater.model > 1
                     && Math.max(slider.horizontal? slider.availableWidth: 0,
                                 slider.horizontal? 0: slider.availableHeight)
                        > 2 * slider.thumbRadius
                          + slider.trackWidth
                          * (repeater.model + 1)

            Rectangle {
                width: slider.trackWidth
                height: slider.trackWidth
                radius: Math.min(width, height) / 2
                color: slider.tickMarkColorLeft
            }
            Repeater {
                id: repeater
                model: slider.stepSize && slider.snapMode != Slider.NoSnap?
                           Math.max(0, Math.round((slider.to - slider.from)
                                                  / slider.stepSize)):
                           0

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Rectangle {
                        x: slider.horizontal?
                               parent.width - slider.trackWidth:
                               (parent.width - slider.trackWidth) / 2
                        y: slider.horizontal?
                               (parent.height - slider.trackWidth) / 2:
                               parent.height - slider.trackWidth
                        width: slider.trackWidth
                        height: slider.trackWidth
                        radius: Math.min(width, height) / 2
                        color: index < slider.visualPosition * repeater.model?
                                   slider.tickMarkColorLeft:
                                   slider.tickMarkColorRight
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
                color: slider.thumbColor
                radius: width / 2
                anchors.verticalCenter: sliderThumbRect.verticalCenter
                anchors.horizontalCenter: sliderThumbRect.horizontalCenter
                width: slider.highlightWidth
                opacity: slider.highlightOpacity
            }
            Rectangle {
                id: shadowRect
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
                radius: Math.min(width, height) / 2
                anchors.fill: parent
                visible: false
            }
            DropShadow {
                anchors.fill: parent
                cached: true
                horizontalOffset: radius / 2
                verticalOffset: radius / 2
                radius: Ak.newUnit(1 * ThemeSettings.controlScale, "dp").pixels
                samples: 2 * radius + 1
                color: ThemeSettings.constShade(ThemeSettings.colorBack, -0.9)
                source: shadowRect
            }
            Rectangle {
                id: sliderThumbRect
                color: slider.thumbColor
                radius: Math.min(width, height) / 2
                anchors.fill: parent
            }
        }
    }

    first.handle: Loader {
        x: slider.leftPadding + (slider.horizontal ?
               slider.first.visualPosition * (slider.availableWidth - width):
               (slider.availableWidth - width) / 2)
        y: slider.topPadding + (slider.horizontal?
               (slider.availableHeight - height) / 2:
               slider.first.visualPosition * (slider.availableHeight - height))
        sourceComponent: handleComponent
    }
    second.handle: Loader {
        x: slider.leftPadding + (slider.horizontal ?
               slider.second.visualPosition * (slider.availableWidth - width) :
               (slider.availableWidth - width) / 2)
        y: slider.topPadding + (slider.horizontal ?
               (slider.availableHeight - height) / 2 :
               slider.second.visualPosition * (slider.availableHeight - height))
        sourceComponent: handleComponent
    }

    states: [
        State {
            name: "Disabled"
            when: !slider.enabled

            PropertyChanges {
                target: slider
                thumbColor: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            }
        },
        State {
            name: "HoverFocus"
            when: slider.enabled
                  && (slider.hovered || slider.activeFocus)
                  && !(slider.first.pressed || slider.second.pressed)

            PropertyChanges {
                target: slider
                thumbColor: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1)
                highlightWidth: 4 * slider.thumbRadius
                highlightOpacity: 0.75
            }
        },
        State {
            name: "Pressed"
            when: slider.enabled
                  && (slider.first.pressed || slider.second.pressed)

            PropertyChanges {
                target: slider
                thumbColor: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.2)
                highlightWidth: 4 * slider.thumbRadius
                highlightOpacity: 0.75
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: slider
            properties: "thumbColor,highlightWidth,highlightOpacity"
            duration: slider.animationTime
        }
    }
}
