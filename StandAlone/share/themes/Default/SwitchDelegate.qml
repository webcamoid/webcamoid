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

T.SwitchDelegate {
    id: control
    opacity: enabled? 1: 0.5
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + implicitIndicatorWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)
    padding: Ak.newUnit(1 * ThemeSettings.constrolScale, "dp").pixels
    spacing: Ak.newUnit(2 * ThemeSettings.constrolScale, "dp").pixels

    readonly property int animationTime: 100

    indicator: Item {
        id: sliderIndicator
        anchors.rightMargin: Ak.newUnit(1 * ThemeSettings.constrolScale,
                                        "dp").pixels
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        implicitWidth: Ak.newUnit(36 * ThemeSettings.constrolScale,
                                  "dp").pixels
        implicitHeight: Ak.newUnit(20 * ThemeSettings.constrolScale,
                                   "dp").pixels

        Rectangle {
            id: switchTrack
            height: parent.height / 2
            color: Qt.lighter(ThemeSettings.colorBack, 0.75)
            radius: height / 2
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.left: parent.left
        }
        Item {
            id: switchThumb
            width: height
            anchors.bottom: sliderIndicator.bottom
            anchors.top: sliderIndicator.top

            Rectangle {
                id: highlight
                width: control.visualFocus? 2 * parent.width: 0
                height: width
                color: switchThumbRect.color
                radius: width / 2
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                opacity: control.visualFocus? 0.5: 0
            }
            Rectangle {
                id: shadowRect
                color: Qt.lighter(ThemeSettings.colorBack, 1.5)
                radius: height / 2
                anchors.fill: parent
                visible: false
            }
            DropShadow {
                anchors.fill: parent
                cached: true
                horizontalOffset: radius / 2
                verticalOffset: radius / 2
                radius: Ak.newUnit(1
                                   * ThemeSettings.constrolScale, "dp").pixels
                samples: 2 * radius + 1
                color: Qt.lighter(ThemeSettings.colorBack, 0.2)
                source: shadowRect
                visible: !control.flat
            }
            Rectangle {
                id: switchThumbRect
                color: Qt.lighter(ThemeSettings.colorBack, 1.5)
                radius: height / 2
                anchors.fill: parent
            }
        }
    }
    contentItem: Item {
        anchors.left: parent.left
        anchors.right: sliderIndicator.left

        IconLabel {
            id: iconLabel
            spacing: control.spacing
            mirrored: control.mirrored
            display: control.display

            icon: control.icon
            text: control.text
            font: control.font
            color: ThemeSettings.colorText
            anchors.verticalCenter: parent.verticalCenter
        }
        MouseArea {
            anchors.fill: parent

            onClicked: control.toggle()
        }
    }

    states: [
        State {
            name: "Checked"
            when: control.checked
                  && !control.hovered
                  && !control.pressed

            PropertyChanges {
                target: switchTrack
                color: Qt.lighter(ThemeSettings.colorPrimary, 1.5)
            }
            PropertyChanges {
                target: switchThumbRect
                color: Qt.lighter(ThemeSettings.colorPrimary, 1.0)
            }
            PropertyChanges {
                target: switchThumb
                x: sliderIndicator.width - switchThumb.width
            }
        },
        State {
            name: "Hovered"
            when: !control.checked
                  && control.hovered
                  && !control.pressed

            PropertyChanges {
                target: highlight
                width: 2 * switchThumb.width
                opacity: 0.5
            }
        },
        State {
            name: "CheckedHovered"
            when: control.checked
                  && control.hovered
                  && !control.pressed

            PropertyChanges {
                target: switchTrack
                color: Qt.lighter(ThemeSettings.colorPrimary, 1.5)
            }
            PropertyChanges {
                target: switchThumbRect
                color: Qt.lighter(ThemeSettings.colorPrimary, 1.0)
            }
            PropertyChanges {
                target: switchThumb
                x: sliderIndicator.width - switchThumb.width
            }
            PropertyChanges {
                target: highlight
                width: 2 * switchThumb.width
                opacity: 0.5
            }
        },
        State {
            name: "Pressed"
            when: !control.checked
                  && control.pressed

            PropertyChanges {
                target: highlight
                width: 2 * switchThumb.width
                opacity: 0.75
            }
        },
        State {
            name: "CheckedPressed"
            when: control.checked
                  && control.pressed

            PropertyChanges {
                target: switchTrack
                color: Qt.lighter(ThemeSettings.colorPrimary, 1.5)
            }
            PropertyChanges {
                target: switchThumbRect
                color: Qt.lighter(ThemeSettings.colorPrimary, 1.0)
            }
            PropertyChanges {
                target: switchThumb
                x: sliderIndicator.width - switchThumb.width
            }
            PropertyChanges {
                target: highlight
                width: 2 * switchThumb.width
                opacity: 0.75
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
        PropertyAnimation {
            target: highlight
            properties: "width,opacity"
            duration: control.animationTime
        }
    }
}
