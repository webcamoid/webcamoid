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
import QtQuick.Controls.impl 2.12
import Ak 1.0

T.RadioButton {
    id: radioButton
    icon.width: AkUnit.create(18 * ThemeSettings.controlScale, "dp").pixels
    icon.height: AkUnit.create(18 * ThemeSettings.controlScale, "dp").pixels
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + 2 * implicitIndicatorWidth
                            + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             2 * implicitIndicatorHeight + topPadding + bottomPadding)
    padding: AkUnit.create(4 * ThemeSettings.controlScale, "dp").pixels
    spacing: AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
    hoverEnabled: true
    clip: true

    readonly property int animationTime: 100

    indicator: Item {
        id: radioButtonIndicator
        anchors.left: radioButton.left
        anchors.leftMargin: indicatorRect.width / 2 + radioButton.leftPadding
        anchors.verticalCenter: radioButton.verticalCenter
        implicitWidth:
            AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight:
            AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels

        Rectangle {
            id: highlight
            width: radioButton.visualFocus? 2 * parent.width: 0
            height: width
            color:
                radioButton.checked?
                    ThemeSettings.colorActiveHighlight:
                    ThemeSettings.colorActiveDark
            radius: width / 2
            anchors.verticalCenter: radioButtonIndicator.verticalCenter
            anchors.horizontalCenter: radioButtonIndicator.horizontalCenter
            opacity: radioButton.visualFocus? 0.5: 0
        }
        Rectangle {
            id: indicatorRect
            border.width: AkUnit.create(2 * ThemeSettings.controlScale, "dp").pixels
            border.color:
                radioButton.checked?
                    ThemeSettings.colorActiveHighlight:
                    ThemeSettings.colorActiveDark
            color: "transparent"
            radius: Math.min(radioButtonIndicator.width, radioButtonIndicator.height) / 2
            anchors.verticalCenter: radioButtonIndicator.verticalCenter
            anchors.horizontalCenter: radioButtonIndicator.horizontalCenter
            width: Math.min(radioButtonIndicator.width, radioButtonIndicator.height)
            height: width

            Rectangle {
                id: indicatorCheckedMark
                color: ThemeSettings.colorActiveHighlight
                width: AkUnit.create(15 * ThemeSettings.controlScale, "dp").pixels
                height: AkUnit.create(15 * ThemeSettings.controlScale, "dp").pixels
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
        icon.color: ThemeSettings.colorActiveWindowText
        text: radioButton.text
        font: radioButton.font
        color: ThemeSettings.colorActiveWindowText
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
                border.color:
                    radioButton.checked?
                        ThemeSettings.colorDisabledHighlight:
                        ThemeSettings.colorDisabledDark
            }
            PropertyChanges {
                target: indicatorCheckedMark
                color: ThemeSettings.colorDisabledHighlight
            }
            PropertyChanges {
                target: iconLabel
                icon.color: ThemeSettings.colorDisabledWindowText
                color: ThemeSettings.colorDisabledWindowText
            }
        },
        State {
            name: "Hovered"
            when: (radioButton.hovered
                   || radioButton.visualFocus
                   || radioButton.activeFocus)
                  && !radioButton.pressed

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
                target: highlight
                width: 2 * radioButtonIndicator.width
                opacity: 0.75
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: indicatorRect
            properties: "border.color"
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
