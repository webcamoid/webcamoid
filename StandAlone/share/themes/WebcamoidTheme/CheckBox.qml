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
import Ak 1.0
import "Private"

T.CheckBox {
    id: control
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

    readonly property int animationTime: 200
    readonly property color activeDark: AkTheme.palette.active.dark
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color activeHighlightedText: AkTheme.palette.active.highlightedText
    readonly property color activeWindowText: AkTheme.palette.active.windowText
    readonly property color disabledDark: AkTheme.palette.disabled.dark
    readonly property color disabledHighlight: AkTheme.palette.disabled.highlight
    readonly property color disabledHighlightedText: AkTheme.palette.disabled.highlightedText
    readonly property color disabledWindowText: AkTheme.palette.disabled.windowText

    indicator: Item {
        id: checkBoxIndicator
        anchors.left: control.left
        anchors.leftMargin: indicatorRect.width / 2 + control.leftPadding
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
                control.checkState == Qt.Unchecked?
                    control.activeDark:
                    indicatorRect.color
            radius: width / 2
            anchors.verticalCenter: checkBoxIndicator.verticalCenter
            anchors.horizontalCenter: checkBoxIndicator.horizontalCenter
            opacity: control.visualFocus? 0.5: 0
        }
        Rectangle {
            id: indicatorRect
            border.width:
                control.checkState == Qt.Unchecked?
                    AkUnit.create(2 * AkTheme.controlScale, "dp").pixels:
                    0
            border.color:
                control.checkState == Qt.Unchecked?
                    control.activeDark:
                    "transparent"
            color: control.checkState == Qt.Unchecked?
                       "transparent":
                       control.activeHighlight
            radius: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
            anchors.verticalCenter: checkBoxIndicator.verticalCenter
            anchors.horizontalCenter: checkBoxIndicator.horizontalCenter
            width: Math.min(checkBoxIndicator.width, checkBoxIndicator.height)
            height: width
        }
        AkColorizedImage {
            id: checkImage
            source: control.checkState == Qt.Checked?
                        "image://icons/check":
                        "image://icons/minus"
            anchors.fill: indicatorRect
            visible: control.checkState != Qt.Unchecked
            color: control.activeHighlightedText
            asynchronous: true
            mipmap: true
        }
    }

    contentItem: Item {
        anchors.leftMargin: indicatorRect.width / 2
        anchors.left: checkBoxIndicator.right
        anchors.rightMargin: control.rightPadding
        anchors.right: control.right

        IconLabel {
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
            anchors.verticalCenter: control.contentItem.verticalCenter
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: indicatorRect
                border.color:
                    control.checkState == Qt.Unchecked?
                        control.disabledDark:
                        "transparent"
                color: control.checkState == Qt.Unchecked?
                           "transparent":
                           control.disabledHighlight
            }
            PropertyChanges {
                target: checkImage
                color: control.disabledHighlightedText
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
                target: indicatorRect
                color: control.checkState == Qt.Unchecked?
                           "transparent":
                           AkTheme.constShade(control.activeHighlight, 0.1)
            }
            PropertyChanges {
                target: highlight
                width: 2 * checkBoxIndicator.width
                opacity: 0.5
            }
        },
        State {
            name: "Pressed"
            when: control.pressed

            PropertyChanges {
                target: indicatorRect
                color: control.checkState == Qt.Unchecked?
                           "transparent":
                           AkTheme.constShade(control.activeHighlight, 0.3)
            }
            PropertyChanges {
                target: highlight
                width: 2 * checkBoxIndicator.width
                opacity: 0.75
            }
        }
    ]

    transitions: Transition {
        ColorAnimation {
            target: indicatorRect
            duration: control.animationTime
        }
        PropertyAnimation {
            target: checkImage
            properties: "color"
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
