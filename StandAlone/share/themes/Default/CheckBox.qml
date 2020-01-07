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

import QtQuick 2.5
import QtQuick.Controls 2.5
import QtQuick.Templates 2.5 as T
import QtGraphicalEffects 1.0
import QtQuick.Controls.impl 2.12
import AkQml 1.0

T.CheckBox {
    id: checkBox
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
        id: checkBoxIndicator
        anchors.left: checkBox.left
        anchors.leftMargin: indicatorRect.width / 2 + checkBox.leftPadding
        anchors.verticalCenter: checkBox.verticalCenter
        implicitWidth:
            Ak.newUnit(24 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight:
            Ak.newUnit(24 * ThemeSettings.controlScale, "dp").pixels

        Rectangle {
            id: highlight
            width: checkBox.visualFocus? 2 * parent.width: 0
            height: width
            color:
                checkBox.checkState == Qt.Unchecked?
                    ThemeSettings.shade(ThemeSettings.colorBack, -0.5):
                    indicatorRect.color
            radius: width / 2
            anchors.verticalCenter: checkBoxIndicator.verticalCenter
            anchors.horizontalCenter: checkBoxIndicator.horizontalCenter
            opacity: checkBox.visualFocus? 0.5: 0
        }
        Rectangle {
            id: indicatorRect
            border.width:
                checkBox.checkState == Qt.Unchecked?
                    Ak.newUnit(2 * ThemeSettings.controlScale, "dp").pixels:
                    0
            border.color:
                checkBox.checkState == Qt.Unchecked?
                    ThemeSettings.shade(ThemeSettings.colorBack, -0.5):
                    "transparent"
            color: checkBox.checkState == Qt.Unchecked?
                       "transparent":
                       ThemeSettings.colorPrimary
            radius: Ak.newUnit(4 * ThemeSettings.controlScale, "dp").pixels
            anchors.verticalCenter: checkBoxIndicator.verticalCenter
            anchors.horizontalCenter: checkBoxIndicator.horizontalCenter
            width: Math.min(checkBoxIndicator.width, checkBoxIndicator.height)
            height: width
        }
        Image {
            id: checkImage
            asynchronous: true
            cache: true
            source: checkBox.checkState == Qt.Checked?
                        "image://icons/check":
                        "image://icons/minus"
            sourceSize: Qt.size(width, height)
            anchors.fill: indicatorRect
            visible: false
        }
        ColorOverlay {
            id: checkOverlay
            anchors.fill: checkImage
            source: checkImage
            color: ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75)
            visible: checkBox.checkState != Qt.Unchecked
        }
    }

    contentItem: IconLabel {
        id: iconLabel
        spacing: checkBox.spacing
        mirrored: checkBox.mirrored
        display: checkBox.display
        icon.name: checkBox.icon.name
        icon.source: checkBox.icon.source
        icon.width: checkBox.icon.width
        icon.height: checkBox.icon.height
        icon.color: ThemeSettings.colorText
        text: checkBox.text
        font: checkBox.font
        color: ThemeSettings.colorText
        alignment: Qt.AlignLeft
        anchors.leftMargin: indicatorRect.width / 2
        anchors.left: checkBoxIndicator.right
        anchors.rightMargin: checkBox.rightPadding
        anchors.right: checkBox.right
    }

    states: [
        State {
            name: "Disabled"
            when: !checkBox.enabled

            PropertyChanges {
                target: indicatorRect
                border.color:
                    checkBox.checkState == Qt.Unchecked?
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.1):
                        "transparent"
                color: checkBox.checkState == Qt.Unchecked?
                           "transparent":
                           ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
            PropertyChanges {
                target: checkOverlay
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
            }
            PropertyChanges {
                target: iconLabel
                icon.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
        },
        State {
            name: "Hovered"
            when: (checkBox.hovered
                   || checkBox.visualFocus
                   || checkBox.activeFocus)
                  && !checkBox.pressed

            PropertyChanges {
                target: indicatorRect
                border.color:
                    checkBox.checkState == Qt.Unchecked?
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.6):
                        "transparent"
                color: checkBox.checkState == Qt.Unchecked?
                           "transparent":
                           ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                    0.1)
            }
            PropertyChanges {
                target: highlight
                width: 2 * checkBoxIndicator.width
                opacity: 0.5
            }
        },
        State {
            name: "Pressed"
            when: checkBox.pressed

            PropertyChanges {
                target: indicatorRect
                border.color:
                    checkBox.checkState == Qt.Unchecked?
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.7):
                        "transparent"
                color: checkBox.checkState == Qt.Unchecked?
                           "transparent":
                           ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                    0.3)
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
            duration: checkBox.animationTime
        }
        ColorAnimation {
            target: checkOverlay
            duration: checkBox.animationTime
        }
        ColorAnimation {
            target: iconLabel
            duration: checkBox.animationTime
        }
        PropertyAnimation {
            target: highlight
            properties: "width,opacity"
            duration: checkBox.animationTime
        }
    }
}
