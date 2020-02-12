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

T.MenuItem {
    id: menuItem
    implicitWidth:
        Math.max(implicitBackgroundWidth + leftInset + rightInset,
                 implicitContentWidth + leftPadding + rightPadding
                 + menuItemArrow.width
                 + (menuItem.checkable? AkUnit.create(20 * ThemeSettings.controlScale, "dp").pixels: 0)
                 + AkUnit.create(48 * ThemeSettings.controlScale, "dp").pixels )
    implicitHeight:
        Math.max(implicitBackgroundHeight + topInset + bottomInset,
                 implicitContentHeight + topPadding + bottomPadding,
                 implicitIndicatorHeight + topPadding + bottomPadding)
    rightPadding: AkUnit.create(16 * ThemeSettings.controlScale, "dp").pixels
    spacing: AkUnit.create(20 * ThemeSettings.controlScale, "dp").pixels
    icon.width: AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels
    icon.height: AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels
    clip: true
    hoverEnabled: true

    readonly property int animationTime: 200

    // Checked indicator
    indicator: Item {
        id: menuItemCheck
        width: menuItem.checkable?
                   AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels: 0
        height: menuItem.checkable?
                    AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels: 0
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: AkUnit.create(16 * ThemeSettings.controlScale, "dp").pixels
        visible: menuItem.checkable && menuItem.checked

        AkColorizedImage {
            id: checkImage
            width: menuItemCheck.width / 2
            height: menuItemCheck.height / 2
            anchors.verticalCenter: menuItemCheck.verticalCenter
            anchors.horizontalCenter: menuItemCheck.horizontalCenter
            source: "image://icons/check"
            color:
                menuItem.highlighted?
                    ThemeSettings.colorActiveHighlightedText:
                    ThemeSettings.colorActiveWindowText
            asynchronous: true
            mipmap: true
        }
    }

    // >
    arrow: Item {
        id: menuItemArrow
        width: visible? AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels: 0
        height: visible? AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels: 0
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: AkUnit.create(16 * ThemeSettings.controlScale, "dp").pixels
        visible: menuItem.subMenu

        AkColorizedImage {
            id: arrowImage
            width: menuItemArrow.width / 2
            height: menuItemArrow.height / 2
            anchors.verticalCenter: menuItemArrow.verticalCenter
            anchors.horizontalCenter: menuItemArrow.horizontalCenter
            source: "image://icons/right"
            color:
                menuItem.highlighted?
                    ThemeSettings.colorActiveHighlightedText:
                    ThemeSettings.colorActiveWindowText
            asynchronous: true
            mipmap: true
        }
    }

    contentItem: IconLabel {
        id: iconLabel
        spacing: menuItem.spacing
        mirrored: menuItem.mirrored
        display: menuItem.display
        iconName: menuItem.icon.name
        iconSource: menuItem.icon.source
        iconWidth: menuItem.icon.width
        iconHeight: menuItem.icon.height
        text: menuItem.text
        anchors.left: menuItemCheck.right
        anchors.leftMargin:
            menuItem.checkable?
                AkUnit.create(20 * ThemeSettings.controlScale, "dp").pixels: 0
        anchors.right: menuItemArrow.left
        font: menuItem.font
        color: menuItem.highlighted?
                   ThemeSettings.colorActiveHighlightedText:
                   ThemeSettings.colorActiveWindowText
        alignment: Qt.AlignLeft | Qt.AlignVCenter
    }

    background: Rectangle {
        id: background
        implicitWidth: AkUnit.create(128 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight: AkUnit.create(48 * ThemeSettings.controlScale, "dp").pixels
        color: menuItem.highlighted?
                   ThemeSettings.colorActiveHighlight:
                   ThemeSettings.shade(ThemeSettings.colorActiveWindow, 0, 0)
    }

    states: [
        State {
            name: "Disabled"
            when: !menuItem.enabled

            PropertyChanges {
                target: iconLabel
                color: menuItem.highlighted?
                           ThemeSettings.colorDisabledHighlightedText:
                           ThemeSettings.colorDisabledWindowText
            }
            PropertyChanges {
                target: checkImage
                color:
                    menuItem.highlighted?
                        ThemeSettings.colorDisabledHighlightedText:
                        ThemeSettings.colorDisabledWindowText
            }
            PropertyChanges {
                target: arrowImage
                color:
                    menuItem.highlighted?
                        ThemeSettings.colorDisabledHighlightedText:
                        ThemeSettings.colorDisabledWindowText
            }
            PropertyChanges {
                target: background
                color: menuItem.highlighted?
                           ThemeSettings.colorDisabledHighlight:
                           ThemeSettings.shade(ThemeSettings.colorDisabledWindow, 0, 0)
            }
        },
        State {
            name: "Hovered"
            when: menuItem.enabled
                  && menuItem.hovered
                  && !menuItem.visualFocus
                  && !menuItem.pressed

            PropertyChanges {
                target: checkImage
                color:
                    menuItem.highlighted?
                        ThemeSettings.colorActiveHighlightedText:
                        ThemeSettings.shade(ThemeSettings.colorActiveWindow, -0.6)
            }
            PropertyChanges {
                target: arrowImage
                color:
                    menuItem.highlighted?
                        ThemeSettings.colorActiveHighlightedText:
                        ThemeSettings.shade(ThemeSettings.colorActiveWindow, -0.6)
            }
            PropertyChanges {
                target: background
                color:
                    menuItem.highlighted?
                        ThemeSettings.constShade(ThemeSettings.colorActiveHighlight, 0.1):
                        ThemeSettings.shade(ThemeSettings.colorActiveWindow, -0.1)
            }
        },
        State {
            name: "Focused"
            when: menuItem.enabled
                  && menuItem.visualFocus
                  && !menuItem.pressed

            PropertyChanges {
                target: checkImage
                color:
                    menuItem.highlighted?
                        ThemeSettings.colorActiveHighlightedText:
                        ThemeSettings.shade(ThemeSettings.colorActiveWindow, -0.6)
            }
            PropertyChanges {
                target: arrowImage
                color:
                    menuItem.highlighted?
                        ThemeSettings.colorActiveHighlightedText:
                        ThemeSettings.shade(ThemeSettings.colorActiveWindow, -0.6)
            }
            PropertyChanges {
                target: background
                color:
                    menuItem.highlighted?
                        ThemeSettings.constShade(ThemeSettings.colorActiveHighlight, 0.2):
                        ThemeSettings.shade(ThemeSettings.colorActiveWindow, -0.2)
            }
        },
        State {
            name: "Pressed"
            when: menuItem.enabled
                  && menuItem.pressed

            PropertyChanges {
                target: checkImage
                color:
                    menuItem.highlighted?
                        ThemeSettings.colorActiveHighlightedText:
                        ThemeSettings.shade(ThemeSettings.colorActiveWindow, -0.7)
            }
            PropertyChanges {
                target: arrowImage
                color:
                    menuItem.highlighted?
                        ThemeSettings.colorActiveHighlightedText:
                        ThemeSettings.shade(ThemeSettings.colorActiveWindow, -0.7)
            }
            PropertyChanges {
                target: background
                color:
                    menuItem.highlighted?
                        ThemeSettings.constShade(ThemeSettings.colorActiveHighlight, 0.3):
                        ThemeSettings.shade(ThemeSettings.colorActiveWindow, -0.3)
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: iconLabel
            properties: "opacity"
            duration: menuItem.animationTime
        }
        PropertyAnimation {
            target: checkImage
            properties: "color"
            duration: menuItem.animationTime
        }
        PropertyAnimation {
            target: arrowImage
            properties: "color"
            duration: menuItem.animationTime
        }
        ColorAnimation {
            target: background
            duration: menuItem.animationTime
        }
    }
}
