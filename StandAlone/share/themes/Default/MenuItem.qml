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
                 + menuItemArrow.width
                 + (menuItem.checkable? Ak.newUnit(20 * ThemeSettings.constrolScale, "dp").pixels: 0)
                 + Ak.newUnit(48 * ThemeSettings.constrolScale, "dp").pixels )
    implicitHeight:
        Math.max(implicitBackgroundHeight + topInset + bottomInset,
                 implicitContentHeight + topPadding + bottomPadding,
                 implicitIndicatorHeight + topPadding + bottomPadding)

    rightPadding: Ak.newUnit(16 * ThemeSettings.constrolScale, "dp").pixels
    spacing: Ak.newUnit(20 * ThemeSettings.constrolScale, "dp").pixels
    icon.width: Ak.newUnit(24 * ThemeSettings.constrolScale, "dp").pixels
    icon.height: Ak.newUnit(24 * ThemeSettings.constrolScale, "dp").pixels
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
    indicator: Item {
        id: checkMark
        width: menuItem.checkable?
                   Ak.newUnit(24 * ThemeSettings.constrolScale, "dp").pixels: 0
        height: menuItem.checkable?
                    Ak.newUnit(24 * ThemeSettings.constrolScale, "dp").pixels: 0
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: Ak.newUnit(16 * ThemeSettings.constrolScale, "dp").pixels
        visible: menuItem.checkable && menuItem.checked

        Shape {
            id: shapeChecked
            width: visible?
                       Ak.newUnit(12 * ThemeSettings.constrolScale, "dp").pixels: 0
            height: visible?
                        Ak.newUnit(12 * ThemeSettings.constrolScale, "dp").pixels: 0
            anchors.verticalCenter: checkMark.verticalCenter
            anchors.horizontalCenter: checkMark.horizontalCenter
            antialiasing: true
            smooth: true

            ShapePath {
                id: shapePathChecked
                startX: 0 * shapeChecked.width / 3
                startY: 2 * shapeChecked.height / 3
                fillColor: "transparent"
                strokeColor:
                    menuItem.highlighted?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
                strokeWidth: Ak.newUnit(3 * ThemeSettings.constrolScale, "dp").pixels
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin

                PathLine { x: 1 * shapeChecked.width / 3; y: 3 * shapeChecked.height / 3 }
                PathLine { x: 3 * shapeChecked.width / 3; y: 0 * shapeChecked.height / 3 }
            }
        }
    }

    // >
    arrow: Item {
        id: menuItemArrow
        width: visible? Ak.newUnit(24 * ThemeSettings.constrolScale, "dp").pixels: 0
        height: visible? Ak.newUnit(24 * ThemeSettings.constrolScale, "dp").pixels: 0
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: Ak.newUnit(16 * ThemeSettings.constrolScale, "dp").pixels
        visible: menuItem.subMenu

        Shape {
            id: shapeRight
            width: visible? Ak.newUnit(6 * ThemeSettings.constrolScale, "dp").pixels: 0
            height: visible? Ak.newUnit(12 * ThemeSettings.constrolScale, "dp").pixels: 0
            anchors.verticalCenter: menuItemArrow.verticalCenter
            anchors.horizontalCenter: menuItemArrow.horizontalCenter

            ShapePath {
                id: shapePathRight
                startX: 0 * shapeRight.width
                startY: 0 * shapeRight.height
                fillColor:
                    menuItem.highlighted?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
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
        icon.name: menuItem.icon.name
        icon.source: menuItem.icon.source
        icon.width: menuItem.icon.width
        icon.height: menuItem.icon.height
        icon.color: menuItem.highlighted?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                        ThemeSettings.colorText
        text: menuItem.text
        anchors.left: checkMark.right
        anchors.leftMargin:
            menuItem.checkable?
                Ak.newUnit(20 * ThemeSettings.constrolScale, "dp").pixels: 0
        anchors.right: menuItemArrow.left
        font: menuItem.font
        color: menuItem.highlighted?
                   ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                   ThemeSettings.colorText
        alignment: Qt.AlignLeft
    }

    background: Item {
        id: backgroundItem
        implicitWidth: Ak.newUnit(128 * ThemeSettings.constrolScale, "dp").pixels
        implicitHeight: Ak.newUnit(48 * ThemeSettings.constrolScale, "dp").pixels

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
                       ThemeSettings.shade(ThemeSettings.colorBack,
                                               -0.5)
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
                       ThemeSettings.shade(ThemeSettings.colorPrimary, 0, 0)
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
                target: shapePathChecked
                strokeColor:
                    menuItem.highlighted?
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.2):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
            PropertyChanges {
                target: shapePathRight
                fillColor:
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
                target: shapePathChecked
                strokeColor:
                    menuItem.highlighted?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.6)
            }
            PropertyChanges {
                target: shapePathRight
                fillColor:
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
                target: shapePathChecked
                strokeColor:
                    menuItem.highlighted?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.6)
            }
            PropertyChanges {
                target: shapePathRight
                fillColor:
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
                target: shapePathChecked
                strokeColor:
                    menuItem.highlighted?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
            PropertyChanges {
                target: shapePathRight
                fillColor:
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
        PropertyAnimation {
            target: shapePathChecked
            properties: "strokeColor"
            duration: menuItem.animationTime
        }
        PropertyAnimation {
            target: shapePathRight
            properties: "fillColor"
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
