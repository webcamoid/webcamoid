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
    color: AkTheme.palette.active.text
    placeholderTextColor: AkTheme.palette.active.placeholderText
    selectedTextColor: AkTheme.palette.active.highlightedText
    selectionColor: AkTheme.palette.active.highlight
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
                       AkTheme.palette.active.link:
                       AkTheme.palette.disabled.link
        enabled: control.enabled
    }

    background: Rectangle {
        id: textAreaBackground
        color: AkTheme.palette.active.base
        border.color: AkTheme.palette.active.mid
        border.width: AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
        radius: AkUnit.create(6 * AkTheme.controlScale, "dp").pixels
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0
                color:
                    AkTheme.palette.active.window.hslLightness < 0.5?
                        Qt.tint(textAreaBackground.color,
                                AkTheme.shade(AkTheme.palette.active.light, 0, 0.25)):
                        Qt.tint(textAreaBackground.color,
                                AkTheme.shade(AkTheme.palette.active.dark, 0, 0.25))
            }
            GradientStop {
                position: 1
                color:
                    AkTheme.palette.active.window.hslLightness < 0.5?
                        Qt.tint(textAreaBackground.color,
                                AkTheme.shade(AkTheme.palette.active.dark, 0, 0.25)):
                        Qt.tint(textAreaBackground.color,
                                AkTheme.shade(AkTheme.palette.active.light, 0, 0.25))
            }
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: placeholder
                color: AkTheme.palette.disabled.placeholderText
            }
            PropertyChanges {
                target: textAreaBackground
                color: AkTheme.palette.disabled.base
                border.color: AkTheme.palette.disabled.mid
            }
        },
        State {
            name: "Hovered"
            when: control.enabled
                  && control.hovered
                  && !control.activeFocus

            PropertyChanges {
                target: textAreaBackground
                border.color: AkTheme.palette.active.dark
            }
        },
        State {
            name: "Focused"
            when: control.enabled
                  && control.activeFocus

            PropertyChanges {
                target: textAreaBackground
                border.color: AkTheme.palette.active.highlight
                border.width:
                    AkUnit.create(2 * AkTheme.controlScale, "dp").pixels
            }
            PropertyChanges {
                target: placeholder
                color: AkTheme.palette.active.highlight
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
