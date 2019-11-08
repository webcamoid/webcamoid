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
    placeholderTextColor: ThemeSettings.colorBack
    selectedTextColor: ThemeSettings.colorText
    selectionColor: Qt.lighter(ThemeSettings.colorSecondary,
                               0.65)
    padding: Ak.newUnit(12 * ThemeSettings.constrolScale, "dp").pixels
    implicitWidth: Math.max(contentWidth + leftPadding + rightPadding,
                            implicitBackgroundWidth + leftInset + rightInset,
                            placeholder.implicitWidth + leftPadding + rightPadding,
                            Ak.newUnit(280 * ThemeSettings.constrolScale,
                                       "dp").pixels)
    implicitHeight: Math.max(contentHeight + topPadding + bottomPadding,
                             implicitBackgroundHeight + topInset + bottomInset,
                             placeholder.implicitHeight + topPadding + bottomPadding,
                             Ak.newUnit(36 * ThemeSettings.constrolScale,
                                        "dp").pixels)

    readonly property int animationTime: 100
    readonly property int placeHolderPadding:
        Ak.newUnit(4 * ThemeSettings.constrolScale, "dp").pixels
    readonly property int placeHolderFontSize: 8

    Rectangle {
        id: placeholderRectangle
        color: Qt.lighter(ThemeSettings.colorBack, 0.2)
        width: placeholder.contentWidth + 2 * textField.placeHolderPadding
        height: placeholder.contentHeight
        x: placeholder.x - textField.placeHolderPadding
        y: placeholder.y - textField.placeHolderPadding
        opacity: 0
        visible: textField.placeholderText.length
    }

    PlaceholderText {
        id: placeholder
        x: textField.leftPadding
        y: textField.topPadding

        text: textField.placeholderText
        color: ThemeSettings.colorBack
        verticalAlignment: textField.verticalAlignment
        elide: Text.ElideRight
        renderType: textField.renderType
        visible: textField.placeholderText.length
    }

    background: Rectangle {
        id: textAreaBackground
        anchors.fill: parent
        color: Qt.hsla(0, 0, 0, 0)
        border.color: ThemeSettings.colorBack
        border.width: Ak.newUnit(1 * ThemeSettings.constrolScale,
                                 "dp").pixels
        radius: Ak.newUnit(8 * ThemeSettings.constrolScale,
                           "dp").pixels
    }

    states: [
        State {
            name: "Disabled"
            when: !textField.enabled
                  && !textField.hovered
                  && !textField.activeFocus
                  && !textField.visualFocus
                  && !textField.length

            PropertyChanges {
                target: textField
                opacity: 0.5
            }
        },
        State {
            name: "Hovered"
            when: textField.enabled
                  && textField.hovered
                  && !textField.activeFocus
                  && !textField.length

            PropertyChanges {
                target: textAreaBackground
                border.color: Qt.lighter(ThemeSettings.colorBack, 1.5)
            }
        },
        State {
            name: "Focused"
            when: textField.enabled
                  && textField.activeFocus
                  && !textField.length

            PropertyChanges {
                target: textAreaBackground
                border.color: ThemeSettings.colorPrimary
                border.width: Ak.newUnit(2 * ThemeSettings.constrolScale,
                                         "dp").pixels
            }
            PropertyChanges {
                target: placeholder
                color: ThemeSettings.colorPrimary
            }
        },
        State {
            name: "Active"
            when: textField.enabled
                  && !textField.hovered
                  && !textField.activeFocus
                  && !textField.visualFocus
                  && textField.length

            PropertyChanges {
                target: placeholderRectangle
                opacity: 1
            }
            PropertyChanges {
                target: placeholder
                x: textField.leftPadding
                   + Ak.newUnit(14 * ThemeSettings.constrolScale, "dp").pixels
                y: - placeholder.contentHeight / 2
                color: ThemeSettings.colorText
                font.pointSize: textField.placeHolderFontSize
            }
        },
        State {
            name: "DisabledActive"
            when: !textField.enabled
                  && !textField.hovered
                  && !textField.activeFocus
                  && !textField.visualFocus
                  && textField.length

            PropertyChanges {
                target: textField
                opacity: 0.5
            }
            PropertyChanges {
                target: placeholderRectangle
                opacity: 1
            }
            PropertyChanges {
                target: placeholder
                x: textField.leftPadding
                   + Ak.newUnit(14 * ThemeSettings.constrolScale, "dp").pixels
                y: - placeholder.contentHeight / 2
                font.pointSize: textField.placeHolderFontSize
            }
        },
        State {
            name: "HoveredActive"
            when: textField.enabled
                  && textField.hovered
                  && !textField.activeFocus
                  && textField.length

            PropertyChanges {
                target: textAreaBackground
                border.color: Qt.lighter(ThemeSettings.colorBack, 1.5)
            }
            PropertyChanges {
                target: placeholderRectangle
                opacity: 1
            }
            PropertyChanges {
                target: placeholder
                x: textField.leftPadding
                   + Ak.newUnit(14 * ThemeSettings.constrolScale, "dp").pixels
                y: - placeholder.contentHeight / 2
                color: ThemeSettings.colorText
                font.pointSize: textField.placeHolderFontSize
            }
        },
        State {
            name: "FocusedActive"
            when: textField.enabled
                  && textField.activeFocus
                  && textField.length

            PropertyChanges {
                target: textAreaBackground
                border.color: ThemeSettings.colorPrimary
                border.width: Ak.newUnit(2 * ThemeSettings.constrolScale,
                                         "dp").pixels
            }
            PropertyChanges {
                target: placeholderRectangle
                opacity: 1
            }
            PropertyChanges {
                target: placeholder
                x: textField.leftPadding
                   + Ak.newUnit(14 * ThemeSettings.constrolScale, "dp").pixels
                y: - placeholder.contentHeight / 2
                color: ThemeSettings.colorPrimary
                font.pointSize: textField.placeHolderFontSize
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: placeholderRectangle
            properties: "opacity,x,y"
            duration: textField.animationTime
        }
        PropertyAnimation {
            target: placeholder
            properties: "font,opacity,x,y"
            duration: textField.animationTime
        }
        PropertyAnimation {
            target: textAreaBackground
            properties: "border"
            duration: textField.animationTime
        }
    }
}
