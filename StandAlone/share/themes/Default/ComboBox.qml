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
    id: comboBox
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
        text: comboBox.editable?
                  comboBox.editText:
                  comboBox.displayText
        enabled: comboBox.editable
        autoScroll: comboBox.editable
        readOnly: comboBox.down
        inputMethodHints: comboBox.inputMethodHints
        validator: comboBox.validator
        font: comboBox.font
        color: ThemeSettings.colorText
        selectionColor: ThemeSettings.colorHighlight
        selectedTextColor: ThemeSettings.colorHighlightedText
        verticalAlignment: Text.AlignVCenter
        selectByMouse: true
    }

    // v
    indicator: Item {
        id: indicator
        x: comboBox.mirrored?
               comboBox.padding:
               comboBox.width
               - width
               - comboBox.padding
        y: comboBox.topPadding + (comboBox.availableHeight - height) / 2
        width: AkUnit.create(24 * ThemeSettings.controlScale, "dp").pixels
        height: width

        AkColorizedImage {
            id: indicatorUpImage
            width: indicator.width / 2
            height: indicator.height / 2
            anchors.verticalCenter: indicator.verticalCenter
            anchors.horizontalCenter: indicator.horizontalCenter
            source: comboBox.down? "image://icons/up": "image://icons/down"
            color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            asynchronous: true
        }
    }

    // Background
    background: Rectangle {
        id: comboBoxBackground
        color: ThemeSettings.shade(ThemeSettings.colorBack, 0.0, 0.0)
        border.color: comboBox.flat?
                          Qt.hsla(0, 0, 0, 0):
                          ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
        border.width: AkUnit.create(1 * ThemeSettings.controlScale,
                                 "dp").pixels
        radius: comboBox.flat? 0: comboBox.radius
        anchors.fill: parent
    }

    // List of elements
    popup: T.Popup {
        id: popup
        y: comboBox.height
           + AkUnit.create(4 * ThemeSettings.controlScale, "dp").pixels
        width: comboBox.width
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
            model: comboBox.delegateModel
            currentIndex: comboBox.highlightedIndex
            cacheBuffer: 1

            T.ScrollIndicator.vertical: ScrollIndicator {
            }
        }

        background: Rectangle {
            id: popupBackground
            color: ThemeSettings.colorBack
            border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            radius: comboBox.radius
            anchors.fill: parent
        }
    }

    // Element
    delegate: MenuItem {
        width: parent.width
        text: comboBox.textRole?
                  (Array.isArray(comboBox.model)?
                       modelData[comboBox.textRole]:
                       model[comboBox.textRole]):
                  modelData
        highlighted: comboBox.highlightedIndex === index
        hoverEnabled: comboBox.hoverEnabled
    }

    states: [
        State {
            name: "Disabled"
            when: !comboBox.enabled
                  && !comboBox.flat

            PropertyChanges {
                target: textField
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
            }
            PropertyChanges {
                target: indicatorUpImage
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
            }
            PropertyChanges {
                target: comboBoxBackground
                border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
            }
        },
        State {
            name: "Hover"
            when: comboBox.enabled
                  && !comboBox.flat
                  && comboBox.hovered
                  && !comboBox.activeFocus
                  && !comboBox.pressed

            PropertyChanges {
                target: indicatorUpImage
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
            PropertyChanges {
                target: comboBoxBackground
                color:
                    ThemeSettings.constShade(ThemeSettings.colorHighlight,
                                             0.1,
                                             0.2)
                border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
        },
        State {
            name: "Focused"
            when: comboBox.enabled
                  && !comboBox.flat
                  && comboBox.activeFocus
                  && !comboBox.pressed

            PropertyChanges {
                target: indicatorUpImage
                color: ThemeSettings.colorHighlight
            }
            PropertyChanges {
                target: comboBoxBackground
                color:
                    ThemeSettings.constShade(ThemeSettings.colorHighlight,
                                             0.1,
                                             0.3)
                border.color: ThemeSettings.colorHighlight
                border.width: AkUnit.create(2 * ThemeSettings.controlScale,
                                                        "dp").pixels
            }
        },
        State {
            name: "Pressed"
            when: comboBox.enabled
                  && !comboBox.flat
                  && comboBox.pressed

            PropertyChanges {
                target: indicatorUpImage
                color:
                    ThemeSettings.constShade(ThemeSettings.colorHighlight, 0.3)
            }
            PropertyChanges {
                target: comboBoxBackground
                color:
                    ThemeSettings.constShade(ThemeSettings.colorHighlight,
                                             0.1,
                                             0.4)
                border.color:
                    ThemeSettings.constShade(ThemeSettings.colorHighlight, 0.3)
                border.width:
                    AkUnit.create(2 * ThemeSettings.controlScale, "dp").pixels
            }
        },
        State {
            name: "FlatDisabled"
            when: !comboBox.enabled
                  && comboBox.flat

            PropertyChanges {
                target: textField
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
            }
            PropertyChanges {
                target: indicatorUpImage
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
            }
        },
        State {
            name: "FlatHover"
            when: comboBox.enabled
                  && comboBox.flat
                  && comboBox.hovered
                  && !comboBox.activeFocus
                  && !comboBox.pressed

            PropertyChanges {
                target: indicatorUpImage
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
            PropertyChanges {
                target: comboBoxBackground
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
        },
        State {
            name: "FlatFocused"
            when: comboBox.enabled
                  && comboBox.flat
                  && comboBox.activeFocus
                  && !comboBox.pressed

            PropertyChanges {
                target: indicatorUpImage
                color: ThemeSettings.colorHighlight
            }
            PropertyChanges {
                target: comboBoxBackground
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
            }
        },
        State {
            name: "FlatPressed"
            when: comboBox.enabled
                  && comboBox.flat
                  && comboBox.pressed

            PropertyChanges {
                target: comboBoxBackground
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            }
        }
    ]

    transitions: Transition {
        ColorAnimation {
            target: textField
            duration: comboBox.animationTime
        }
        PropertyAnimation {
            target: comboBoxBackground
            properties: "border.color,border.width,color"
            duration: comboBox.animationTime
        }
        PropertyAnimation {
            target: indicatorUpImage
            properties: "color"
            duration: comboBox.animationTime
        }
    }
}
