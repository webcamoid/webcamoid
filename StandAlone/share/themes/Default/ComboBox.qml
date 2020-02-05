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
                 AkUnit.create(96 * ThemeSettings.controlScale, "dp").pixels)
    implicitHeight:
        Math.max(implicitBackgroundHeight + topInset + bottomInset,
                 implicitContentHeight + topPadding + bottomPadding,
                 implicitIndicatorHeight + topPadding + bottomPadding,
                 AkUnit.create(36 * ThemeSettings.controlScale, "dp").pixels)
    padding: AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
    leftPadding: AkUnit.create(16 * ThemeSettings.controlScale, "dp").pixels
    rightPadding: AkUnit.create(40 * ThemeSettings.controlScale, "dp").pixels
    hoverEnabled: true
    clip: true

    readonly property int radius:
        AkUnit.create(6 * ThemeSettings.controlScale, "dp").pixels
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
                   ThemeSettings.colorActiveText:
               control.flat?
                   ThemeSettings.colorActiveHighlight:
                   ThemeSettings.colorActiveButtonText
        selectionColor: ThemeSettings.colorActiveHighlight
        selectedTextColor: ThemeSettings.colorActiveHighlightedText
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
        width: AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels
        height: width

        AkColorizedImage {
            id: indicatorUpImage
            width: indicator.width / 2
            height: indicator.height / 2
            anchors.verticalCenter: indicator.verticalCenter
            anchors.horizontalCenter: indicator.horizontalCenter
            source: control.down? "image://icons/up": "image://icons/down"
            color: control.editable?
                       ThemeSettings.colorActiveText:
                   control.flat?
                       ThemeSettings.colorActiveHighlight:
                       ThemeSettings.colorActiveButtonText
            asynchronous: true
        }
    }

    // Background
    background: Rectangle {
        id: comboBoxBackground
        color:
            control.editable?
                ThemeSettings.colorActiveBase:
            control.flat?
                ThemeSettings.shade(ThemeSettings.colorActiveWindow, 0, 0):
                ThemeSettings.colorActiveButton
        border.color:
            control.flat?
                ThemeSettings.shade(ThemeSettings.colorActiveWindow, 0, 0):
            control.editable?
                ThemeSettings.colorActiveMid:
                ThemeSettings.colorActiveDark
        border.width: AkUnit.create(1 * ThemeSettings.controlScale,
                                 "dp").pixels
        radius: control.flat? 0: control.radius
        anchors.fill: parent
    }

    // List of elements
    popup: T.Popup {
        id: popup
        y: control.height
           + AkUnit.create(4 * ThemeSettings.controlScale, "dp").pixels
        width: control.width
        implicitHeight: contentItem.implicitHeight + 2 * topPadding
        transformOrigin: Item.Top
        topPadding: AkUnit.create(8 * ThemeSettings.controlScale, "dp").pixels
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
            color: ThemeSettings.colorActiveWindow
            border.color: ThemeSettings.colorActiveDark
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
                           ThemeSettings.colorDisabledText:
                       control.flat?
                           ThemeSettings.colorDisabledHighlight:
                           ThemeSettings.colorDisabledButtonText
            }
            PropertyChanges {
                target: indicatorUpImage
                color: control.editable?
                           ThemeSettings.colorDisabledText:
                       control.flat?
                           ThemeSettings.colorDisabledHighlight:
                           ThemeSettings.colorDisabledButtonText
            }
            PropertyChanges {
                target: comboBoxBackground
                color:
                    control.editable?
                        ThemeSettings.colorDisabledBase:
                    control.flat?
                        ThemeSettings.shade(ThemeSettings.colorDisabledWindow, 0, 0):
                        ThemeSettings.colorDisabledButton
                border.color:
                    control.flat?
                        ThemeSettings.shade(ThemeSettings.colorDisabledWindow, 0, 0):
                    control.editable?
                        ThemeSettings.colorDisabledMid:
                        ThemeSettings.colorDisabledDark
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
                        ThemeSettings.shade(ThemeSettings.colorActiveWindow, 0, 0):
                        ThemeSettings.colorActiveDark
                color: control.editable?
                           ThemeSettings.colorActiveBase:
                       control.flat?
                           ThemeSettings.shade(ThemeSettings.colorActiveMid, 0, 0.5):
                           ThemeSettings.colorActiveMid
            }
        },
        State {
            name: "Focused"
            when: control.enabled
                  && (control.activeFocus || control.visualFocus)
                  && !control.pressed

            PropertyChanges {
                target: comboBoxBackground
                border.width: AkUnit.create(2 * ThemeSettings.controlScale,
                                                        "dp").pixels
                border.color:
                    control.flat?
                        ThemeSettings.shade(ThemeSettings.colorActiveWindow, 0, 0):
                        ThemeSettings.colorActiveHighlight
                color: control.editable?
                           ThemeSettings.colorActiveBase:
                       control.flat?
                           ThemeSettings.shade(ThemeSettings.colorActiveMid, 0, 0.5):
                           ThemeSettings.colorActiveMid
            }
        },
        State {
            name: "Pressed"
            when: control.enabled
                  && control.pressed

            PropertyChanges {
                target: comboBoxBackground
                border.width:
                    AkUnit.create(2 * ThemeSettings.controlScale, "dp").pixels
                border.color:
                    control.flat?
                        ThemeSettings.shade(ThemeSettings.colorActiveWindow, 0, 0):
                        ThemeSettings.colorActiveHighlight
                color: control.editable?
                           ThemeSettings.colorActiveBase:
                       control.flat?
                           ThemeSettings.shade(ThemeSettings.colorActiveDark, 0, 0.5):
                           ThemeSettings.colorActiveDark
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
