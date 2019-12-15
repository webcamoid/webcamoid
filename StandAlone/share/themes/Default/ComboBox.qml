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
import QtQuick.Templates 2.5 as T
import QtGraphicalEffects 1.0
import QtQuick.Controls.impl 2.12
import AkQml 1.0

T.ComboBox {
    id: comboBox
    implicitWidth:
        Math.max(implicitBackgroundWidth + leftInset + rightInset,
                 implicitContentWidth + leftPadding + rightPadding,
                 Ak.newUnit(96 * ThemeSettings.constrolScale, "dp").pixels)
    implicitHeight:
        Math.max(implicitBackgroundHeight + topInset + bottomInset,
                 implicitContentHeight + topPadding + bottomPadding,
                 implicitIndicatorHeight + topPadding + bottomPadding,
                 Ak.newUnit(36 * ThemeSettings.constrolScale, "dp").pixels)
    padding: Ak.newUnit(8 * ThemeSettings.constrolScale, "dp").pixels
    leftPadding: Ak.newUnit(16 * ThemeSettings.constrolScale, "dp").pixels
    rightPadding: Ak.newUnit(40 * ThemeSettings.constrolScale, "dp").pixels
    hoverEnabled: true
    clip: true

    readonly property int radius:
        Ak.newUnit(6 * ThemeSettings.constrolScale, "dp").pixels
    readonly property int animationTime: 200

    function pressIndicatorRadius()
    {
        let diffX = comboBox.width / 2
        let diffY = comboBox.height / 2
        let r2 = diffX * diffX + diffY * diffY

        return Math.sqrt(r2)
    }

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
        selectionColor: ThemeSettings.colorSecondary
        selectedTextColor: ThemeSettings.colorText
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
        width: Ak.newUnit(24 * ThemeSettings.constrolScale, "dp").pixels
        height: width

        Item {
            id: indicatorUp
            width: indicator.width / 2
            height: indicator.height / 2
            anchors.verticalCenter: indicator.verticalCenter
            anchors.horizontalCenter: indicator.horizontalCenter

            Image {
                id: indicatorUpImage
                asynchronous: true
                cache: true
                source: comboBox.down? "image://icons/up": "image://icons/down"
                sourceSize: Qt.size(width, height)
                visible: false
                anchors.fill: indicatorUp
            }
            ColorOverlay {
                id: indicatorUpOverlay
                anchors.fill: indicatorUpImage
                source: indicatorUpImage
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            }
        }
    }

    // Background
    background: Item {
        anchors.fill: parent

        // Rectagle below the indicator
        Rectangle {
            id: comboBoxRectangleBelow
            anchors.fill: parent
            color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            visible: false
        }

        // Press indicator
        Rectangle{
            id: comboBoxPressIndicatorMask
            anchors.fill: parent
            radius: comboBox.flat? 0: comboBox.radius
            color: Qt.hsla(0, 0, 0, 1)
            visible: false
        }
        Item {
            id: comboBoxPressIndicatorItem
            anchors.fill: comboBoxPressIndicatorMask
            clip: true
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: comboBoxPressIndicatorMask
            }

            Rectangle {
                id: comboBoxPress
                radius: 0
                anchors.verticalCenter: comboBoxPressIndicatorItem.verticalCenter
                anchors.horizontalCenter: comboBoxPressIndicatorItem.horizontalCenter
                width: 2 * radius
                height: 2 * radius
                color: ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.1,
                                                0.3)
                opacity: 0
            }
        }

        // Rectangle
        Rectangle {
            id: comboBoxBackground
            color: Qt.hsla(0, 0, 0, 0)
            border.color: comboBox.flat?
                              Qt.hsla(0, 0, 0, 0):
                              ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            border.width: Ak.newUnit(1 * ThemeSettings.constrolScale,
                                     "dp").pixels
            radius: comboBox.flat? 0: comboBox.radius
            anchors.fill: parent
        }
    }

    // List of elements
    popup: T.Popup {
        id: popup
        y: comboBox.height
           + Ak.newUnit(4 * ThemeSettings.constrolScale, "dp").pixels
        width: comboBox.width
        implicitHeight: contentItem.implicitHeight
        transformOrigin: Item.Top
        topMargin: Ak.newUnit(8 * ThemeSettings.constrolScale, "dp").pixels
        bottomMargin: topMargin

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
            layer.enabled: true
            layer.effect: OpacityMask {
                cached: true
                maskSource: Rectangle {
                    width: popup.width
                    height: popup.height
                    radius: comboBox.radius
                    visible: false
                }

            }
            T.ScrollIndicator.vertical: ScrollIndicator {
            }
        }

        background: Rectangle {
            id: popupBackground
            color: ThemeSettings.colorBack
            border.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            radius: comboBox.radius
            anchors.fill: parent
            layer.enabled: true
            layer.effect: DropShadow {
                cached: true
                horizontalOffset: popupBackground.radius / 2
                verticalOffset: popupBackground.radius / 2
                radius: comboBox.radius
                samples: 2 * radius + 1
                color: ThemeSettings.constShade(ThemeSettings.colorBack, -0.9)
            }
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
                target: indicatorUpOverlay
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
                target: indicatorUpOverlay
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
            PropertyChanges {
                target: comboBoxBackground
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
                target: indicatorUpOverlay
                color: ThemeSettings.colorPrimary
            }
            PropertyChanges {
                target: comboBoxBackground
                border.color: ThemeSettings.colorPrimary
                border.width: Ak.newUnit(2 * ThemeSettings.constrolScale,
                                                        "dp").pixels
            }
        },
        State {
            name: "Pressed"
            when: comboBox.enabled
                  && !comboBox.flat
                  && comboBox.pressed

            PropertyChanges {
                target: indicatorUpOverlay
                color:
                    ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.3)
            }
            PropertyChanges {
                target: comboBoxBackground
                border.color:
                    ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.3)
                border.width:
                    Ak.newUnit(2 * ThemeSettings.constrolScale, "dp").pixels
            }
            PropertyChanges {
                target: comboBoxPress
                radius: comboBox.pressIndicatorRadius()
                opacity: 1
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
                target: indicatorUpOverlay
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
                target: indicatorUpOverlay
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
            PropertyChanges {
                target: comboBoxRectangleBelow
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
                visible: true
            }
        },
        State {
            name: "FlatFocused"
            when: comboBox.enabled
                  && comboBox.flat
                  && comboBox.activeFocus
                  && !comboBox.pressed

            PropertyChanges {
                target: indicatorUpOverlay
                color: ThemeSettings.colorPrimary
            }
            PropertyChanges {
                target: comboBoxRectangleBelow
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
                visible: true
            }
        },
        State {
            name: "FlatPressed"
            when: comboBox.enabled
                  && comboBox.flat
                  && comboBox.pressed

            PropertyChanges {
                target: indicatorUpOverlay
                color:
                    ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.3)
            }
            PropertyChanges {
                target: comboBoxPress
                radius: comboBox.pressIndicatorRadius()
                opacity: 1
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
            properties: "border"
            duration: comboBox.animationTime
        }
        PropertyAnimation {
            target: comboBoxPress
            properties: "opacity,radius"
            duration: comboBox.animationTime
        }
        ColorAnimation {
            target: comboBoxRectangleBelow
            duration: comboBox.animationTime
        }
        ColorAnimation {
            target: indicatorUpOverlay
            duration: comboBox.animationTime
        }
    }
}
