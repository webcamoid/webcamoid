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
import QtQuick.Templates 2.5 as T
import QtGraphicalEffects 1.0
import QtQuick.Controls.impl 2.12
import Ak 1.0

T.ItemDelegate {
    id: control
    icon.width: AkUnit.create(18 * ThemeSettings.controlScale, "dp").pixels
    icon.height: AkUnit.create(18 * ThemeSettings.controlScale, "dp").pixels
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + implicitIndicatorWidth
                            + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)
    padding: AkUnit.create(4 * ThemeSettings.controlScale, "dp").pixels
    spacing: AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
    hoverEnabled: true
    clip: true

    readonly property int animationTime: 100

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
        icon.color:
            control.highlighted?
                ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                ThemeSettings.colorText
        text: control.text
        font: control.font
        color: control.highlighted?
                   ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                   ThemeSettings.colorText
        alignment: control.display === IconLabel.IconOnly
                   || control.display === IconLabel.TextUnderIcon?
                       Qt.AlignCenter: Qt.AlignLeft
        anchors.leftMargin: control.leftPadding
        anchors.left: control.left
        anchors.rightMargin: control.rightPadding
        anchors.right: control.right
    }

    background: Item {
        id: backgroundItem
        implicitWidth:
            AkUnit.create(128 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight:
            AkUnit.create(48 * ThemeSettings.controlScale, "dp").pixels

        // Press indicator
        Rectangle{
            id: controlPressIndicatorMask
            anchors.fill: parent
            color: Qt.hsla(0, 0, 0, 1)
            visible: false
        }
        Rectangle {
            id: controlPress
            radius: 0
            anchors.verticalCenter: backgroundItem.verticalCenter
            anchors.horizontalCenter: backgroundItem.horizontalCenter
            width: 2 * radius
            height: 2 * radius
            color: control.highlighted?
                       ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.3,
                                                0.75):
                       ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            opacity: 0
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: controlPressIndicatorMask
            }
        }

        Rectangle {
            id: background
            color: control.highlighted?
                       ThemeSettings.colorPrimary:
                       ThemeSettings.shade(ThemeSettings.colorBack, -0.1, 0)
            anchors.fill: parent
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: iconLabel
                icon.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
            PropertyChanges {
                target: background
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1, 0)
            }
        },
        State {
            name: "Hovered"
            when: (control.hovered
                   || control.visualFocus
                   || control.activeFocus)
                  && !control.pressed

            PropertyChanges {
                target: background
                color:
                    control.highlighted?
                        ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                 0.1):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
        },
        State {
            name: "Pressed"
            when: control.pressed

            PropertyChanges {
                target: controlPress
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
        ColorAnimation {
            target: iconLabel
            duration: control.animationTime
        }
        ColorAnimation {
            target: background
            duration: control.animationTime
        }
        PropertyAnimation {
            target: controlPress
            properties: "opacity,radius"
            duration: control.animationTime
        }
    }
}
