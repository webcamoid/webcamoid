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

T.SpinBox {
    id: control
    implicitWidth: AkUnit.create(96 * ThemeSettings.controlScale, "dp").pixels
    implicitHeight: AkUnit.create(32 * ThemeSettings.controlScale, "dp").pixels
    padding: AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
    hoverEnabled: true

    readonly property int animationTime: 200

    validator: IntValidator {
        locale: control.locale.name
        bottom: Math.min(control.from, control.to)
        top: Math.max(control.from, control.to)
    }

    contentItem: TextInput {
        id: spinBoxText
        text: control.displayText
        font: control.font
        color: ThemeSettings.colorActiveText
        selectionColor: ThemeSettings.colorActiveHighlight
        selectedTextColor: ThemeSettings.colorActiveHighlightedText
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        anchors.left: control.mirrored? upIndicator.left: downIndicator.right
        anchors.right: control.mirrored? downIndicator.right: upIndicator.left
        anchors.top: control.top
        anchors.bottom: control.bottom
        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: control.inputMethodHints
        selectByMouse: true
    }

    up.indicator: Item {
        id: upIndicator
        x: control.mirrored? 0: parent.width - width
        width: parent.height
        height: parent.height
        visible: enabled
        opacity: control.up.hovered? 1: 0.5

        Text {
            id: upIndicatorText
            text: "+"
            font.bold: true
            font.pixelSize: control.font.pixelSize * 2
            color: ThemeSettings.colorActiveText
            anchors.fill: parent
            fontSizeMode: Text.Fit
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    down.indicator: Item {
        id: downIndicator
        x: control.mirrored? parent.width - width: 0
        width: parent.height
        height: parent.height
        visible: enabled
        opacity: control.down.hovered? 1: 0.5

        Text {
            id: downIndicatorText
            text: "-"
            font.bold: true
            font.pixelSize: control.font.pixelSize * 2
            color: ThemeSettings.colorActiveText
            anchors.fill: parent
            fontSizeMode: Text.Fit
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    background: Rectangle {
        id: background
        color: ThemeSettings.colorActiveBase
        border.color: ThemeSettings.colorActiveMid
        border.width: AkUnit.create(1 * ThemeSettings.controlScale, "dp").pixels
        radius: AkUnit.create(6 * ThemeSettings.controlScale, "dp").pixels
        width: control.width
        height: control.height
        gradient: Gradient {
            GradientStop {
                position: 0
                color:
                    ThemeSettings.colorActiveWindow.hslLightness < 0.5?
                        Qt.tint(background.color,
                                ThemeSettings.shade(ThemeSettings.colorActiveLight, 0, 0.25)):
                        Qt.tint(background.color,
                                ThemeSettings.shade(ThemeSettings.colorActiveDark, 0, 0.25))
            }
            GradientStop {
                position: 1
                color:
                    ThemeSettings.colorActiveWindow.hslLightness < 0.5?
                        Qt.tint(background.color,
                                ThemeSettings.shade(ThemeSettings.colorActiveDark, 0, 0.25)):
                        Qt.tint(background.color,
                                ThemeSettings.shade(ThemeSettings.colorActiveLight, 0, 0.25))
            }
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: spinBoxText
                color: ThemeSettings.colorDisabledText
            }
            PropertyChanges {
                target: downIndicatorText
                color: ThemeSettings.colorDisabledText
            }
            PropertyChanges {
                target: upIndicatorText
                color: ThemeSettings.colorDisabledText
            }
            PropertyChanges {
                target: background
                color: ThemeSettings.colorDisabledBase
                border.color: ThemeSettings.colorDisabledMid
            }
        },
        State {
            name: "Hovered"
            when: control.enabled
                  && (control.hovered
                      || control.down.hovered
                      || control.up.hovered)
                  && !(control.activeFocus || control.visualFocus)

            PropertyChanges {
                target: background
                border.color: ThemeSettings.colorActiveDark
            }
        },
        State {
            name: "Focused"
            when: control.enabled
                  && (control.activeFocus || control.visualFocus)

            PropertyChanges {
                target: background
                border.color: ThemeSettings.colorActiveHighlight
                border.width:
                    AkUnit.create(2 * ThemeSettings.controlScale, "dp").pixels
            }
        }
    ]

    transitions: Transition {
        ColorAnimation {
            target: spinBoxText
            duration: control.animationTime
        }
        PropertyAnimation {
            target: background
            properties: "border.color,border.width,color"
            duration: control.animationTime
        }
        ColorAnimation {
            target: downIndicatorText
            duration: control.animationTime
        }
        ColorAnimation {
            target: upIndicatorText
            duration: control.animationTime
        }
    }
}
