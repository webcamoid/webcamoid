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
import QtGraphicalEffects 1.0
import QtQuick.Controls.impl 2.12
import Ak 1.0

T.MenuItem {
    id: menuItem
    implicitWidth:
        Math.max(implicitBackgroundWidth + leftInset + rightInset,
                 implicitContentWidth + leftPadding + rightPadding
                 + menuItemArrow.width
                 + (menuItem.checkable? AkUnit.create(20 * ThemeSettings.controlScale, "dp").pixels: 0)
                 + AkUnit.create(48 * ThemeSettings.controlScale, "dp").pixels )
    implicitHeight:
        Math.max(implicitBackgroundHeight + topInset + bottomInset,
                 implicitContentHeight + topPadding + bottomPadding,
                 implicitIndicatorHeight + topPadding + bottomPadding)
    rightPadding: AkUnit.create(16 * ThemeSettings.controlScale, "dp").pixels
    spacing: AkUnit.create(20 * ThemeSettings.controlScale, "dp").pixels
    icon.width: AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels
    icon.height: AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels
    clip: true
    hoverEnabled: true

    readonly property int animationTime: 200

    function pressIndicatorRadius()
    {
        let diffX = menuItem.width / 2
        let diffY = menuItem.height / 2
        let r2 = diffX * diffX + diffY * diffY

        return Math.sqrt(r2)
    }

    // Checked indicator
    indicator: Item {
        id: menuItemCheck
        width: menuItem.checkable?
                   AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels: 0
        height: menuItem.checkable?
                    AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels: 0
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: AkUnit.create(16 * ThemeSettings.controlScale, "dp").pixels
        visible: menuItem.checkable && menuItem.checked

        Item {
            id: checkItem
            width: menuItemCheck.width / 2
            height: menuItemCheck.height / 2
            anchors.verticalCenter: menuItemCheck.verticalCenter
            anchors.horizontalCenter: menuItemCheck.horizontalCenter

            Image {
                id: checkImage
                asynchronous: true
                cache: true
                source: "image://icons/check"
                sourceSize: Qt.size(width, height)
                visible: false
                anchors.fill: checkItem
            }
            ColorOverlay {
                id: checkOverlay
                anchors.fill: checkImage
                source: checkImage
                color: menuItem.highlighted?
                           ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                           ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            }
        }
    }

    // >
    arrow: Item {
        id: menuItemArrow
        width: visible? AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels: 0
        height: visible? AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels: 0
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: AkUnit.create(16 * ThemeSettings.controlScale, "dp").pixels
        visible: menuItem.subMenu

        Item {
            id: arrowItem
            width: menuItemArrow.width / 2
            height: menuItemArrow.height / 2
            anchors.verticalCenter: menuItemArrow.verticalCenter
            anchors.horizontalCenter: menuItemArrow.horizontalCenter

            Image {
                id: arrowImage
                asynchronous: true
                cache: true
                source: "image://icons/right"
                sourceSize: Qt.size(width, height)
                visible: false
                anchors.fill: arrowItem
            }
            ColorOverlay {
                id: arrowOverlay
                anchors.fill: arrowImage
                source: arrowImage
                color: menuItem.highlighted?
                           ThemeSettings.contrast(ThemeSettings.colorPrimary,
                                                  0.75):
                           ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            }
        }
    }

    contentItem: IconLabel {
        id: iconLabel
        spacing: menuItem.spacing
        mirrored: menuItem.mirrored
        display: menuItem.display
        icon.name: menuItem.icon.name
        icon.source: menuItem.icon.source
        icon.width: menuItem.icon.width
        icon.height: menuItem.icon.height
        icon.color: menuItem.highlighted?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                        ThemeSettings.colorText
        text: menuItem.text
        anchors.left: menuItemCheck.right
        anchors.leftMargin:
            menuItem.checkable?
                AkUnit.create(20 * ThemeSettings.controlScale, "dp").pixels: 0
        anchors.right: menuItemArrow.left
        font: menuItem.font
        color: menuItem.highlighted?
                   ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                   ThemeSettings.colorText
        alignment: Qt.AlignLeft
    }

    background: Item {
        id: backgroundItem
        implicitWidth: AkUnit.create(128 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight: AkUnit.create(48 * ThemeSettings.controlScale, "dp").pixels

        // Press indicator
        Rectangle{
            id: menuItemPressIndicatorMask
            anchors.fill: parent
            color: Qt.hsla(0, 0, 0, 1)
            visible: false
        }
        Rectangle {
            id: menuItemPress
            radius: 0
            anchors.verticalCenter: backgroundItem.verticalCenter
            anchors.horizontalCenter: backgroundItem.horizontalCenter
            width: 2 * radius
            height: 2 * radius
            color: menuItem.highlighted?
                       ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.3,
                                                0.75):
                       ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            opacity: 0
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: menuItemPressIndicatorMask
            }
        }

        Rectangle {
            id: background
            color: menuItem.highlighted?
                       ThemeSettings.colorPrimary:
                       ThemeSettings.shade(ThemeSettings.colorBack, -0.1, 0)
            anchors.fill: parent
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !menuItem.enabled

            PropertyChanges {
                target: iconLabel
                opacity: 0.5
            }
            PropertyChanges {
                target: checkOverlay
                color:
                    menuItem.highlighted?
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.2):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
            PropertyChanges {
                target: arrowOverlay
                color:
                    menuItem.highlighted?
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.2):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
            PropertyChanges {
                target: background
                color: menuItem.highlighted?
                           ThemeSettings.shade(ThemeSettings.colorBack, -0.3):
                           Qt.hsla(0, 0, 0, 0)
            }
        },
        State {
            name: "Hovered"
            when: menuItem.enabled
                  && menuItem.hovered
                  && !menuItem.visualFocus
                  && !menuItem.pressed

            PropertyChanges {
                target: checkOverlay
                color:
                    menuItem.highlighted?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.6)
            }
            PropertyChanges {
                target: arrowOverlay
                color:
                    menuItem.highlighted?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.6)
            }
            PropertyChanges {
                target: background
                color:
                    menuItem.highlighted?
                        ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
        },
        State {
            name: "Focused"
            when: menuItem.enabled
                  && menuItem.visualFocus
                  && !menuItem.pressed

            PropertyChanges {
                target: checkOverlay
                color:
                    menuItem.highlighted?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.6)
            }
            PropertyChanges {
                target: arrowOverlay
                color:
                    menuItem.highlighted?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.6)
            }
            PropertyChanges {
                target: background
                color:
                    menuItem.highlighted?
                        ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.2):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.2)
            }
        },
        State {
            name: "Pressed"
            when: menuItem.enabled
                  && menuItem.pressed

            PropertyChanges {
                target: checkOverlay
                color:
                    menuItem.highlighted?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
            PropertyChanges {
                target: arrowOverlay
                color:
                    menuItem.highlighted?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
            PropertyChanges {
                target: menuItemPress
                radius: menuItem.pressIndicatorRadius()
                opacity: 1
            }
            PropertyChanges {
                target: background
                visible: false
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: iconLabel
            properties: "opacity"
            duration: menuItem.animationTime
        }
        ColorAnimation {
            target: checkOverlay
            duration: menuItem.animationTime
        }
        ColorAnimation {
            target: arrowOverlay
            duration: menuItem.animationTime
        }
        ColorAnimation {
            target: background
            duration: menuItem.animationTime
        }
        PropertyAnimation {
            target: menuItemPress
            properties: "opacity,radius"
            duration: menuItem.animationTime
        }
    }
}
