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
import Ak 1.0
import "Private"

T.SwipeDelegate {
    id: control
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
    swipe.transition: Transition {
        SmoothedAnimation {
            velocity: 3
            easing.type: Easing.InOutCubic
        }
    }

    readonly property int animationTime: 200

    contentItem: IconLabel {
        id: iconLabel
        leftPadding: swipe.right || control.swipe.position > 0.9?
                         control.icon.width:
                         0
        rightPadding: swipe.left || control.swipe.position < -0.9?
                          control.icon.width:
                          0
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
                   ThemeSettings.colorActiveHighlightedText:
                   ThemeSettings.colorActiveWindowText
        alignment: control.display === IconLabel.IconOnly
                   || control.display === IconLabel.TextUnderIcon?
                       Qt.AlignCenter | Qt.AlignVCenter:
                       Qt.AlignLeft | Qt.AlignVCenter
    }

    background: Rectangle {
        id: backgroundRect
        implicitWidth:
            AkUnit.create(128 * ThemeSettings.controlScale, "dp").pixels
        implicitHeight:
            AkUnit.create(48 * ThemeSettings.controlScale, "dp").pixels
        color: control.highlighted?
                   ThemeSettings.colorActiveHighlight:
                   ThemeSettings.colorActiveWindow

        property color iconColor:
            control.highlighted?
                ThemeSettings.colorActiveHighlightedText:
                ThemeSettings.colorActiveWindowText

        AkColorizedImage {
            source: "image://icons/swipe-left.png"
            width: control.icon.width
            height: control.icon.height
            anchors.verticalCenter: parent.verticalCenter
            visible: swipe.right || control.swipe.position > 0.9
            color: backgroundRect.iconColor
            asynchronous: true
            mipmap: true
        }
        AkColorizedImage {
            source: "image://icons/swipe-right.png"
            width: control.icon.width
            height: control.icon.height
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            visible: swipe.left || control.swipe.position < -0.9
            color: backgroundRect.iconColor
            asynchronous: true
            mipmap: true
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: iconLabel
                color:
                    control.highlighted?
                        ThemeSettings.colorDisabledHighlightedText:
                        ThemeSettings.colorDisabledWindowText
            }
            PropertyChanges {
                target: backgroundRect
                color: control.highlighted?
                           ThemeSettings.colorDisabledHighlight:
                           ThemeSettings.colorDisabledWindow
                iconColor:
                    control.highlighted?
                        ThemeSettings.colorDisabledHighlightedText:
                        ThemeSettings.colorDisabledWindowText
            }
        },
        State {
            name: "Hovered"
            when: (control.hovered
                   || control.visualFocus
                   || control.activeFocus)
                  && !control.pressed

            PropertyChanges {
                target: backgroundRect
                color:
                    control.highlighted?
                        ThemeSettings.constShade(ThemeSettings.colorActiveHighlight,
                                                 0.1):
                        ThemeSettings.shade(ThemeSettings.colorActiveWindow, -0.1)
            }
        },
        State {
            name: "Pressed"
            when: control.pressed

            PropertyChanges {
                target: backgroundRect
                color:
                    control.highlighted?
                        ThemeSettings.constShade(ThemeSettings.colorActiveHighlight,
                                                 0.2):
                        ThemeSettings.shade(ThemeSettings.colorActiveWindow, -0.2)
            }
        }
    ]

    transitions: Transition {
        ColorAnimation {
            target: backgroundRect
            duration: control.animationTime
        }
        ColorAnimation {
            target: iconLabel
            duration: control.animationTime
        }
    }
}
