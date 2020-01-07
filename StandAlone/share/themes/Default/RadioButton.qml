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
import QtQuick.Controls.impl 2.12
import AkQml 1.0

T.RadioButton {
    id: radioButton
    icon.width: Ak.newUnit(18 * ThemeSettings.controlScale, "dp").pixels
    icon.height: Ak.newUnit(18 * ThemeSettings.controlScale, "dp").pixels
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + 2 * implicitIndicatorWidth
                            + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             2 * implicitIndicatorHeight + topPadding + bottomPadding)
    padding: Ak.newUnit(4 * ThemeSettings.controlScale, "dp").pixels
    spacing: Ak.newUnit(8 * ThemeSettings.controlScale, "dp").pixels
    hoverEnabled: true
    clip: true

    readonly property int animationTime: 100

    indicator: Item {
        id: radioButtonIndicator
        anchors.left: radioButton.left
        anchors.leftMargin: indicatorRect.width / 2 + radioButton.leftPadding
        anchors.verticalCenter: radioButton.verticalCenter
        implicitWidth:
            Ak.newUnit(24 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight:
            Ak.newUnit(24 * ThemeSettings.controlScale, "dp").pixels

        Rectangle {
            id: highlight
            width: radioButton.visualFocus? 2 * parent.width: 0
            height: width
            color:
                radioButton.checked?
                    indicatorRect.border.color:
                    ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            radius: width / 2
            anchors.verticalCenter: radioButtonIndicator.verticalCenter
            anchors.horizontalCenter: radioButtonIndicator.horizontalCenter
            opacity: radioButton.visualFocus? 0.5: 0
        }
        Rectangle {
            id: indicatorRect
            border.width: Ak.newUnit(2 * ThemeSettings.controlScale, "dp").pixels
            border.color:
                radioButton.checked?
                    ThemeSettings.colorPrimary:
                    ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            color: "transparent"
            radius: Math.min(radioButtonIndicator.width, radioButtonIndicator.height) / 2
            anchors.verticalCenter: radioButtonIndicator.verticalCenter
            anchors.horizontalCenter: radioButtonIndicator.horizontalCenter
            width: Math.min(radioButtonIndicator.width, radioButtonIndicator.height)
            height: width

            Rectangle {
                id: indicatorCheckedMark
                color: ThemeSettings.colorPrimary
                width: Ak.newUnit(15 * ThemeSettings.controlScale, "dp").pixels
                height: Ak.newUnit(15 * ThemeSettings.controlScale, "dp").pixels
                radius: Math.min(width, height) / 2
                anchors.verticalCenter: indicatorRect.verticalCenter
                anchors.horizontalCenter: indicatorRect.horizontalCenter
                visible: radioButton.checked
            }
        }
    }

    contentItem: IconLabel {
        id: iconLabel
        spacing: radioButton.spacing
        mirrored: radioButton.mirrored
        display: radioButton.display
        icon.name: radioButton.icon.name
        icon.source: radioButton.icon.source
        icon.width: radioButton.icon.width
        icon.height: radioButton.icon.height
        icon.color: ThemeSettings.colorText
        text: radioButton.text
        font: radioButton.font
        color: ThemeSettings.colorText
        alignment: Qt.AlignLeft
        anchors.leftMargin: indicatorRect.width / 2
        anchors.left: radioButtonIndicator.right
        anchors.rightMargin: radioButton.rightPadding
        anchors.right: radioButton.right
    }

    states: [
        State {
            name: "Disabled"
            when: !radioButton.enabled

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
        },
        State {
            name: "Hovered"
            when: (radioButton.hovered
                   || radioButton.visualFocus
                   || radioButton.activeFocus)
                  && !radioButton.pressed

            PropertyChanges {
                target: indicatorRect
                border.color:
                    radioButton.checked?
                        ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.6)
            }
            PropertyChanges {
                target: indicatorCheckedMark
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1)
            }
            PropertyChanges {
                target: highlight
                width: 2 * radioButtonIndicator.width
                opacity: 0.5
            }
        },
        State {
            name: "Pressed"
            when: radioButton.pressed

            PropertyChanges {
                target: indicatorRect
                border.color:
                    radioButton.checked?
                        ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.2):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
            PropertyChanges {
                target: indicatorCheckedMark
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.2)
            }
            PropertyChanges {
                target: highlight
                width: 2 * radioButtonIndicator.width
                opacity: 0.75
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: indicatorRect
            properties: "border"
            duration: radioButton.animationTime
        }
        ColorAnimation {
            target: indicatorCheckedMark
            duration: radioButton.animationTime
        }
        ColorAnimation {
            target: iconLabel
            duration: radioButton.animationTime
        }
        PropertyAnimation {
            target: highlight
            properties: "width,opacity"
            duration: radioButton.animationTime
        }
    }
}
