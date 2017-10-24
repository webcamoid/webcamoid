/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

import QtQuick 2.7
import QtQuick.Controls 2.0

ListView {
    id: lsvOptionList
    width: childrenRect.width
    height: childrenRect.height
    clip: true

    property string filter: ""
    property string textRole: ""

    function optionValues(index)
    {
        if (index < 0 || index >= lsvOptionList.count)
            return []

        var values = []
        var option = lstOptions.get(index)

        for (var key in option)
            if (option[key] && typeof option[key] != "function")
                values.push(String(option[key]))

        return values
    }

    model: ListModel {
        id: lstOptions
    }
    delegate: Rectangle {
        id: rectOption
        height: visible? 32: 0
        anchors.right: parent.right
        anchors.left: parent.left
        visible: Webcamoid.matches(filter, optionValues(index))

        property color gradUp: Qt.rgba(0, 0, 0, 0)
        property color gradLow: Qt.rgba(0, 0, 0, 0)

        gradient: Gradient {
            GradientStop {
                position: 0
                color: rectOption.gradUp
            }
            GradientStop {
                position: 1
                color: rectOption.gradLow
            }
        }

        Label {
            id: txtOptionText
            text: index >= 0 && index < lsvOptionList.count?
                      lsvOptionList.model.get(index)[lsvOptionList.textRole]: ""
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
        }

        MouseArea {
            id: msaOption
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            anchors.fill: parent

            onEntered: {
                txtOptionText.font.bold = true

                if (index == lsvOptionList.currentIndex) {
                    rectOption.gradUp = Qt.rgba(0.25, 0.75, 1, 1)
                    rectOption.gradLow = Qt.rgba(0, 0.5, 1, 1)
                } else {
                    rectOption.gradUp = Qt.rgba(0.67, 0.5, 1, 0.5)
                    rectOption.gradLow = Qt.rgba(0.5, 0.25, 1, 1)
                }
            }
            onExited: {
                txtOptionText.font.bold = false
                txtOptionText.scale = 1
                rectOption.gradUp = Qt.rgba(0, 0, 0, 0)
                rectOption.gradLow = Qt.rgba(0, 0, 0, 0)
            }
            onPressed: txtOptionText.scale = 0.75
            onReleased: txtOptionText.scale = 1
            onClicked: {
                rectOption.gradUp = Qt.rgba(0.25, 0.75, 1, 1)
                rectOption.gradLow = Qt.rgba(0, 0.5, 1, 1)
                lsvOptionList.currentIndex = index
                lsvOptionList.positionViewAtIndex(index, ListView.Contain)
            }
        }
    }
    highlight: Rectangle {
        gradient: Gradient {
            GradientStop {
                position: 0
                color: Qt.rgba(1, 0.75, 0.25, 1)
            }

            GradientStop {
                position: 1
                color: Qt.rgba(1, 0.5, 0, 1)
            }
        }
    }
}
