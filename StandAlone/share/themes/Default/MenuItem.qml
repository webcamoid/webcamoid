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
import QtQuick.Shapes 1.0
import AkQml 1.0

T.MenuItem {
    id: menuItem
    implicitWidth:
        Math.max(implicitBackgroundWidth + leftInset + rightInset,
                 implicitContentWidth + leftPadding + rightPadding
                 + itemMenuArrow.width
                 + (menuItem.checkable? Ak.newUnit(20, "dp").pixels: 0)
                 + Ak.newUnit(48, "dp").pixels )
    implicitHeight:
        Math.max(implicitBackgroundHeight + topInset + bottomInset,
                 implicitContentHeight + topPadding + bottomPadding,
                 implicitIndicatorHeight + topPadding + bottomPadding)

    rightPadding: Ak.newUnit(16, "dp").pixels
    spacing: Ak.newUnit(20, "dp").pixels
    icon.width: Ak.newUnit(24, "dp").pixels
    icon.height: Ak.newUnit(24, "dp").pixels
    clip: true
    readonly property int animationTime: 200

    function pressIndicatorRadius()
    {
        let diffX = menuItem.width / 2
        let diffY = menuItem.height / 2
        let r2 = diffX * diffX + diffY * diffY

        return Math.sqrt(r2)
    }

    // Checked indicator
    indicator: CheckBox {
        id: checkBox
        width: menuItem.checkable? Ak.newUnit(24, "dp").pixels: 0
        height: height
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: Ak.newUnit(16, "dp").pixels
        checked: menuItem.checked
        visible: menuItem.checkable
        enabled: menuItem.enabled
    }

    // >
    arrow: Item {
        id: itemMenuArrow
        width: visible? Ak.newUnit(24, "dp").pixels: 0
        height: width
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: Ak.newUnit(16, "dp").pixels
        visible: menuItem.subMenu

        Shape {
            id: shapeRight
            width: itemMenuArrow.width / 4
            height: itemMenuArrow.height / 2
            anchors.verticalCenter: itemMenuArrow.verticalCenter
            anchors.horizontalCenter: itemMenuArrow.horizontalCenter

            ShapePath {
                id: shapePathRight
                startX: 0 * shapeRight.width
                startY: 0 * shapeRight.height
                fillColor: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
                strokeColor: "transparent"

                PathLine { x: 1 * shapeRight.width; y: 0.5 * shapeRight.height }
                PathLine { x: 0 * shapeRight.width; y:   1 * shapeRight.height }
            }
        }
    }

    contentItem: IconLabel {
        id: iconLabel
        spacing: menuItem.spacing
        mirrored: menuItem.mirrored
        display: menuItem.display
        icon: menuItem.icon
        text: menuItem.text
        anchors.left: checkBox.right
        anchors.leftMargin: menuItem.checkable? Ak.newUnit(20, "dp").pixels: 0
        anchors.right: itemMenuArrow.left
        font: menuItem.font
        color: ThemeSettings.colorText
        alignment: Qt.AlignLeft
    }

    background: Item {
        id: backgroundItem
        implicitWidth: Ak.newUnit(128, "dp").pixels
        implicitHeight: Ak.newUnit(48, "dp").pixels

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
            color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            opacity: 0
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: menuItemPressIndicatorMask
            }
        }

        Rectangle {
            id: background
            color: "transparent"
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
                target: shapePathRight
                fillColor: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
        },
        State {
            name: "Hovered"
            when: menuItem.enabled
                  && menuItem.hovered
                  && !menuItem.visualFocus
                  && !menuItem.pressed

            PropertyChanges {
                target: shapePathRight
                fillColor: ThemeSettings.shade(ThemeSettings.colorBack, -0.6)
            }
            PropertyChanges {
                target: background
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
        },
        State {
            name: "Focused"
            when: menuItem.enabled
                  && menuItem.visualFocus
                  && !menuItem.pressed

            PropertyChanges {
                target: shapePathRight
                fillColor: ThemeSettings.shade(ThemeSettings.colorBack, -0.6)
            }
            PropertyChanges {
                target: background
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.2)
            }
        },
        State {
            name: "Pressed"
            when: menuItem.enabled
                  && menuItem.pressed

            PropertyChanges {
                target: shapePathRight
                fillColor: ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
            PropertyChanges {
                target: menuItemPress
                radius: menuItem.pressIndicatorRadius()
                opacity: 1
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: iconLabel
            properties: "opacity"
            duration: menuItem.animationTime
        }
        PropertyAnimation {
            target: shapePathRight
            properties: "fillColor"
            duration: menuItem.animationTime
        }
        ColorAnimation {
            target: background
            properties: "color"
            duration: menuItem.animationTime
        }
        PropertyAnimation {
            target: menuItemPress
            properties: "opacity,radius"
            duration: menuItem.animationTime
        }
    }
}
