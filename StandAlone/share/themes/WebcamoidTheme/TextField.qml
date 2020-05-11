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

T.TextField {
    id: control
    color: activeText
    placeholderTextColor: activePlaceholderText
    selectedTextColor: activeHighlightedText
    selectionColor: activeHighlight
    padding: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
    implicitWidth:
        Math.max(contentWidth + leftPadding + rightPadding,
                 implicitBackgroundWidth + leftInset + rightInset,
                 placeholder.implicitWidth + leftPadding + rightPadding,
                 AkUnit.create(96 * AkTheme.controlScale, "dp").pixels)
    implicitHeight:
        Math.max(contentHeight + topPadding + bottomPadding,
                 implicitBackgroundHeight + topInset + bottomInset,
                 placeholder.implicitHeight + topPadding + bottomPadding,
                 AkUnit.create(36 * AkTheme.controlScale, "dp").pixels)
    hoverEnabled: true

    readonly property int animationTime: 200
    readonly property int placeHolderPadding:
        AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
    readonly property color activeBase: AkTheme.palette.active.base
    readonly property color activeDark: AkTheme.palette.active.dark
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color activeHighlightedText: AkTheme.palette.active.highlightedText
    readonly property color activeLight: AkTheme.palette.active.light
    readonly property color activeLink: AkTheme.palette.active.link
    readonly property color activeMid: AkTheme.palette.active.mid
    readonly property color activePlaceholderText: AkTheme.palette.active.placeholderText
    readonly property color activeText: AkTheme.palette.active.text
    readonly property color activeWindow: AkTheme.palette.active.window
    readonly property color disabledBase: AkTheme.palette.disabled.base
    readonly property color disabledLink: AkTheme.palette.disabled.link
    readonly property color disabledMid: AkTheme.palette.disabled.mid
    readonly property color disabledPlaceholderText: AkTheme.palette.disabled.placeholderText

    Text {
        id: placeholder
        x: control.leftPadding
        y: control.topPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        height: control.height - (control.topPadding + control.bottomPadding)
        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        verticalAlignment: control.verticalAlignment
        elide: Text.ElideRight
        renderType: control.renderType
        visible: !control.length
                 && !control.preeditText
                 && (!control.activeFocus
                     || control.horizontalAlignment !== Qt.AlignHCenter)
        linkColor: control.enabled?
                       control.activeLink:
                       control.disabledLink
        enabled: control.enabled
    }

    background: Rectangle {
        id: textAreaBackground
        color: control.activeBase
        border.color: control.activeMid
        border.width: AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
        radius: AkUnit.create(6 * AkTheme.controlScale, "dp").pixels
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0
                color:
                    control.activeWindow.hslLightness < 0.5?
                        Qt.tint(textAreaBackground.color,
                                AkTheme.shade(control.activeLight, 0, 0.25)):
                        Qt.tint(textAreaBackground.color,
                                AkTheme.shade(control.activeDark, 0, 0.25))
            }
            GradientStop {
                position: 1
                color:
                    control.activeWindow.hslLightness < 0.5?
                        Qt.tint(textAreaBackground.color,
                                AkTheme.shade(control.activeDark, 0, 0.25)):
                        Qt.tint(textAreaBackground.color,
                                AkTheme.shade(control.activeLight, 0, 0.25))
            }
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: placeholder
                color: control.disabledPlaceholderText
            }
            PropertyChanges {
                target: textAreaBackground
                color: control.disabledBase
                border.color: control.disabledMid
            }
        },
        State {
            name: "Hovered"
            when: control.enabled
                  && control.hovered
                  && !control.activeFocus

            PropertyChanges {
                target: textAreaBackground
                border.color: control.activeDark
            }
        },
        State {
            name: "Focused"
            when: control.enabled
                  && control.activeFocus

            PropertyChanges {
                target: textAreaBackground
                border.color: control.activeHighlight
                border.width:
                    AkUnit.create(2 * AkTheme.controlScale, "dp").pixels
            }
            PropertyChanges {
                target: placeholder
                color: control.activeHighlight
            }
        }
    ]

    transitions: Transition {
        ColorAnimation {
            target: placeholder
            duration: control.animationTime
        }
        PropertyAnimation {
            target: textAreaBackground
            properties: "border.color,border.width,color"
            duration: control.animationTime
        }
    }
}
