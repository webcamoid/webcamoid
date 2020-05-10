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
    icon.color: AkTheme.palette.active.windowText
    clip: true
    hoverEnabled: true

    readonly property int animationTime: 200

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
            color:
                control.highlighted?
                    AkTheme.palette.active.highlightedText:
                    AkTheme.palette.active.windowText
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
            color:
                control.highlighted?
                    AkTheme.palette.active.highlightedText:
                    AkTheme.palette.active.windowText
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
                       AkTheme.palette.active.highlightedText:
                       control.icon.color
        text: control.text
        anchors.left: menuItemCheck.right
        anchors.leftMargin: 0
        anchors.right: menuItemArrow.left
        anchors.rightMargin: 0
        font: control.font
        color: control.highlighted?
                   AkTheme.palette.active.highlightedText:
                   AkTheme.palette.active.windowText
        alignment: Qt.AlignLeft | Qt.AlignVCenter
        enabled: control.enabled
    }

    background: Rectangle {
        id: background
        implicitWidth: AkUnit.create(128 * AkTheme.controlScale, "dp").pixels
        implicitHeight: AkUnit.create(48 * AkTheme.controlScale, "dp").pixels
        color: control.highlighted?
                   AkTheme.palette.active.highlight:
                   AkTheme.shade(AkTheme.palette.active.window, 0, 0)
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: iconLabel
                iconColor: control.highlighted?
                               AkTheme.palette.disabled.highlightedText:
                               AkTheme.palette.disabled.windowText
                color: control.highlighted?
                           AkTheme.palette.disabled.highlightedText:
                           AkTheme.palette.disabled.windowText
            }
            PropertyChanges {
                target: checkImage
                color:
                    control.highlighted?
                        AkTheme.palette.disabled.highlightedText:
                        AkTheme.palette.disabled.windowText
            }
            PropertyChanges {
                target: arrowImage
                color:
                    control.highlighted?
                        AkTheme.palette.disabled.highlightedText:
                        AkTheme.palette.disabled.windowText
            }
            PropertyChanges {
                target: background
                color: control.highlighted?
                           AkTheme.palette.disabled.highlight:
                           AkTheme.shade(AkTheme.palette.disabled.window, 0, 0)
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
                        AkTheme.palette.active.highlightedText:
                        AkTheme.shade(AkTheme.palette.active.window, -0.6)
            }
            PropertyChanges {
                target: arrowImage
                color:
                    control.highlighted?
                        AkTheme.palette.active.highlightedText:
                        AkTheme.shade(AkTheme.palette.active.window, -0.6)
            }
            PropertyChanges {
                target: background
                color:
                    control.highlighted?
                        AkTheme.constShade(AkTheme.palette.active.highlight, 0.1):
                        AkTheme.shade(AkTheme.palette.active.window, -0.1)
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
                        AkTheme.palette.active.highlightedText:
                        AkTheme.shade(AkTheme.palette.active.window, -0.6)
            }
            PropertyChanges {
                target: arrowImage
                color:
                    control.highlighted?
                        AkTheme.palette.active.highlightedText:
                        AkTheme.shade(AkTheme.palette.active.window, -0.6)
            }
            PropertyChanges {
                target: background
                color:
                    control.highlighted?
                        AkTheme.constShade(AkTheme.palette.active.highlight, 0.2):
                        AkTheme.shade(AkTheme.palette.active.window, -0.2)
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
                        AkTheme.palette.active.highlightedText:
                        AkTheme.shade(AkTheme.palette.active.window, -0.7)
            }
            PropertyChanges {
                target: arrowImage
                color:
                    control.highlighted?
                        AkTheme.palette.active.highlightedText:
                        AkTheme.shade(AkTheme.palette.active.window, -0.7)
            }
            PropertyChanges {
                target: background
                color:
                    control.highlighted?
                        AkTheme.constShade(AkTheme.palette.active.highlight, 0.3):
                        AkTheme.shade(AkTheme.palette.active.window, -0.3)
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
