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
import QtQuick.Layouts 1.3
import QtQuick.Templates 2.5 as T
import QtGraphicalEffects 1.0
import QtQuick.Controls.impl 2.12
import AkQml 1.0

T.SwipeDelegate {
    id: control
    icon.width: Ak.newUnit(18 * ThemeSettings.controlScale, "dp").pixels
    icon.height: Ak.newUnit(18 * ThemeSettings.controlScale, "dp").pixels
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + implicitIndicatorWidth
                            + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)
    padding: Ak.newUnit(4 * ThemeSettings.controlScale, "dp").pixels
    spacing: Ak.newUnit(8 * ThemeSettings.controlScale, "dp").pixels
    hoverEnabled: true
    clip: true
    swipe.transition: Transition {
        SmoothedAnimation {
            velocity: 3
            easing.type: Easing.InOutCubic
        }
    }

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
        leftPadding: swipe.right || control.swipe.position > 0.9?
                         control.icon.width:
                         0
        rightPadding: swipe.left || control.swipe.position < -0.9?
                          control.icon.width:
                          0
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
    }

    background: Rectangle {
        id: backgroundRect
        implicitWidth:
            Ak.newUnit(128 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight:
            Ak.newUnit(48 * ThemeSettings.controlScale, "dp").pixels
        color: control.highlighted?
                   ThemeSettings.colorPrimary:
                   ThemeSettings.colorBack

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
            anchors.verticalCenter: backgroundRect.verticalCenter
            anchors.horizontalCenter: backgroundRect.horizontalCenter
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

        Image {
            asynchronous: true
            cache: true
            source: "image://icons/swipe-left.png"
            sourceSize: Qt.size(width, height)
            width: control.icon.width
            height: control.icon.height
            anchors.verticalCenter: parent.verticalCenter
            visible: swipe.right || control.swipe.position > 0.9
            layer.enabled: true
            layer.effect: ColorOverlay {
                color: control.highlighted?
                           ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                           ThemeSettings.colorText
             }
        }
        Image {
            asynchronous: true
            cache: true
            source: "image://icons/swipe-right.png"
            sourceSize: Qt.size(width, height)
            width: control.icon.width
            height: control.icon.height
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            visible: swipe.left || control.swipe.position < -0.9
            layer.enabled: true
            layer.effect: ColorOverlay {
                color: control.highlighted?
                           ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                           ThemeSettings.colorText
             }
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
        },
        State {
            name: "Hovered"
            when: (control.hovered
                   || control.visualFocus
                   || control.activeFocus)
                  && !control.pressed

            PropertyChanges {
                target: backgroundRect
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
        }
    ]

    transitions: Transition {
        ColorAnimation {
            target: backgroundRect
            duration: control.animationTime
        }
        ColorAnimation {
            target: iconLabel
            duration: control.animationTime
        }
        PropertyAnimation {
            target: controlPress
            properties: "opacity,radius"
            duration: control.animationTime
        }
    }
}
