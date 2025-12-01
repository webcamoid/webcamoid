/* Webcamoid, camera capture application.
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

import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import Ak
import "Private"

T.RadioButton {
    id: control
    font: AkTheme.fontSettings.body1
    icon.width: AkUnit.create(18 * AkTheme.controlScale, "dp").pixels
    icon.height: AkUnit.create(18 * AkTheme.controlScale, "dp").pixels
    icon.color: activeWindowText
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + 2 * implicitIndicatorWidth
                            + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             2 * implicitIndicatorHeight + topPadding + bottomPadding)
    padding: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
    spacing: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
    hoverEnabled: true
    clip: true

    readonly property bool rtl: mirrored != (Qt.application.layoutDirection === Qt.RightToLeft)
    readonly property int animationTime: 200
    readonly property color activeDark: AkTheme.palette.active.dark
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color activeWindowText: AkTheme.palette.active.windowText
    readonly property color disabledDark: AkTheme.palette.disabled.dark
    readonly property color disabledHighlight: AkTheme.palette.disabled.highlight
    readonly property color disabledWindowText: AkTheme.palette.disabled.windowText

    indicator: Item {
        id: radioButtonIndicator
        anchors.left: control.rtl? undefined: control.left
        anchors.leftMargin: control.rtl? 0: indicatorRect.width / 2 + control.leftPadding
        anchors.right: control.rtl? control.right: undefined
        anchors.rightMargin: control.rtl? indicatorRect.width / 2 + control.rightPadding: 0
        anchors.verticalCenter: control.verticalCenter
        implicitWidth:
            AkUnit.create(24 * AkTheme.controlScale, "dp").pixels
        implicitHeight:
            AkUnit.create(24 * AkTheme.controlScale, "dp").pixels

        Rectangle {
            id: highlight
            width: control.visualFocus? 2 * parent.width: 0
            height: width
            color:
                control.checked?
                    control.activeHighlight:
                    control.activeDark
            radius: width / 2
            anchors.verticalCenter: radioButtonIndicator.verticalCenter
            anchors.horizontalCenter: radioButtonIndicator.horizontalCenter
            opacity: control.visualFocus? 0.5: 0
        }
        Rectangle {
            id: indicatorRect
            border.width: AkUnit.create(2 * AkTheme.controlScale, "dp").pixels
            border.color:
                control.checked?
                    control.activeHighlight:
                    control.activeDark
            color: "transparent"
            radius: Math.min(radioButtonIndicator.width, radioButtonIndicator.height) / 2
            anchors.verticalCenter: radioButtonIndicator.verticalCenter
            anchors.horizontalCenter: radioButtonIndicator.horizontalCenter
            width: Math.min(radioButtonIndicator.width, radioButtonIndicator.height)
            height: width

            Rectangle {
                id: indicatorCheckedMark
                color: control.activeHighlight
                width: AkUnit.create(15 * AkTheme.controlScale, "dp").pixels
                height: AkUnit.create(15 * AkTheme.controlScale, "dp").pixels
                radius: Math.min(width, height) / 2
                anchors.verticalCenter: indicatorRect.verticalCenter
                anchors.horizontalCenter: indicatorRect.horizontalCenter
                visible: control.checked
            }
        }
    }

    contentItem: IconLabel {
        id: iconLabel
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        iconName: control.icon.name
        iconSource: control.icon.source
        iconWidth: control.icon.width
        iconHeight: control.icon.height
        iconColor: control.icon.color
        text: control.text
        font: control.font
        color: control.activeWindowText
        enabled: control.enabled
        anchors.leftMargin: control.rtl? control.leftPadding: indicatorRect.width / 2
        anchors.left: control.rtl? control.left: radioButtonIndicator.right
        anchors.rightMargin: control.rtl? indicatorRect.width / 2: control.rightPadding
        anchors.right: control.rtl? radioButtonIndicator.left: control.right
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: indicatorRect
                border.color:
                    control.checked?
                        control.disabledHighlight:
                        control.disabledDark
            }
            PropertyChanges {
                target: indicatorCheckedMark
                color: control.disabledHighlight
            }
            PropertyChanges {
                target: iconLabel
                color: control.disabledWindowText
            }
        },
        State {
            name: "Hovered"
            when: (control.hovered
                   || control.visualFocus
                   || control.activeFocus)
                  && !control.pressed

            PropertyChanges {
                target: highlight
                width: 2 * radioButtonIndicator.width
                opacity: 0.5
            }
        },
        State {
            name: "Pressed"
            when: control.pressed

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
            duration: control.animationTime
        }
        ColorAnimation {
            target: indicatorCheckedMark
            duration: control.animationTime
        }
        ColorAnimation {
            target: iconLabel
            duration: control.animationTime
        }
        PropertyAnimation {
            target: highlight
            properties: "width,opacity"
            duration: control.animationTime
        }
    }
}
