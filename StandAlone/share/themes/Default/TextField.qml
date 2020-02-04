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

T.TextField {
    id: textField
    color: ThemeSettings.colorText
    placeholderTextColor: ThemeSettings.shade(ThemeSettings.colorText, 0, 0.5)
    selectedTextColor: ThemeSettings.colorHighlightedText
    selectionColor: ThemeSettings.colorHighlight
    padding: AkUnit.create(12 * ThemeSettings.controlScale, "dp").pixels
    implicitWidth:
        Math.max(contentWidth + leftPadding + rightPadding,
                 implicitBackgroundWidth + leftInset + rightInset,
                 placeholder.implicitWidth + leftPadding + rightPadding,
                 AkUnit.create(280 * ThemeSettings.controlScale, "dp").pixels)
    implicitHeight:
        Math.max(contentHeight + topPadding + bottomPadding,
                 implicitBackgroundHeight + topInset + bottomInset,
                 placeholder.implicitHeight + topPadding + bottomPadding,
                 AkUnit.create(36 * ThemeSettings.controlScale, "dp").pixels)
    hoverEnabled: true
    selectByMouse: true

    readonly property int animationTime: 200
    readonly property int placeHolderPadding:
        AkUnit.create(4 * ThemeSettings.controlScale, "dp").pixels

    PlaceholderText {
        id: placeholder
        x: textField.leftPadding
        y: textField.topPadding
        width: textField.width - (textField.leftPadding + textField.rightPadding)
        height: textField.height - (textField.topPadding + textField.bottomPadding)
        text: textField.placeholderText
        font: textField.font
        color: textField.placeholderTextColor
        verticalAlignment: textField.verticalAlignment
        elide: Text.ElideRight
        renderType: textField.renderType
        visible: !textField.length
                 && !textField.preeditText
                 && (!textField.activeFocus
                     || textField.horizontalAlignment !== Qt.AlignHCenter)
    }

    background: Rectangle {
        id: textAreaBackground
        color: ThemeSettings.colorBase
        border.color: ThemeSettings.colorMid
        border.width: AkUnit.create(1 * ThemeSettings.controlScale, "dp").pixels
        radius: AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
        anchors.fill: parent
    }

    states: [
        State {
            name: "Disabled"
            when: !textField.enabled

            PropertyChanges {
                target: placeholder
                color: ThemeSettings.shade(ThemeSettings.colorWindow, -0.3)
            }
            PropertyChanges {
                target: textAreaBackground
                color: ThemeSettings.shade(ThemeSettings.colorWindow, -0.1)
                border.color:
                    ThemeSettings.shade(ThemeSettings.colorWindow, -0.3)
            }
        },
        State {
            name: "Hovered"
            when: textField.enabled
                  && textField.hovered
                  && !textField.activeFocus

            PropertyChanges {
                target: textAreaBackground
                border.color: ThemeSettings.colorDark
            }
        },
        State {
            name: "Focused"
            when: textField.enabled
                  && textField.activeFocus

            PropertyChanges {
                target: textAreaBackground
                border.color: ThemeSettings.colorHighlight
                border.width:
                    AkUnit.create(2 * ThemeSettings.controlScale, "dp").pixels
            }
            PropertyChanges {
                target: placeholder
                color: ThemeSettings.colorHighlight
            }
        }
    ]

    transitions: Transition {
        ColorAnimation {
            target: placeholder
            duration: textField.animationTime
        }
        PropertyAnimation {
            target: textAreaBackground
            properties: "border.color,border.width,color"
            duration: textField.animationTime
        }
    }
}
