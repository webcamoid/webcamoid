/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

import QtQuick
import QtQuick.Templates as T
import Ak

T.SelectionRectangle {
    id: control
    topLeftHandle: handle
    bottomRightHandle: handle

    readonly property color activeHighlight: AkTheme.palette.active.highlight
    readonly property int animationTime: 200

    Component {
        id: handle

        Item {
            id: handleItem
            width: AkUnit.create(20 * AkTheme.controlScale, "dp").pixels
            height: width
            visible: SelectionRectangle.control.active

            property Item rectControl: SelectionRectangle.control
            property bool dragging: SelectionRectangle.dragging

            Rectangle {
                id: highlight
                width: 2 * parent.width
                height: width
                radius: width / 2
                color: control.activeHighlight
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                opacity: 0.5
            }
            Rectangle {
                id: handleRect
                width: parent.width
                height: width
                radius: width / 2
                anchors.centerIn: parent
                color: control.activeHighlight
            }

            states: [
                State {
                    name: "Dragged"
                    when: handleItem.dragging

                    PropertyChanges {
                        target: highlight
                        color: AkTheme.constShade(control.activeHighlight, 0.2)
                    }
                    PropertyChanges {
                        target: handleRect
                        color: AkTheme.constShade(control.activeHighlight, 0.2)
                    }
                }
            ]
            transitions: Transition {
                ColorAnimation {
                    target: highlight
                    duration: control.animationTime
                }
                ColorAnimation {
                    target: handleRect
                    duration: control.animationTime
                }
            }
        }
    }
}
