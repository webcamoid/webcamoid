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

T.CheckDelegate {
    id: control
    icon.width: AkUnit.create(18 * AkTheme.controlScale, "dp").pixels
    icon.height: AkUnit.create(18 * AkTheme.controlScale, "dp").pixels
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + implicitIndicatorWidth
                            + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)
    padding: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
    leftPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    rightPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    spacing: AkUnit.create(20 * AkTheme.controlScale, "dp").pixels
    hoverEnabled: true
    clip: true

    readonly property int animationTime: 200

    indicator: Item {
        id: checkBoxIndicator
        anchors.right: control.right
        anchors.rightMargin: control.rightPadding
        anchors.verticalCenter: control.verticalCenter
        implicitWidth:
            AkUnit.create(24 * AkTheme.controlScale, "dp").pixels
        implicitHeight:
            AkUnit.create(24 * AkTheme.controlScale, "dp").pixels

        Rectangle {
            id: indicatorRect
            border.width:
                control.checkState == Qt.Unchecked
                || control.highlighted?
                    AkUnit.create(2 * AkTheme.controlScale, "dp").pixels:
                    0
            border.color:
                control.checkState != Qt.Unchecked?
                    "transparent":
                control.highlighted?
                    AkTheme.palette.active.highlightedText:
                    AkTheme.palette.active.windowText
            color: control.checkState == Qt.Unchecked?
                       "transparent":
                   control.highlighted?
                       AkTheme.palette.active.highlightedText:
                       AkTheme.palette.active.windowText
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
            color: control.highlighted?
                       AkTheme.palette.active.highlight:
                       AkTheme.palette.active.window
            asynchronous: true
            mipmap: true
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
        text: control.text
        font: control.font
        color: control.highlighted?
                   AkTheme.palette.active.highlightedText:
                   AkTheme.palette.active.windowText
        enabled: control.enabled
        alignment: Qt.AlignLeft | Qt.AlignVCenter
        anchors.leftMargin: control.leftPadding
        anchors.left: control.left
        anchors.right: checkBoxIndicator.left
    }

    background: Rectangle {
        id: background
        implicitWidth:
            AkUnit.create(128 * AkTheme.controlScale, "dp").pixels
        implicitHeight:
            AkUnit.create(48 * AkTheme.controlScale, "dp").pixels
        color: control.highlighted?
                   AkTheme.palette.active.highlight:
                   AkTheme.shade(AkTheme.palette.active.window, 0, 0)
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: indicatorRect
                border.color:
                    control.checkState != Qt.Unchecked?
                        "transparent":
                    control.highlighted?
                        AkTheme.palette.disabled.highlightedText:
                        AkTheme.palette.disabled.windowText
                color: control.checkState == Qt.Unchecked?
                           "transparent":
                       control.highlighted?
                           AkTheme.palette.disabled.highlightedText:
                           AkTheme.palette.disabled.windowText
            }
            PropertyChanges {
                target: checkImage
                color: control.highlighted?
                           AkTheme.palette.disabled.highlight:
                           AkTheme.palette.disabled.window
            }
            PropertyChanges {
                target: iconLabel
                color:
                    control.highlighted?
                        AkTheme.palette.disabled.highlightedText:
                        AkTheme.palette.disabled.windowText
            }
            PropertyChanges {
                target: background
                color:
                    control.highlighted?
                        AkTheme.palette.disabled.highlight:
                        AkTheme.shade(AkTheme.palette.disabled.window, 0, 0)
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
                        AkTheme.constShade(AkTheme.palette.active.highlight,
                                                 0.1):
                        AkTheme.shade(AkTheme.palette.active.window, -0.1)
            }
        },
        State {
            name: "Pressed"
            when: control.pressed

            PropertyChanges {
                target: background
                color:
                    control.highlighted?
                        AkTheme.constShade(AkTheme.palette.active.highlight,
                                                 0.2):
                        AkTheme.shade(AkTheme.palette.active.window, -0.2)
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
        ColorAnimation {
            target: background
            duration: control.animationTime
        }
    }
}
