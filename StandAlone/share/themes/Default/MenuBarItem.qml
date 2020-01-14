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

import QtQuick 2.0
import QtQuick.Controls 2.5
import QtQuick.Templates 2.5 as T
import QtGraphicalEffects 1.0
import QtQuick.Controls.impl 2.12
import AkQml 1.0

T.MenuBarItem {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)
    rightPadding: Ak.newUnit(16 * ThemeSettings.controlScale, "dp").pixels
    spacing: Ak.newUnit(20 * ThemeSettings.controlScale, "dp").pixels
    icon.width: Ak.newUnit(24 * ThemeSettings.controlScale, "dp").pixels
    icon.height: Ak.newUnit(24 * ThemeSettings.controlScale, "dp").pixels
    clip: true
    hoverEnabled: true

    readonly property int animationTime: 200

    function pressIndicatorRadius()
    {
        let diffX = control.width / 2
        let diffY = control.height / 2
        let r2 = diffX * diffX + diffY * diffY

        return Math.sqrt(r2)
    }

    contentItem: IconLabel {
        id: iconLabel
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        icon.name: control.icon.name
        icon.source: control.icon.source
        icon.width: control.icon.width
        icon.height: control.icon.height
        icon.color: ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75)
        text: control.text
        font: control.font
        color: ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75)
        alignment: Qt.AlignLeft
    }

    background: Item {
        id: backgroundItem
        implicitWidth: Ak.newUnit(64 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight: Ak.newUnit(48 * ThemeSettings.controlScale, "dp").pixels

        Rectangle {
            id: background
            color: control.highlighted?
                       ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1):
                       ThemeSettings.constShade(ThemeSettings.colorPrimary, 0, 0)
            anchors.fill: parent
        }

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
            color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.3)
            opacity: 0
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: menuItemPressIndicatorMask
            }
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: iconLabel
                opacity: 0.5
            }
            PropertyChanges {
                target: background
                color: control.highlighted?
                           ThemeSettings.shade(ThemeSettings.colorBack, -0.3):
                           ThemeSettings.constShade(ThemeSettings.colorPrimary, 0, 0)
            }
        },
        State {
            name: "Hovered"
            when: control.enabled
                  && control.hovered
                  && !control.visualFocus
                  && !control.pressed

            PropertyChanges {
                target: background
                color:
                    control.highlighted?
                        ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.2):
                        ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1)
            }
        },
        State {
            name: "Pressed"
            when: control.enabled
                  && control.pressed

            PropertyChanges {
                target: menuItemPress
                radius: control.pressIndicatorRadius()
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
            duration: control.animationTime
        }
        ColorAnimation {
            target: background
            duration: control.animationTime
        }
        PropertyAnimation {
            target: menuItemPress
            properties: "opacity,radius"
            duration: control.animationTime
        }
    }
}
