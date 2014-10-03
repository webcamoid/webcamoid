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

Rectangle
{
    id: recSpaces
    width: 256
    height: 35
    color: "#00000000"
    property real dotSize: 12

    signal spaceSelected(int index, string name)

    function updateSpaces(spaces)
    {
        lsmSpaces.clear()

        for(var space in spaces)
            lsmSpaces.append({"name": spaces[space]})

        lsvSpaces.currentIndex = 0
        txtSpace.text = lsmSpaces.get(0).name
        lsvSpaces.width = lsmSpaces.count * (recSpaces.dotSize +  lsvSpaces.spacing) - lsvSpaces.spacing
        lsvSpaces.height = recSpaces.dotSize
    }

    Text
    {
        id: txtSpace
        color: "#ffffff"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
    }

    ListView
    {
        id: lsvSpaces
        width: 72
        interactive: false
        spacing: 8
        anchors.topMargin: 4
        anchors.top: txtSpace.bottom
        orientation: ListView.Horizontal
        anchors.horizontalCenter: parent.horizontalCenter

        highlight: Rectangle
        {
            id: recHighlight
            width: recSpaces.dotSize
            radius: recSpaces.dotSize / 2
            color: "#ffffff"
            smooth: true
        }

        delegate: Component
        {
            Rectangle
            {
                width: recSpaces.dotSize
                height: width
                radius: width / 2
                color: "#00000000"
                border.color: "#7f7f7f"
                border.width: width / 4
                smooth: true

                MouseArea
                {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: parent.border.color = "#7f7fff"
                    onExited: parent.border.color = "#7f7f7f"
                }
            }
        }

        model: ListModel
        {
            id: lsmSpaces
        }

        MouseArea
        {
            id: msaSpaces
            anchors.fill: parent

            onClicked:
            {
                var index = lsvSpaces.indexAt(mouseX, mouseY)

                if (index >= 0)
                {
                    lsvSpaces.currentIndex = index
                    txtSpace.text = lsmSpaces.get(index).name
                    recSpaces.spaceSelected(index, txtSpace.text)
                }
            }
        }
    }
}
