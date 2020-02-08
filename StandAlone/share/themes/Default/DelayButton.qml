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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Templates 2.5 as T
import QtQuick.Layouts 1.3
import Ak 1.0

T.DelayButton {
    id: control
    font.bold: true
    icon.width: AkUnit.create(18 * ThemeSettings.controlScale, "dp").pixels
    icon.height: AkUnit.create(18 * ThemeSettings.controlScale, "dp").pixels
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
    spacing: AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
    hoverEnabled: true

    readonly property int animationTime: 200

    transition: Transition {
        NumberAnimation {
            duration:
                control.delay
                * (control.pressed? 1.0 - control.progress: 0.3 * control.progress)
        }
    }

    contentItem: Item {
        id: buttonContent
        implicitWidth:
            iconLabel.implicitWidth
            + AkUnit.create(18 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight: iconLabel.implicitHeight

        GridLayout {
            id: iconLabel
            columnSpacing: control.spacing
            rowSpacing: control.spacing
            layoutDirection: control.mirrored?
                                 Qt.RightToLeft:
                                 Qt.LeftToRight
            columns: control.display == AbstractButton.TextUnderIcon? 1: 2
            anchors.verticalCenter: buttonContent.verticalCenter
            anchors.horizontalCenter: buttonContent.horizontalCenter

            property color color: ThemeSettings.colorActiveButtonText

            AkColorizedImage {
                width: control.icon.width
                height: control.icon.height
                source: control.icon.source
                color: iconLabel.color
                visible: status == Image.Ready
                         && control.display != AbstractButton.TextOnly
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            }
            Text {
                text: control.text
                font: control.font
                color: iconLabel.color
                visible: text
                         && control.display != AbstractButton.IconOnly
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            }
        }
    }
    background: Item {
        id: back
        implicitWidth: AkUnit.create(64 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight: AkUnit.create(36 * ThemeSettings.controlScale, "dp").pixels

        // Rectangle
        Rectangle {
            id: buttonRectangle
            anchors.fill: parent
            radius: AkUnit.create(6 * ThemeSettings.controlScale, "dp").pixels
            border.width:
                AkUnit.create(1 * ThemeSettings.controlScale, "dp").pixels
            border.color: ThemeSettings.colorActiveDark
            color: ThemeSettings.colorActiveButton
            gradient: Gradient {
                GradientStop {
                    position: 0
                    color: ThemeSettings.colorActiveWindow.hslLightness < 0.5?
                               Qt.tint(buttonRectangle.color,
                                       ThemeSettings.shade(ThemeSettings.colorActiveDark, 0, 0.25)):
                               Qt.tint(buttonRectangle.color,
                                       ThemeSettings.shade(ThemeSettings.colorActiveLight, 0, 0.25))
                }
                GradientStop {
                    position: 1
                    color: ThemeSettings.colorActiveWindow.hslLightness < 0.5?
                               Qt.tint(buttonRectangle.color,
                                       ThemeSettings.shade(ThemeSettings.colorActiveLight, 0, 0.25)):
                               Qt.tint(buttonRectangle.color,
                                       ThemeSettings.shade(ThemeSettings.colorActiveDark, 0, 0.25))
                }
            }
        }

        // Checked indicator
        Rectangle {
            id: buttonCheckableIndicator
            height: AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
            color: ThemeSettings.colorActiveDark
            anchors.bottom: back.bottom
            anchors.left: back.left
            anchors.right: back.right
        }
        Rectangle {
            width: parent.width * control.progress
            height: AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
            color: ThemeSettings.colorActiveHighlight
            anchors.bottom: back.bottom
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: iconLabel
                color: ThemeSettings.colorDisabledButtonText
            }
            PropertyChanges {
                target: buttonCheckableIndicator
                color: ThemeSettings.colorDisabledDark
            }
            PropertyChanges {
                target: buttonRectangle
                border.color: ThemeSettings.colorDisabledDark
                color: ThemeSettings.colorDisabledButton
            }
        },
        State {
            name: "Hovered"
            when: control.enabled
                  && control.hovered
                  && !(control.activeFocus || control.visualFocus)
                  && !control.pressed

            PropertyChanges {
                target: buttonRectangle
                color: ThemeSettings.colorActiveMid
            }
        },
        State {
            name: "Focused"
            when: control.enabled
                  && (control.hovered || control.visualFocus)
                  && !control.pressed

            PropertyChanges {
                target: buttonRectangle
                border.width: AkUnit.create(2 * ThemeSettings.controlScale,
                                            "dp").pixels
                border.color: ThemeSettings.colorActiveHighlight
                color: ThemeSettings.colorActiveMid
            }
        },
        State {
            name: "Pressed"
            when: control.enabled
                  && control.pressed

            PropertyChanges {
                target: buttonRectangle
                border.width: AkUnit.create(2 * ThemeSettings.controlScale,
                                            "dp").pixels
                border.color: ThemeSettings.colorActiveHighlight
                color: ThemeSettings.colorActiveDark
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: buttonRectangle
            properties: "color,border.color,border.width"
            duration: control.animationTime
        }
    }
}
