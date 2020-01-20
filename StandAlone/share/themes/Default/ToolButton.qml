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
import QtQuick.Layouts 1.3
import QtQuick.Templates 2.5 as T
import QtGraphicalEffects 1.0
import QtQuick.Controls.impl 2.12
import Ak 1.0

T.ToolButton {
    id: button
    font.bold: true
    icon.width:
        button.display == AbstractButton.IconOnly?
            0.8 * Math.min(width, height):
            0.375 * Math.min(width, height)
    icon.height:
        button.display == AbstractButton.IconOnly?
            0.8 * Math.min(width, height):
            0.375 * Math.min(width, height)
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    spacing: AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
    hoverEnabled: true

    readonly property int radius:
        AkUnit.create(6 * ThemeSettings.controlScale, "dp").pixels
    readonly property int animationTime: 200

    function pressIndicatorRadius()
    {
        let diffX = button.width / 2
        let diffY = button.height / 2
        let r2 = diffX * diffX + diffY * diffY

        return Math.sqrt(r2)
    }

    function buttonWidth()
    {
        if (button.display == AbstractButton.IconOnly)
            return AkUnit.create(48 * ThemeSettings.controlScale, "dp").pixels

        return AkUnit.create(64 * ThemeSettings.controlScale, "dp").pixels
    }

    function buttonHeight()
    {
        if (button.display == AbstractButton.IconOnly)
            return AkUnit.create(48 * ThemeSettings.controlScale, "dp").pixels

        let defaultHeight =
            button.highlighted?
                AkUnit.create(48 * ThemeSettings.controlScale, "dp").pixels:
                AkUnit.create(36 * ThemeSettings.controlScale, "dp").pixels

        return Math.max(defaultHeight,
                        iconLabel.height
                        + AkUnit.create(18 * ThemeSettings.controlScale,
                                     "dp").pixels)
    }

    contentItem: Item {
        id: buttonContent
        implicitWidth:
            iconLabel.implicitWidth
            + (button.display == AbstractButton.IconOnly?
               0: AkUnit.create(18 * ThemeSettings.controlScale, "dp").pixels)
        implicitHeight: iconLabel.implicitHeight

        IconLabel {
            id: iconLabel
            spacing: button.spacing
            mirrored: button.mirrored
            display: button.display
            anchors.verticalCenter: buttonContent.verticalCenter
            anchors.horizontalCenter: buttonContent.horizontalCenter
            icon.name: button.icon.name
            icon.source: button.icon.source
            icon.width: button.icon.width
            icon.height: button.icon.height
            icon.color: ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75)
            text: button.text
            font: button.font
            color: ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75)
        }
    }
    background: Item {
        id: back
        implicitWidth: button.buttonWidth()
        implicitHeight: button.buttonHeight()

        // Rectagle below the indicator
        Rectangle {
            id: buttonRectangleBelow
            anchors.fill: parent
            color: button.highlighted?
                       ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1):
                       ThemeSettings.constShade(ThemeSettings.colorPrimary, 0, 0)
        }

        // Press indicator
        Rectangle{
            id: buttonPressIndicatorMask
            anchors.fill: parent
            color: Qt.hsla(0, 0, 0, 1)
            visible: false
        }
        Item {
            id: buttonPressIndicatorItem
            anchors.fill: buttonPressIndicatorMask
            clip: true
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: buttonPressIndicatorMask
            }

            Rectangle {
                id: buttonPress
                radius: 0
                anchors.verticalCenter:
                    buttonPressIndicatorItem.verticalCenter
                anchors.horizontalCenter:
                    buttonPressIndicatorItem.horizontalCenter
                width: 2 * radius
                height: 2 * radius
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.3)
                opacity: 0
            }
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !button.enabled

            PropertyChanges {
                target: iconLabel
                opacity: 0.5
            }
        },
        State {
            name: "Hovered"
            when: button.enabled
                  && (button.hovered || button.visualFocus)
                  && !button.checked
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangleBelow
                color:
                    button.highlighted?
                        ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.2):
                        ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1)
            }
        },
        State {
            name: "Checked"
            when: button.enabled
                  && button.checked
                  && !button.pressed

            PropertyChanges {
                target: buttonRectangleBelow
                color:
                    ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.2)
            }
        },
        State {
            name: "Pressed"
            when: button.pressed

            PropertyChanges {
                target: buttonPress
                radius: button.pressIndicatorRadius()
                opacity: 1
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: buttonPress
            properties: "radius,opacity"
            duration: button.animationTime
        }
        ColorAnimation {
            target: buttonRectangleBelow
            duration: button.animationTime
        }
    }
}
