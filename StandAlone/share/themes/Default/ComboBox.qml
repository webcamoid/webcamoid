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

T.ComboBox {
    id: control
    implicitWidth:
        Math.max(implicitBackgroundWidth + leftInset + rightInset,
                 implicitContentWidth + leftPadding + rightPadding,
                 AkUnit.create(96 * AkTheme.controlScale, "dp").pixels)
    implicitHeight:
        Math.max(implicitBackgroundHeight + topInset + bottomInset,
                 implicitContentHeight + topPadding + bottomPadding,
                 implicitIndicatorHeight + topPadding + bottomPadding,
                 AkUnit.create(36 * AkTheme.controlScale, "dp").pixels)
    padding: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
    leftPadding: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    rightPadding: AkUnit.create(40 * AkTheme.controlScale, "dp").pixels
    hoverEnabled: true
    clip: true

    readonly property int radius:
        AkUnit.create(6 * AkTheme.controlScale, "dp").pixels
    readonly property int animationTime: 200

    // Writeable text
    contentItem: T.TextField {
        id: textField
        text: control.editable?
                  control.editText:
                  control.displayText
        enabled: control.editable
        autoScroll: control.editable
        readOnly: control.down
        inputMethodHints: control.inputMethodHints
        validator: control.validator
        font: control.font
        color: control.editable?
                   AkTheme.palette.active.text:
               control.flat?
                   AkTheme.palette.active.highlight:
                   AkTheme.palette.active.buttonText
        selectionColor: AkTheme.palette.active.highlight
        selectedTextColor: AkTheme.palette.active.highlightedText
        verticalAlignment: Text.AlignVCenter
        selectByMouse: true
    }

    // v
    indicator: Item {
        id: indicator
        x: control.mirrored?
               control.padding:
               control.width
               - width
               - control.padding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: AkUnit.create(24 * AkTheme.controlScale, "dp").pixels
        height: width

        AkColorizedImage {
            id: indicatorUpImage
            width: indicator.width / 2
            height: indicator.height / 2
            anchors.verticalCenter: indicator.verticalCenter
            anchors.horizontalCenter: indicator.horizontalCenter
            source: control.down? "image://icons/up": "image://icons/down"
            color: control.editable?
                       AkTheme.palette.active.text:
                   control.flat?
                       AkTheme.palette.active.highlight:
                       AkTheme.palette.active.buttonText
            asynchronous: true
            mipmap: true
        }
    }

    // Background
    background: Rectangle {
        id: comboBoxBackground
        color:
            control.editable?
                AkTheme.palette.active.base:
            control.flat?
                AkTheme.shade(AkTheme.palette.active.window, 0, 0):
                AkTheme.palette.active.button
        border.color:
            control.flat?
                AkTheme.shade(AkTheme.palette.active.window, 0, 0):
            control.editable?
                AkTheme.palette.active.mid:
                AkTheme.palette.active.dark
        border.width: AkUnit.create(1 * AkTheme.controlScale,
                                 "dp").pixels
        radius: control.flat? 0: control.radius
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0
                color: AkTheme.palette.active.window.hslLightness < 0.5?
                           Qt.tint(comboBoxBackground.color,
                                   AkTheme.shade(AkTheme.palette.active.dark, 0, 0.25)):
                           Qt.tint(comboBoxBackground.color,
                                   AkTheme.shade(AkTheme.palette.active.light, 0, 0.25))
            }
            GradientStop {
                position: 1
                color: AkTheme.palette.active.window.hslLightness < 0.5?
                           Qt.tint(comboBoxBackground.color,
                                   AkTheme.shade(AkTheme.palette.active.light, 0, 0.25)):
                           Qt.tint(comboBoxBackground.color,
                                   AkTheme.shade(AkTheme.palette.active.dark, 0, 0.25))
            }
        }
    }

    // List of elements
    popup: T.Popup {
        id: popup
        y: control.height
           + AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
        width: control.width
        implicitHeight: contentItem.implicitHeight + 2 * topPadding
        transformOrigin: Item.Top
        topPadding: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
        bottomPadding: topPadding

        // Fade in
        enter: Transition {
            NumberAnimation {
                property: "scale"
                from: 0.9
                to: 1.0
                easing.type: Easing.OutQuint
                duration: 220
            }
            NumberAnimation {
                property: "opacity"
                from: 0.0
                to: 1.0
                easing.type: Easing.OutCubic
                duration: 150
            }
        }

        // Fade out
        exit: Transition {
            NumberAnimation {
                property: "scale"
                from: 1.0
                to: 0.9
                easing.type: Easing.OutQuint
                duration: 220
            }
            NumberAnimation {
                property: "opacity"
                from: 1.0
                to: 0.0
                easing.type: Easing.OutCubic
                duration: 150
            }
        }

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.delegateModel
            currentIndex: control.highlightedIndex
            cacheBuffer: 1

            T.ScrollIndicator.vertical: ScrollIndicator {
            }
        }

        background: Rectangle {
            id: popupBackground
            color: AkTheme.palette.active.window
            border.color: AkTheme.palette.active.dark
            radius: control.radius
            anchors.fill: parent
        }
    }

    // Element
    delegate: MenuItem {
        width: parent.width
        text: control.textRole?
                  (Array.isArray(control.model)?
                       modelData[control.textRole]:
                       model[control.textRole]):
                  modelData
        highlighted: control.highlightedIndex === index
        hoverEnabled: control.hoverEnabled
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: textField
                color: control.editable?
                           AkTheme.palette.disabled.text:
                       control.flat?
                           AkTheme.palette.disabled.highlight:
                           AkTheme.palette.disabled.buttonText
            }
            PropertyChanges {
                target: indicatorUpImage
                color: control.editable?
                           AkTheme.palette.disabled.text:
                       control.flat?
                           AkTheme.palette.disabled.highlight:
                           AkTheme.palette.disabled.buttonText
            }
            PropertyChanges {
                target: comboBoxBackground
                color:
                    control.editable?
                        AkTheme.palette.disabled.base:
                    control.flat?
                        AkTheme.shade(AkTheme.palette.disabled.window, 0, 0):
                        AkTheme.palette.disabled.button
                border.color:
                    control.flat?
                        AkTheme.shade(AkTheme.palette.disabled.window, 0, 0):
                    control.editable?
                        AkTheme.palette.disabled.mid:
                        AkTheme.palette.disabled.dark
            }
        },
        State {
            name: "Hover"
            when: control.enabled
                  && control.hovered
                  && !(control.activeFocus || control.visualFocus)
                  && !control.pressed

            PropertyChanges {
                target: comboBoxBackground
                border.color:
                    control.flat?
                        AkTheme.shade(AkTheme.palette.active.window, 0, 0):
                        AkTheme.palette.active.dark
                color: control.editable?
                           AkTheme.palette.active.base:
                       control.flat?
                           AkTheme.shade(AkTheme.palette.active.mid, 0, 0.5):
                           AkTheme.palette.active.mid
            }
        },
        State {
            name: "Focused"
            when: control.enabled
                  && (control.activeFocus || control.visualFocus)
                  && !control.pressed

            PropertyChanges {
                target: comboBoxBackground
                border.width: AkUnit.create(2 * AkTheme.controlScale,
                                                        "dp").pixels
                border.color:
                    control.flat?
                        AkTheme.shade(AkTheme.palette.active.window, 0, 0):
                        AkTheme.palette.active.highlight
                color: control.editable?
                           AkTheme.palette.active.base:
                       control.flat?
                           AkTheme.shade(AkTheme.palette.active.mid, 0, 0.5):
                           AkTheme.palette.active.mid
            }
        },
        State {
            name: "Pressed"
            when: control.enabled
                  && control.pressed

            PropertyChanges {
                target: comboBoxBackground
                border.width:
                    AkUnit.create(2 * AkTheme.controlScale, "dp").pixels
                border.color:
                    control.flat?
                        AkTheme.shade(AkTheme.palette.active.window, 0, 0):
                        AkTheme.palette.active.highlight
                color: control.editable?
                           AkTheme.palette.active.base:
                       control.flat?
                           AkTheme.shade(AkTheme.palette.active.dark, 0, 0.5):
                           AkTheme.palette.active.dark
            }
        }
    ]

    transitions: Transition {
        ColorAnimation {
            target: textField
            duration: control.animationTime
        }
        PropertyAnimation {
            target: comboBoxBackground
            properties: "border.color,border.width,color"
            duration: control.animationTime
        }
        PropertyAnimation {
            target: indicatorUpImage
            properties: "color"
            duration: control.animationTime
        }
    }
}
