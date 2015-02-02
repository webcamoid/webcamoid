/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

import QtQuick 2.3
import QtQuick.Controls 1.2

ListView {
    id: lsvOptionList
    width: 400
    height: 400
    clip: true

    property string curOptionName: ""
    property string filter: ""
    property string showField: "description"

    model: ListModel {
    }
    delegate: Item {
        id: itmOption
        height: Webcamoid.matches(filter, [name, description])? 32: 0
        anchors.right: parent.right
        anchors.left: parent.left
        visible: Webcamoid.matches(filter, [name, description])

        property color gradUp: selected?
                                   Qt.rgba(1, 0.75, 0.25, 1):
                                   Qt.rgba(0, 0, 0, 0)
        property color gradLow: selected?
                                    Qt.rgba(1, 0.5, 0, 1):
                                    Qt.rgba(0, 0, 0, 0)

        Rectangle {
            id: recOption
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            gradient: Gradient {
                GradientStop {
                    position: 0
                    color: itmOption.gradUp
                }

                GradientStop {
                    position: 1
                    color: itmOption.gradLow
                }
            }

            Label {
                id: txtOptionText
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                text: lsvOptionList.showField == "description"? description: name
            }

            MouseArea {
                id: msaOption
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                anchors.fill: parent

                onEntered: {
                    txtOptionText.font.bold = true

                    if (selected) {
                        itmOption.gradUp = Qt.rgba(0.25, 0.75, 1, 1)
                        itmOption.gradLow = Qt.rgba(0, 0.5, 1, 1)
                    }
                    else {
                        itmOption.gradUp = Qt.rgba(0.67, 0.5, 1, 0.5)
                        itmOption.gradLow = Qt.rgba(0.5, 0.25, 1, 1)
                    }
                }
                onExited: {
                    txtOptionText.font.bold = false
                    txtOptionText.scale = 1

                    if (selected) {
                        itmOption.gradUp = Qt.rgba(1, 0.75, 0.25, 1)
                        itmOption.gradLow = Qt.rgba(1, 0.5, 0, 1)
                    }
                    else {
                        itmOption.gradUp = Qt.rgba(0, 0, 0, 0)
                        itmOption.gradLow = Qt.rgba(0, 0, 0, 0)
                    }
                }
                onPressed: txtOptionText.scale = 0.75
                onReleased: txtOptionText.scale = 1
                onClicked: {
                    var curIndex = -1
                    var iName = name
                    var options = lsvOptionList

                    for (var i = 0; i < options.count; i++) {
                        var iOption = options.model.get(i).name
                        options.currentIndex = i

                        if (iOption === iName) {
                            itmOption.gradUp = Qt.rgba(0.25, 0.75, 1, 1)
                            itmOption.gradLow = Qt.rgba(0, 0.5, 1, 1)
                            options.model.setProperty(i, "selected", true)
                            curIndex = i
                        }
                        else {
                            options.currentItem.gradUp = Qt.rgba(0, 0, 0, 0)
                            options.currentItem.gradLow = Qt.rgba(0, 0, 0, 0)
                            options.model.setProperty(i, "selected", false)
                        }
                    }

                    options.currentIndex = curIndex
                    options.curOptionName = iName
                    options.positionViewAtIndex(curIndex, ListView.Contain)
                }
            }
        }
    }
}
