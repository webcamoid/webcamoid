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
import QtQuick.Templates 2.5 as T
import Ak 1.0
import "Private"

T.MenuItem {
    id: control
    implicitWidth:
        Math.max(implicitBackgroundWidth + leftInset + rightInset,
                 implicitContentWidth + leftPadding + rightPadding
                 + menuItemArrow.width
                 + (control.checkable? AkUnit.create(20 * AkTheme.controlScale, "dp").pixels: 0)
                 + AkUnit.create(48 * AkTheme.controlScale, "dp").pixels )
    implicitHeight:
        Math.max(implicitBackgroundHeight + topInset + bottomInset,
                 implicitContentHeight + topPadding + bottomPadding,
                 implicitIndicatorHeight + topPadding + bottomPadding)
    padding: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
    leftPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    rightPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    spacing: AkUnit.create(20 * AkTheme.controlScale, "dp").pixels
    icon.width: AkUnit.create(24 * AkTheme.controlScale, "dp").pixels
    icon.height: AkUnit.create(24 * AkTheme.controlScale, "dp").pixels
    icon.color: activeWindowText
    clip: true
    hoverEnabled: true

    readonly property int animationTime: 200
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color activeHighlightedText: AkTheme.palette.active.highlightedText
    readonly property color activeWindow: AkTheme.palette.active.window
    readonly property color activeWindowText: AkTheme.palette.active.windowText
    readonly property color disabledHighlight: AkTheme.palette.disabled.highlight
    readonly property color disabledHighlightedText: AkTheme.palette.disabled.highlightedText
    readonly property color disabledWindow: AkTheme.palette.disabled.window
    readonly property color disabledWindowText: AkTheme.palette.disabled.windowText

    // Checked indicator
    indicator: Item {
        id: menuItemCheck
        width: control.checkable?
                   AkUnit.create(24 * AkTheme.controlScale, "dp").pixels: 0
        height: control.checkable?
                    AkUnit.create(24 * AkTheme.controlScale, "dp").pixels: 0
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        visible: control.checkable && control.checked

        AkColorizedImage {
            id: checkImage
            width: menuItemCheck.width / 2
            height: menuItemCheck.height / 2
            anchors.verticalCenter: menuItemCheck.verticalCenter
            anchors.horizontalCenter: menuItemCheck.horizontalCenter
            source: "image://icons/check"
            color: control.highlighted?
                       control.activeHighlightedText:
                       control.activeWindowText
            asynchronous: true
            mipmap: true
        }
    }

    // >
    arrow: Item {
        id: menuItemArrow
        width: visible? AkUnit.create(24 * AkTheme.controlScale, "dp").pixels: 0
        height: visible? AkUnit.create(24 * AkTheme.controlScale, "dp").pixels: 0
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        visible: control.subMenu

        AkColorizedImage {
            id: arrowImage
            width: menuItemArrow.width / 2
            height: menuItemArrow.height / 2
            anchors.verticalCenter: menuItemArrow.verticalCenter
            anchors.horizontalCenter: menuItemArrow.horizontalCenter
            source: "image://icons/right"
            color: control.highlighted?
                       control.activeHighlightedText:
                       control.activeWindowText
            asynchronous: true
            mipmap: true
        }
    }

    contentItem: IconLabel {
        id: iconLabel
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        iconName: control.icon.name
        iconSource: control.icon.source
        iconWidth: control.icon.width
        iconHeight: control.icon.height
        iconColor: control.highlighted?
                       control.activeHighlightedText:
                       control.icon.color
        text: control.text
        anchors.left: menuItemCheck.right
        anchors.leftMargin: 0
        anchors.right: menuItemArrow.left
        anchors.rightMargin: 0
        font: control.font
        color: control.highlighted?
                   control.activeHighlightedText:
                   control.activeWindowText
        alignment: Qt.AlignLeft | Qt.AlignVCenter
        enabled: control.enabled
    }

    background: Rectangle {
        id: background
        implicitWidth: AkUnit.create(128 * AkTheme.controlScale, "dp").pixels
        implicitHeight: AkUnit.create(48 * AkTheme.controlScale, "dp").pixels
        color: control.highlighted?
                   control.activeHighlight:
                   AkTheme.shade(control.activeWindow, 0, 0)
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: iconLabel
                iconColor: control.highlighted?
                               control.disabledHighlightedText:
                               control.disabledWindowText
                color: control.highlighted?
                           control.disabledHighlightedText:
                           control.disabledWindowText
            }
            PropertyChanges {
                target: checkImage
                color: control.highlighted?
                           control.disabledHighlightedText:
                           control.disabledWindowText
            }
            PropertyChanges {
                target: arrowImage
                color: control.highlighted?
                           control.disabledHighlightedText:
                           control.disabledWindowText
            }
            PropertyChanges {
                target: background
                color: control.highlighted?
                           control.disabledHighlight:
                           AkTheme.shade(control.disabledWindow, 0, 0)
            }
        },
        State {
            name: "Hovered"
            when: control.enabled
                  && control.hovered
                  && !control.visualFocus
                  && !control.pressed

            PropertyChanges {
                target: checkImage
                color:
                    control.highlighted?
                        control.activeHighlightedText:
                        AkTheme.shade(control.activeWindow, -0.6)
            }
            PropertyChanges {
                target: arrowImage
                color:
                    control.highlighted?
                        control.activeHighlightedText:
                        AkTheme.shade(control.activeWindow, -0.6)
            }
            PropertyChanges {
                target: background
                color:
                    control.highlighted?
                        AkTheme.constShade(control.activeHighlight, 0.1):
                        AkTheme.shade(control.activeWindow, -0.1)
            }
        },
        State {
            name: "Focused"
            when: control.enabled
                  && control.visualFocus
                  && !control.pressed

            PropertyChanges {
                target: checkImage
                color:
                    control.highlighted?
                        control.activeHighlightedText:
                        AkTheme.shade(control.activeWindow, -0.6)
            }
            PropertyChanges {
                target: arrowImage
                color:
                    control.highlighted?
                        control.activeHighlightedText:
                        AkTheme.shade(control.activeWindow, -0.6)
            }
            PropertyChanges {
                target: background
                color:
                    control.highlighted?
                        AkTheme.constShade(control.activeHighlight, 0.2):
                        AkTheme.shade(control.activeWindow, -0.2)
            }
        },
        State {
            name: "Pressed"
            when: control.enabled
                  && control.pressed

            PropertyChanges {
                target: checkImage
                color:
                    control.highlighted?
                        control.activeHighlightedText:
                        AkTheme.shade(control.activeWindow, -0.7)
            }
            PropertyChanges {
                target: arrowImage
                color:
                    control.highlighted?
                        control.activeHighlightedText:
                        AkTheme.shade(control.activeWindow, -0.7)
            }
            PropertyChanges {
                target: background
                color:
                    control.highlighted?
                        AkTheme.constShade(control.activeHighlight, 0.3):
                        AkTheme.shade(control.activeWindow, -0.3)
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: iconLabel
            properties: "opacity"
            duration: control.animationTime
        }
        PropertyAnimation {
            target: checkImage
            properties: "color"
            duration: control.animationTime
        }
        PropertyAnimation {
            target: arrowImage
            properties: "color"
            duration: control.animationTime
        }
        ColorAnimation {
            target: background
            duration: control.animationTime
        }
    }
}
