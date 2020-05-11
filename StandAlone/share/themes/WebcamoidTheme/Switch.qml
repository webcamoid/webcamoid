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

T.Switch {
    id: control
    icon.width: AkUnit.create(18 * AkTheme.controlScale, "dp").pixels
    icon.height: AkUnit.create(18 * AkTheme.controlScale, "dp").pixels
    icon.color: activeWindowText
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)
    padding: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
    spacing: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
    hoverEnabled: true

    readonly property int animationTime: 100
    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property color activeWindow: AkTheme.palette.active.window
    readonly property color activeWindowText: AkTheme.palette.active.windowText
    readonly property color disabledWindow: AkTheme.palette.disabled.window
    readonly property color disabledWindowText: AkTheme.palette.disabled.windowText

    indicator: Item {
        id: sliderIndicator
        x: control.text?
               (control.mirrored ?
                    control.width - width - control.rightPadding:
                    control.leftPadding):
               control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        implicitWidth:
            AkUnit.create(56 * AkTheme.controlScale, "dp").pixels
        implicitHeight:
            AkUnit.create(40 * AkTheme.controlScale, "dp").pixels

        Rectangle {
            id: switchTrack
            height: parent.height / 4
            color: AkTheme.shade(control.activeWindow, -0.5)
            radius: height / 2
            anchors.verticalCenter: sliderIndicator.verticalCenter
            anchors.right: sliderIndicator.right
            anchors.rightMargin:
                AkUnit.create(10 * AkTheme.controlScale, "dp").pixels
            anchors.left: sliderIndicator.left
            anchors.leftMargin:
                AkUnit.create(10 * AkTheme.controlScale, "dp").pixels
        }
        Item {
            id: switchThumb
            width: height
            anchors.bottom: sliderIndicator.bottom
            anchors.top: sliderIndicator.top

            Rectangle {
                id: highlight
                width: control.visualFocus? parent.width: 0
                height: width
                color: switchThumbRect.color
                radius: width / 2
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                opacity: control.visualFocus? 0.5: 0
            }
            Rectangle {
                id: switchThumbRect
                color: AkTheme.shade(control.activeWindow, -0.1)
                width: parent.width / 2
                height: width
                radius: height / 2
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    contentItem: IconLabel {
        id: iconLabel
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        iconName: control.icon.name
        iconSource: control.icon.source
        iconWidth: control.icon.width
        iconHeight: control.icon.height
        iconColor: control.icon.color
        text: control.text
        font: control.font
        color: control.activeWindowText
        alignment: Qt.AlignLeft | Qt.AlignVCenter
        enabled: control.enabled
        elide: Text.ElideRight
        leftPadding: !control.mirrored?
                         sliderIndicator.width + control.spacing:
                         0
        rightPadding: control.mirrored?
                          sliderIndicator.width + control.spacing:
                          0
    }

    states: [
        State {
            name: "Disabled"
            when: !control.enabled

            PropertyChanges {
                target: switchTrack
                color: AkTheme.shade(control.disabledWindow, -0.5)
            }
            PropertyChanges {
                target: switchThumbRect
                color: AkTheme.shade(control.disabledWindow, -0.1)
            }
            PropertyChanges {
                target: iconLabel
                color: control.disabledWindowText
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
                color: control.activeHighlight
            }
            PropertyChanges {
                target: switchThumbRect
                color: AkTheme.constShade(control.activeHighlight, 0.2)
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
                color: AkTheme.shade(control.activeWindow, -0.6)
            }
            PropertyChanges {
                target: switchThumbRect
                color: AkTheme.shade(control.activeWindow, -0.2)
            }
            PropertyChanges {
                target: highlight
                width: switchThumb.width
                opacity: 0.5
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
                color: AkTheme.constShade(control.activeHighlight, 0.1)
            }
            PropertyChanges {
                target: switchThumbRect
                color: AkTheme.constShade(control.activeHighlight, 0.3)
            }
            PropertyChanges {
                target: switchThumb
                x: sliderIndicator.width - switchThumb.width
            }
            PropertyChanges {
                target: highlight
                width: switchThumb.width
                opacity: 0.5
            }
        },
        State {
            name: "Pressed"
            when: !control.checked
                  && control.pressed

            PropertyChanges {
                target: switchTrack
                color: AkTheme.shade(control.activeWindow, -0.7)
            }
            PropertyChanges {
                target: switchThumbRect
                color: AkTheme.shade(control.activeWindow, -0.3)
            }
            PropertyChanges {
                target: highlight
                width: switchThumb.width
                opacity: 0.75
            }
        },
        State {
            name: "CheckedPressed"
            when: control.checked
                  && control.pressed

            PropertyChanges {
                target: switchTrack
                color: AkTheme.constShade(control.activeHighlight, 0.2)
            }
            PropertyChanges {
                target: switchThumbRect
                color: AkTheme.constShade(control.activeHighlight, 0.4)
            }
            PropertyChanges {
                target: switchThumb
                x: sliderIndicator.width - switchThumb.width
            }
            PropertyChanges {
                target: highlight
                width: switchThumb.width
                opacity: 0.75
            }
        }
    ]

    transitions: Transition {
        ColorAnimation {
            target: switchTrack
            duration: control.animationTime
        }
        PropertyAnimation {
            target: switchThumb
            properties: "x"
            duration: control.animationTime
        }
        ColorAnimation {
            target: switchThumbRect
            duration: control.animationTime
        }
        PropertyAnimation {
            target: highlight
            properties: "width,opacity"
            duration: control.animationTime
        }
    }
}
