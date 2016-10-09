/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

import QtQuick 2.5

MouseArea {
    id: mouseArea
    width: 48
    height: 48
    hoverEnabled: true
    cursorShape: Qt.PointingHandCursor

    property string text: ""
    property string icon: ""

    onClicked: tooltip.visible = false
    onPressed: icon.scale = 0.75
    onReleased: icon.scale = 1
    onEntered: {
        highlighter.visible = true
        tooltip.text = mouseArea.text
        tooltip.visible = true
    }

    onExited: {
        icon.scale = 1
        highlighter.visible = false
    }

    Rectangle {
        id: highlighter
        radius: mouseArea.height / 2
        anchors.fill: parent
        visible: false
        gradient: Gradient {
            GradientStop {
                position: 0
                color: Qt.rgba(0.67, 0.5, 1, 0.5)
            }

            GradientStop {
                position: 1
                color: Qt.rgba(0.5, 0.25, 1, 1)
            }
        }

        DialogBox {
            id: tooltip
            y: -58
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    Image {
        id: icon
        sourceSize: Qt.size(width, height)
        anchors.fill: parent
        source: mouseArea.icon
        opacity: mouseArea.enabled? 1: 0.25
    }
}
