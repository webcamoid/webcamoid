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

T.SpinBox {
    id: spinBox
    implicitWidth: Ak.newUnit(96 * ThemeSettings.controlScale, "dp").pixels
    implicitHeight: Ak.newUnit(32 * ThemeSettings.controlScale, "dp").pixels
    hoverEnabled: true

    readonly property int animationTime: 200

    function pressIndicatorRadius()
    {
        let diffX = spinBox.width - spinBox.height / 2
        let diffY = spinBox.height / 2
        let r2 = diffX * diffX + diffY * diffY

        return Math.sqrt(r2)
    }

    validator: IntValidator {
        locale: spinBox.locale.name
        bottom: Math.min(spinBox.from, spinBox.to)
        top: Math.max(spinBox.from, spinBox.to)
    }

    contentItem: TextInput {
        id: spinBoxText
        text: spinBox.displayText
        font: spinBox.font
        color: ThemeSettings.colorText
        selectionColor: ThemeSettings.colorPrimary
        selectedTextColor: ThemeSettings.contrast(selectionColor, 0.75)
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        anchors.left: spinBox.mirrored? upIndicator.left: downIndicator.right
        anchors.right: spinBox.mirrored? downIndicator.right: upIndicator.left
        anchors.top: spinBox.top
        anchors.bottom: spinBox.bottom
        readOnly: !spinBox.editable
        validator: spinBox.validator
        inputMethodHints: spinBox.inputMethodHints
        selectByMouse: true
    }

    up.indicator: Rectangle {
        id: upIndicator
        x: spinBox.mirrored? 0 : parent.width - width
        width: parent.height
        height: parent.height
        color: Qt.hsla(0, 0, 0, 0)

        Text {
            id: upIndicatorText
            text: "+"
            font.bold: true
            font.pixelSize: spinBox.font.pixelSize * 2
            color: enabled?
                       ThemeSettings.colorPrimary:
                       ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            anchors.fill: parent
            fontSizeMode: Text.Fit
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    down.indicator: Rectangle {
        id: downIndicator
        x: spinBox.mirrored? parent.width - width : 0
        width: parent.height
        height: parent.height
        color: Qt.hsla(0, 0, 0, 0)

        Text {
            id: downIndicatorText
            text: "-"
            font.bold: true
            font.pixelSize: spinBox.font.pixelSize * 2
            color: enabled?
                       ThemeSettings.colorPrimary:
                       ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            anchors.fill: parent
            fontSizeMode: Text.Fit
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    background: Item {
        anchors.fill: parent

        Rectangle{
            id: backgroundMask
            anchors.fill: parent
            radius: background.radius
            color: Qt.hsla(0, 0, 0, 1)
            visible: false
        }
        Item {
            anchors.fill: parent
            clip: true
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: backgroundMask
            }

            Rectangle{
                id: downPressIndicator
                radius: 0
                width: 2 * radius
                height: 2 * radius
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.1,
                                                0.3)
                x: parent.height / 2 - radius
                y: parent.height / 2 - radius
                opacity: 0
            }
            Rectangle{
                id: upPressIndicator
                radius: 0
                width: 2 * radius
                height: 2 * radius
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.1,
                                                0.3)
                x: parent.width - parent.height / 2 - radius
                y: parent.height / 2 - radius
                opacity: 0
            }
        }
        Rectangle {
            id: background
            color: Qt.hsla(0, 0, 0, 0)
            border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            border.width: Ak.newUnit(1 * ThemeSettings.controlScale, "dp").pixels
            radius: Ak.newUnit(6 * ThemeSettings.controlScale, "dp").pixels
            anchors.fill: parent
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !spinBox.enabled
                  && !spinBox.hovered
                  && !spinBox.activeFocus
                  && !spinBox.visualFocus

            PropertyChanges {
                target: spinBoxText
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            }
            PropertyChanges {
                target: downIndicatorText
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            }
            PropertyChanges {
                target: upIndicatorText
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            }
        },
        State {
            name: "Hovered"
            when: spinBox.enabled
                  && spinBox.hovered
                  && !spinBox.down.hovered
                  && !spinBox.up.hovered
                  && !spinBox.down.pressed
                  && !spinBox.up.pressed
                  && !spinBox.activeFocus

            PropertyChanges {
                target: background
                border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
        },
        State {
            name: "DownDisabledHovered"
            when: !upIndicator.enabled
                  && spinBox.down.hovered
                  && !spinBox.down.pressed
                  && !spinBox.activeFocus

            PropertyChanges {
                target: background
                border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
        },
        State {
            name: "DownHovered"
            when: upIndicator.enabled
                  && spinBox.down.hovered
                  && !spinBox.down.pressed
                  && !spinBox.activeFocus

            PropertyChanges {
                target: background
                border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
            PropertyChanges {
                target: downIndicatorText
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1)
            }
        },
        State {
            name: "DownPressed"
            when: upIndicator.enabled
                  && spinBox.down.pressed
                  && !spinBox.activeFocus

            PropertyChanges {
                target: background
                border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
            PropertyChanges {
                target: downPressIndicator
                radius: spinBox.pressIndicatorRadius()
                opacity: 1.0
            }
            PropertyChanges {
                target: downIndicatorText
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.3)
            }
        },
        State {
            name: "UpDisabledHovered"
            when: !upIndicator.enabled
                  && spinBox.up.hovered
                  && !spinBox.up.pressed
                  && !spinBox.activeFocus

            PropertyChanges {
                target: background
                border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
        },
        State {
            name: "UpHovered"
            when: upIndicator.enabled
                  && spinBox.up.hovered
                  && !spinBox.up.pressed
                  && !spinBox.activeFocus

            PropertyChanges {
                target: background
                border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
            PropertyChanges {
                target: upIndicatorText
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1)
            }
        },
        State {
            name: "UpPressed"
            when: upIndicator.enabled
                  && spinBox.up.pressed
                  && !spinBox.activeFocus

            PropertyChanges {
                target: background
                border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
            PropertyChanges {
                target: upPressIndicator
                radius: spinBox.pressIndicatorRadius()
                opacity: 1.0
            }
            PropertyChanges {
                target: upIndicatorText
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.3)
            }
        },
        State {
            name: "Focused"
            when: spinBox.enabled
                  && !spinBox.down.hovered
                  && !spinBox.up.hovered
                  && !spinBox.down.pressed
                  && !spinBox.up.pressed
                  && spinBox.activeFocus

            PropertyChanges {
                target: spinBoxText
                color: ThemeSettings.colorPrimary
            }
            PropertyChanges {
                target: background
                border.color: ThemeSettings.colorPrimary
                border.width: Ak.newUnit(2 * ThemeSettings.controlScale,
                                         "dp").pixels
            }
        },
        State {
            name: "DownDisabledFocused"
            when: !upIndicator.enabled
                  && spinBox.down.hovered
                  && !spinBox.down.pressed
                  && spinBox.activeFocus

            PropertyChanges {
                target: spinBoxText
                color: ThemeSettings.colorPrimary
            }
            PropertyChanges {
                target: background
                border.color: ThemeSettings.colorPrimary
                border.width: Ak.newUnit(2 * ThemeSettings.controlScale,
                                         "dp").pixels
            }
        },
        State {
            name: "DownFocused"
            when: upIndicator.enabled
                  && spinBox.down.hovered
                  && !spinBox.down.pressed
                  && spinBox.activeFocus

            PropertyChanges {
                target: spinBoxText
                color: ThemeSettings.colorPrimary
            }
            PropertyChanges {
                target: background
                border.color: ThemeSettings.colorPrimary
                border.width: Ak.newUnit(2 * ThemeSettings.controlScale,
                                         "dp").pixels
            }
            PropertyChanges {
                target: downIndicatorText
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1)
            }
        },
        State {
            name: "DownFocusedPressed"
            when: upIndicator.enabled
                  && spinBox.down.pressed
                  && spinBox.activeFocus

            PropertyChanges {
                target: spinBoxText
                color: ThemeSettings.colorPrimary
            }
            PropertyChanges {
                target: background
                border.color: ThemeSettings.colorPrimary
                border.width: Ak.newUnit(2 * ThemeSettings.controlScale,
                                         "dp").pixels
            }
            PropertyChanges {
                target: downPressIndicator
                radius: spinBox.pressIndicatorRadius()
                opacity: 1.0
            }
            PropertyChanges {
                target: downIndicatorText
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.3)
            }
        },
        State {
            name: "UpDisabledFocused"
            when: !upIndicator.enabled
                  && spinBox.up.hovered
                  && !spinBox.up.pressed
                  && spinBox.activeFocus

            PropertyChanges {
                target: spinBoxText
                color: ThemeSettings.colorPrimary
            }
            PropertyChanges {
                target: background
                border.color: ThemeSettings.colorPrimary
                border.width: Ak.newUnit(2 * ThemeSettings.controlScale,
                                         "dp").pixels
            }
        },
        State {
            name: "UpFocused"
            when: upIndicator.enabled
                  && spinBox.up.hovered
                  && !spinBox.up.pressed
                  && spinBox.activeFocus

            PropertyChanges {
                target: spinBoxText
                color: ThemeSettings.colorPrimary
            }
            PropertyChanges {
                target: background
                border.color: ThemeSettings.colorPrimary
                border.width: Ak.newUnit(2 * ThemeSettings.controlScale,
                                         "dp").pixels
            }
            PropertyChanges {
                target: upIndicatorText
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1)
            }
        },
        State {
            name: "UpFocusedPressed"
            when: upIndicator.enabled
                  && spinBox.up.pressed
                  && spinBox.activeFocus

            PropertyChanges {
                target: spinBoxText
                color: ThemeSettings.colorPrimary
            }
            PropertyChanges {
                target: background
                border.color: ThemeSettings.colorPrimary
                border.width: Ak.newUnit(2 * ThemeSettings.controlScale,
                                         "dp").pixels
            }
            PropertyChanges {
                target: upPressIndicator
                radius: spinBox.pressIndicatorRadius()
                opacity: 1.0
            }
            PropertyChanges {
                target: upIndicatorText
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.3)
            }
        }
    ]

    transitions: Transition {
        ColorAnimation {
            target: spinBoxText
            duration: spinBox.animationTime
        }
        PropertyAnimation {
            target: background
            properties: "border"
            duration: spinBox.animationTime
        }
        ColorAnimation {
            target: downIndicatorText
            duration: spinBox.animationTime
        }
        ColorAnimation {
            target: upIndicatorText
            duration: spinBox.animationTime
        }
        PropertyAnimation {
            target: downPressIndicator
            properties: "radius,opacity"
            duration: spinBox.animationTime
        }
        PropertyAnimation {
            target: upPressIndicator
            properties: "radius,opacity"
            duration: spinBox.animationTime
        }
    }
}
