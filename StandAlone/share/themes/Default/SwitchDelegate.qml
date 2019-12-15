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

T.SwitchDelegate {
    id: control
    icon.width: Ak.newUnit(18 * ThemeSettings.constrolScale, "dp").pixels
    icon.height: Ak.newUnit(18 * ThemeSettings.constrolScale, "dp").pixels
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + implicitIndicatorWidth
                            + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)
    padding: Ak.newUnit(4 * ThemeSettings.constrolScale, "dp").pixels
    spacing: Ak.newUnit(8 * ThemeSettings.constrolScale, "dp").pixels
    hoverEnabled: true
    clip: true

    readonly property int animationTime: 100

    function pressIndicatorRadius()
    {
        let diffX = control.width / 2
        let diffY = control.height / 2
        let r2 = diffX * diffX + diffY * diffY

        return Math.sqrt(r2)
    }

    indicator: Item {
        id: sliderIndicator
        anchors.right: control.right
        anchors.rightMargin: control.rightPadding
        anchors.verticalCenter: control.verticalCenter
        implicitWidth:
            Ak.newUnit(36 * ThemeSettings.constrolScale, "dp").pixels
        implicitHeight:
            Ak.newUnit(20 * ThemeSettings.constrolScale, "dp").pixels

        Rectangle {
            id: switchTrack
            height: parent.height / 2
            color: control.highlighted?
                       ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                       ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            radius: height / 2
            anchors.verticalCenter: sliderIndicator.verticalCenter
            anchors.right: sliderIndicator.right
            anchors.left: sliderIndicator.left
        }
        Item {
            id: switchThumb
            width: height
            anchors.bottom: sliderIndicator.bottom
            anchors.top: sliderIndicator.top

            Rectangle {
                id: shadowRect
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
                radius: height / 2
                anchors.fill: parent
                visible: false
            }
            DropShadow {
                anchors.fill: parent
                cached: true
                horizontalOffset: radius / 2
                verticalOffset: radius / 2
                radius: Ak.newUnit(1 * ThemeSettings.constrolScale, "dp").pixels
                samples: 2 * radius + 1
                color: ThemeSettings.constShade(ThemeSettings.colorBack, -0.9)
                source: shadowRect
                visible: !control.highlighted
            }
            Rectangle {
                id: switchThumbRect
                color: control.highlighted?
                           ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                           ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
                radius: height / 2
                anchors.fill: parent
            }
        }
    }

    contentItem: IconLabel {
        id: iconLabel
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        icon.name: control.icon.name
        icon.source: control.icon.source
        icon.width: control.icon.width
        icon.height: control.icon.height
        icon.color: ThemeSettings.colorText
        text: control.text
        font: control.font
        color: control.highlighted?
                   ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                   ThemeSettings.colorText
        alignment: Qt.AlignLeft
        anchors.leftMargin: control.leftPadding
        anchors.left: control.left
        anchors.right: sliderIndicator.left
    }

    background: Item {
        id: backgroundItem
        implicitWidth:
            Ak.newUnit(128 * ThemeSettings.constrolScale, "dp").pixels
        implicitHeight:
            Ak.newUnit(48 * ThemeSettings.constrolScale, "dp").pixels

        // Press indicator
        Rectangle{
            id: controlPressIndicatorMask
            anchors.fill: parent
            color: Qt.hsla(0, 0, 0, 1)
            visible: false
        }
        Rectangle {
            id: controlPress
            radius: 0
            anchors.verticalCenter: backgroundItem.verticalCenter
            anchors.horizontalCenter: backgroundItem.horizontalCenter
            width: 2 * radius
            height: 2 * radius
            color: control.highlighted?
                       ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                0.3,
                                                0.75):
                       ThemeSettings.shade(ThemeSettings.colorBack, -0.5)
            opacity: 0
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: controlPressIndicatorMask
            }
        }

        Rectangle {
            id: background
            color: control.highlighted?
                       ThemeSettings.colorPrimary:
                       ThemeSettings.shade(ThemeSettings.colorBack, -0.1, 0)
            anchors.fill: parent
        }
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: switchTrack
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
            PropertyChanges {
                target: switchThumbRect
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
            PropertyChanges {
                target: iconLabel
                icon.color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
                color: ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
            PropertyChanges {
                target: background
                color: "transparent"
            }
        },
        State {
            name: "Checked"
            when: control.checked
                  && !(control.hovered
                       || control.visualFocus
                       || control.activeFocus)
                  && !control.pressed

            PropertyChanges {
                target: switchTrack
                color: control.highlighted?
                           ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                           ThemeSettings.colorPrimary
            }
            PropertyChanges {
                target: switchThumbRect
                color: control.highlighted?
                           ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                           ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.2)
            }
            PropertyChanges {
                target: switchThumb
                x: sliderIndicator.width - switchThumb.width
            }
        },
        State {
            name: "Hovered"
            when: !control.checked
                  && (control.hovered
                      || control.visualFocus
                      || control.activeFocus)
                  && !control.pressed

            PropertyChanges {
                target: switchTrack
                color: control.highlighted?
                           ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                           ThemeSettings.shade(ThemeSettings.colorBack, -0.6)
            }
            PropertyChanges {
                target: switchThumbRect
                color: control.highlighted?
                           ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                           ThemeSettings.shade(ThemeSettings.colorBack, -0.2)
            }
            PropertyChanges {
                target: background
                color:
                    control.highlighted?
                        ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                 0.1):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
        },
        State {
            name: "CheckedHovered"
            when: control.checked
                  && (control.hovered
                      || control.visualFocus
                      || control.activeFocus)
                  && !control.pressed

            PropertyChanges {
                target: switchTrack
                color: control.highlighted?
                           ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                           ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.1)
            }
            PropertyChanges {
                target: switchThumbRect
                color: control.highlighted?
                           ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                           ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.3)
            }
            PropertyChanges {
                target: switchThumb
                x: sliderIndicator.width - switchThumb.width
            }
            PropertyChanges {
                target: background
                color:
                    control.highlighted?
                        ThemeSettings.constShade(ThemeSettings.colorPrimary,
                                                 0.1):
                        ThemeSettings.shade(ThemeSettings.colorBack, -0.1)
            }
        },
        State {
            name: "Pressed"
            when: !control.checked
                  && control.pressed

            PropertyChanges {
                target: switchTrack
                color: control.highlighted?
                           ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                           ThemeSettings.shade(ThemeSettings.colorBack, -0.7)
            }
            PropertyChanges {
                target: switchThumbRect
                color: control.highlighted?
                           ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                           ThemeSettings.shade(ThemeSettings.colorBack, -0.3)
            }
            PropertyChanges {
                target: controlPress
                radius: control.pressIndicatorRadius()
                opacity: 1
            }
            PropertyChanges {
                target: background
                visible: false
            }
        },
        State {
            name: "CheckedPressed"
            when: control.checked
                  && control.pressed

            PropertyChanges {
                target: switchTrack
                color: control.highlighted?
                           ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                           ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.2)
            }
            PropertyChanges {
                target: switchThumbRect
                color: control.highlighted?
                           ThemeSettings.contrast(ThemeSettings.colorPrimary, 0.75):
                           ThemeSettings.constShade(ThemeSettings.colorPrimary, 0.4)
            }
            PropertyChanges {
                target: switchThumb
                x: sliderIndicator.width - switchThumb.width
            }
            PropertyChanges {
                target: controlPress
                radius: control.pressIndicatorRadius()
                opacity: 1
            }
            PropertyChanges {
                target: background
                visible: false
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: switchTrack
            properties: "color"
            duration: control.animationTime
        }
        PropertyAnimation {
            target: switchThumb
            properties: "x"
            duration: control.animationTime
        }
        PropertyAnimation {
            target: switchThumbRect
            properties: "color"
            duration: control.animationTime
        }
        ColorAnimation {
            target: iconLabel
            duration: control.animationTime
        }
        ColorAnimation {
            target: background
            duration: control.animationTime
        }
        PropertyAnimation {
            target: controlPress
            properties: "opacity,radius"
            duration: control.animationTime
        }
    }
}
