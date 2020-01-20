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

T.RadioDelegate {
    id: radioDelegate
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

    readonly property int animationTime: 200

    function pressIndicatorRadius()
    {
        let diffX = radioDelegate.width / 2
        let diffY = radioDelegate.height / 2
        let r2 = diffX * diffX + diffY * diffY

        return Math.sqrt(r2)
    }

    indicator: Item {
        id: radioDelegateIndicator
        anchors.right: radioDelegate.right
        anchors.rightMargin: radioDelegate.rightPadding
        anchors.verticalCenter: radioDelegate.verticalCenter
        implicitWidth:
            AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight:
            AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels

        Rectangle {
            id: indicatorRect
            border.width: AkUnit.create(2 * ThemeSettings.controlScale, "dp").pixels
            border.color:
                radioDelegate.highlighted?
                    ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                radioDelegate.checked?
                    ThemeSettings.colorPrimary:
                    ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            color: "transparent"
            radius: Math.min(radioDelegateIndicator.width, radioDelegateIndicator.height) / 2
            anchors.verticalCenter: radioDelegateIndicator.verticalCenter
            anchors.horizontalCenter: radioDelegateIndicator.horizontalCenter
            width: Math.min(radioDelegateIndicator.width, radioDelegateIndicator.height)
            height: width

            Rectangle {
                id: indicatorCheckedMark
                color:
                    radioDelegate.highlighted?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                        ThemeSettings.colorPrimary
                width: AkUnit.create(15 * ThemeSettings.controlScale, "dp").pixels
                height: AkUnit.create(15 * ThemeSettings.controlScale, "dp").pixels
                radius: Math.min(width, height) / 2
                anchors.verticalCenter: indicatorRect.verticalCenter
                anchors.horizontalCenter: indicatorRect.horizontalCenter
                visible: radioDelegate.checked
            }
        }
    }

    contentItem: IconLabel {
        id: iconLabel
        spacing: radioDelegate.spacing
        mirrored: radioDelegate.mirrored
        display: radioDelegate.display
        icon.name: radioDelegate.icon.name
        icon.source: radioDelegate.icon.source
        icon.width: radioDelegate.icon.width
        icon.height: radioDelegate.icon.height
        icon.color:
            control.highlighted?
                ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                ThemeSettings.colorText
        text: radioDelegate.text
        font: radioDelegate.font
        color: radioDelegate.highlighted?
                   ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                   ThemeSettings.colorText
        alignment: Qt.AlignLeft
        anchors.leftMargin: radioDelegate.leftPadding
        anchors.left: radioDelegate.left
        anchors.right: radioDelegateIndicator.left
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
            color: radioDelegate.highlighted?
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
            color: radioDelegate.highlighted?
                       ThemeSettings.colorPrimary:
                       ThemeSettings.shade(ThemeSettings.colorBack, -0.1, 0)
            anchors.fill: parent
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !radioDelegate.enabled

            PropertyChanges {
                target: indicatorRect
                border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
            PropertyChanges {
                target: indicatorCheckedMark
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
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
            when: (radioDelegate.hovered
                   || radioDelegate.visualFocus
                   || radioDelegate.activeFocus)
                  && !radioDelegate.pressed

            PropertyChanges {
                target: indicatorRect
                border.color:
                    radioDelegate.highlighted?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                    radioDelegate.checked?
                        ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.6)
            }
            PropertyChanges {
                target: indicatorCheckedMark
                color:
                    radioDelegate.highlighted?
                           ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                           ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1)
            }
            PropertyChanges {
                target: background
                color:
                    radioDelegate.highlighted?
                        ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                 0.1):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
        },
        State {
            name: "Pressed"
            when: radioDelegate.pressed

            PropertyChanges {
                target: indicatorRect
                border.color:
                    radioDelegate.highlighted?
                        ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                    radioDelegate.checked?
                        ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.2):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
            PropertyChanges {
                target: indicatorCheckedMark
                color:
                    radioDelegate.highlighted?
                           ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                           ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.2)
            }
            PropertyChanges {
                target: controlPress
                radius: radioDelegate.pressIndicatorRadius()
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
            target: indicatorRect
            properties: "border"
            duration: radioDelegate.animationTime
        }
        ColorAnimation {
            target: indicatorCheckedMark
            duration: radioDelegate.animationTime
        }
        ColorAnimation {
            target: iconLabel
            duration: radioDelegate.animationTime
        }
        ColorAnimation {
            target: background
            duration: radioDelegate.animationTime
        }
        PropertyAnimation {
            target: controlPress
            properties: "opacity,radius"
            duration: radioDelegate.animationTime
        }
    }
}
