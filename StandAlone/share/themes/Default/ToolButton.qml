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

T.ToolButton {
    id: button
    flat: true
    font.bold: true
    icon.width: Ak.newUnit(4, "mm").pixels
    icon.height: Ak.newUnit(4, "mm").pixels
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    readonly property int radius: Ak.newUnit(1, "mm").pixels

    contentItem: Item {
        id: buttonContent
        implicitWidth: iconLabel.implicitWidth + Ak.newUnit(4, "mm").pixels
        implicitHeight: iconLabel.implicitHeight

        GridLayout {
            anchors.verticalCenter: buttonContent.verticalCenter
            anchors.horizontalCenter: buttonContent.horizontalCenter
            rowSpacing: Ak.newUnit(0.5, "mm").pixels
            columns: 1
            rows: 2

            IconLabel {
                id: iconLabel
                spacing: button.spacing
                mirrored: button.mirrored
                display: button.display

                icon: button.icon
                text: button.text
                font: button.font
                color: ThemeSettings.colorText
                opacity: button.enabled? 1: 0.5
            }
            Item {
                id: focusHighlight
                Layout.fillWidth: true
                visible: button.visualFocus

                Rectangle {
                    width: Ak.newUnit(2, "mm").pixels
                    height: Ak.newUnit(0.5, "mm").pixels
                    color: ThemeSettings.colorSecondary
                    anchors.horizontalCenter: focusHighlight.horizontalCenter
                }
            }
        }
    }
    background: Item {
        id: back
        implicitWidth: Ak.newUnit(2.5, "cm").pixels
        implicitHeight: Ak.newUnit(1, "cm").pixels

        Rectangle {
            id: buttonBackground
            anchors.fill: parent
            radius: button.flat? 0: button.radius
            color: button.highlighted || button.checked?
                       Qt.lighter(ThemeSettings.colorButton, 1.1):
                       ThemeSettings.colorButton

            Rectangle {
                id: buttonHighlight
                height: button.radius
                color: button.highlighted || button.checked?
                           Qt.lighter(ThemeSettings.colorButtonHighlight, 2.8):
                           ThemeSettings.colorButtonHighlight
                anchors.right: buttonBackground.right
                anchors.bottom: buttonBackground.bottom
                anchors.left: buttonBackground.left
                visible: button.checkable
            }
        }
        DropShadow {
            anchors.fill: parent
            cached: true
            horizontalOffset: button.radius / 2
            verticalOffset: button.radius / 2
            radius: button.radius
            samples: 2 * radius + 1
            color: ThemeSettings.colorShadow
            source: buttonBackground
            visible: !button.flat
        }
    }

    states: [
        State {
            name: "HighlightedHovered"
            when: (button.highlighted || button.checked)
                  && button.hovered
                  && !button.pressed

            PropertyChanges {
                target: buttonBackground
                color: Qt.lighter(ThemeSettings.colorButton, 2)
            }
            PropertyChanges {
                target: buttonHighlight
                color: Qt.lighter(ThemeSettings.colorButtonHighlight, 2.9)
            }
        },
        State {
            name: "Hovered"
            when: !(button.highlighted || button.checked)
                  && button.hovered
                  && !button.pressed

            PropertyChanges {
                target: buttonBackground
                color: Qt.lighter(ThemeSettings.colorButton, 1.9)
            }
            PropertyChanges {
                target: buttonHighlight
                color: Qt.lighter(ThemeSettings.colorButtonHighlight, 2.8)
            }
        },
        State {
            name: "Pressed"
            when: button.pressed

            PropertyChanges {
                target: buttonBackground
                color: Qt.lighter(ThemeSettings.colorButton, 2.1)
            }
            PropertyChanges {
                target: buttonHighlight
                color: Qt.lighter(ThemeSettings.colorButtonHighlight, 3)
            }
        }
    ]
}
