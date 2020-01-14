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

import QtQuick 2.0
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Templates 2.5 as T
import QtGraphicalEffects 1.0
import QtQuick.Controls.impl 2.12
import AkQml 1.0

T.TextField {
    id: textField
    color: ThemeSettings.colorText
    placeholderTextColor: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
    selectedTextColor: ThemeSettings.contrast(selectionColor, 0.75)
    selectionColor: ThemeSettings.colorPrimary
    padding: Ak.newUnit(12 * ThemeSettings.controlScale, "dp").pixels
    implicitWidth: Math.max(contentWidth + leftPadding + rightPadding,
                            implicitBackgroundWidth + leftInset + rightInset,
                            placeholder.implicitWidth + leftPadding + rightPadding,
                            Ak.newUnit(280 * ThemeSettings.controlScale,
                                       "dp").pixels)
    implicitHeight: Math.max(contentHeight + topPadding + bottomPadding,
                             implicitBackgroundHeight + topInset + bottomInset,
                             placeholder.implicitHeight + topPadding + bottomPadding,
                             Ak.newUnit(36 * ThemeSettings.controlScale,
                                        "dp").pixels)
    hoverEnabled: true
    selectByMouse: true

    readonly property int animationTime: 100
    readonly property int placeHolderPadding:
        Ak.newUnit(4 * ThemeSettings.controlScale, "dp").pixels
    readonly property int placeHolderFontSize: 8

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
        color: Qt.hsla(0, 0, 0, 0)
        border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
        border.width: Ak.newUnit(1 * ThemeSettings.controlScale, "dp").pixels
        radius: Ak.newUnit(8 * ThemeSettings.controlScale, "dp").pixels
        anchors.fill: parent
    }

    states: [
        State {
            name: "Disabled"
            when: !textField.enabled

            PropertyChanges {
                target: placeholder
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
            }
            PropertyChanges {
                target: textAreaBackground
                border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
            }
        },
        State {
            name: "Hovered"
            when: textField.enabled
                  && textField.hovered
                  && !textField.activeFocus

            PropertyChanges {
                target: textAreaBackground
                border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
        },
        State {
            name: "Focused"
            when: textField.enabled
                  && textField.activeFocus

            PropertyChanges {
                target: textAreaBackground
                border.color: ThemeSettings.colorPrimary
                border.width: Ak.newUnit(2 * ThemeSettings.controlScale,
                                         "dp").pixels
            }
            PropertyChanges {
                target: placeholder
                color: ThemeSettings.colorPrimary
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
            properties: "border"
            duration: textField.animationTime
        }
    }
}
