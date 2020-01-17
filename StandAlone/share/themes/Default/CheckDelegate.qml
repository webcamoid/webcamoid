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
import Ak 1.0

T.CheckDelegate {
    id: checkBox
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
        let diffX = checkBox.width / 2
        let diffY = checkBox.height / 2
        let r2 = diffX * diffX + diffY * diffY

        return Math.sqrt(r2)
    }

    indicator: Item {
        id: checkBoxIndicator
        anchors.right: checkBox.right
        anchors.rightMargin: checkBox.rightPadding
        anchors.verticalCenter: checkBox.verticalCenter
        implicitWidth:
            AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight:
            AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels

        Rectangle {
            id: indicatorRect
            border.width:
                checkBox.checkState == Qt.Unchecked
                || checkBox.highlighted?
                    AkUnit.create(2 * ThemeSettings.controlScale, "dp").pixels:
                    0
            border.color:
                checkBox.highlighted?
                    checkOverlay.color:
                checkBox.checkState == Qt.Unchecked?
                    ThemeSettings.shade(ThemeSettings.colorBack, -0.5):
                    "transparent"
            color: checkBox.checkState == Qt.Unchecked
                   || checkBox.highlighted?
                       "transparent":
                       ThemeSettings.colorPrimary
            radius: AkUnit.create(4 * ThemeSettings.controlScale, "dp").pixels
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
        icon.color:
            control.highlighted?
                ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                ThemeSettings.colorText
        text: checkBox.text
        font: checkBox.font
        color: checkBox.highlighted?
                   ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                   ThemeSettings.colorText
        alignment: Qt.AlignLeft
        anchors.leftMargin: checkBox.leftPadding
        anchors.left: checkBox.left
        anchors.right: checkBoxIndicator.left
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
            color: checkBox.highlighted?
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
            color: checkBox.highlighted?
                       ThemeSettings.colorPrimary:
                       ThemeSettings.shade(ThemeSettings.colorBack, -0.1, 0)
            anchors.fill: parent
        }
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
            PropertyChanges {
                target: background
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1, 0)
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
                    checkBox.highlighted?
                        checkOverlay.color:
                    checkBox.checkState == Qt.Unchecked?
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.6):
                        "transparent"
                color: checkBox.checkState == Qt.Unchecked
                       || checkBox.highlighted?
                           "transparent":
                           ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                    0.1)
            }
            PropertyChanges {
                target: background
                color:
                    checkBox.highlighted?
                        ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                 0.1):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
        },
        State {
            name: "Pressed"
            when: checkBox.pressed

            PropertyChanges {
                target: indicatorRect
                border.color:
                    checkBox.highlighted?
                        checkOverlay.color:
                    checkBox.checkState == Qt.Unchecked?
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.7):
                        "transparent"
                color: checkBox.checkState == Qt.Unchecked
                       || checkBox.highlighted?
                           "transparent":
                           ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                    0.3)
            }
            PropertyChanges {
                target: controlPress
                radius: checkBox.pressIndicatorRadius()
                opacity: 1
            }
            PropertyChanges {
                target: background
                visible: false
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
        ColorAnimation {
            target: background
            duration: checkBox.animationTime
        }
        PropertyAnimation {
            target: controlPress
            properties: "opacity,radius"
            duration: checkBox.animationTime
        }
    }
}
