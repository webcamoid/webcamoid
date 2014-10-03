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
    id: recWindowControls
    width: 120
    height: 16
    color: Qt.rgba(0, 0, 0, 0)

    property int iconSize: 16
    property color highlightUp: Qt.rgba(0, 0.5, 1, 1)
    property color highlightDown: Qt.rgba(0, 0, 0, 1)
    property bool moving: false

    signal entered
    signal exited

    signal enteredMove
    signal beginMove
    signal move
    signal exitedMove

    signal stayOnTop
    signal minimize
    signal maximize
    signal close

    Component
    {
        id: cmpIconHighlight

        Rectangle
        {
            id: recIconHighlight
            width: recWindowControls.iconSize
            height: recWindowControls.iconSize
            radius: recWindowControls.iconSize / 2

            gradient: Gradient
            {
                GradientStop
                {
                    position: 0
                    color: recWindowControls.highlightUp
                }

                GradientStop
                {
                    position: 1
                    color: recWindowControls.highlightDown
                }
            }
        }
    }

    ListView
    {
        id: lsvWindowControls
        anchors.fill: parent
        spacing: (recWindowControls.width - recWindowControls.iconSize * lsvWindowControls.count) / (lsvWindowControls.count + 1)
        anchors.leftMargin: lsvWindowControls.spacing
        orientation: ListView.Horizontal
        highlight: null
        focus: true
        interactive: false

        model: ListModel
        {
            id: lsmControlsBar

            ListElement
            {
                icon: "qrc:/Webcamoid/share/icons/nostayontop.svg"
            }

            ListElement
            {
                icon: "qrc:/Webcamoid/share/icons/move.svg"
            }

            ListElement
            {
                icon: "qrc:/Webcamoid/share/icons/minimize.svg"
            }

            ListElement
            {
                icon: "qrc:/Webcamoid/share/icons/maximize.svg"
            }

            ListElement
            {
                icon: "qrc:/Webcamoid/share/icons/close.svg"
            }
        }

        delegate: Component
        {
            id: imgIcon

            Image
            {
                width: recWindowControls.iconSize
                height: recWindowControls.iconSize
                source: icon
            }
        }

        MouseArea
        {
            id: msaWindowControls
            hoverEnabled: true
            anchors.fill: parent

            onClicked:
            {
                switch (lsvWindowControls.indexAt(mouseX, mouseY))
                {
                    case 0:
                    {
                        if (lsmControlsBar.get(0).icon == "qrc:/Webcamoid/share/icons/nostayontop.svg")
                            lsmControlsBar.get(0).icon = "qrc:/Webcamoid/share/icons/stayontop.svg"
                        else
                            lsmControlsBar.get(0).icon = "qrc:/Webcamoid/share/icons/nostayontop.svg"

                        recWindowControls.stayOnTop()
                    }
                    break

                    case 1:
                        recWindowControls.move()
                    break

                    case 2:
                        recWindowControls.minimize()
                    break

                    case 3:
                        recWindowControls.maximize()
                    break

                    case 4:
                        recWindowControls.close()
                    break
                }
            }

            onPressed:
            {
                var index = lsvWindowControls.indexAt(mouseX, mouseY)

                if (index < 0)
                    return

                lsvWindowControls.currentIndex = index
                lsvWindowControls.currentItem.scale = 0.75

                if (index == 1)
                {
                    recWindowControls.beginMove()
                    recWindowControls.moving = true
                }
            }

            onPositionChanged:
            {
                var index = lsvWindowControls.indexAt(mouseX, mouseY)
                lsvWindowControls.currentIndex = index

                if (recWindowControls.moving)
                    recWindowControls.move()

                if (index == 1)
                    recWindowControls.enteredMove()
                else
                    recWindowControls.exitedMove()
            }

            onReleased:
            {
                var index = lsvWindowControls.indexAt(mouseX, mouseY)
                lsvWindowControls.currentIndex = index

                if (lsvWindowControls.currentItem)
                    lsvWindowControls.currentItem.scale = 1

                if (recWindowControls.moving)
                    recWindowControls.moving = false
            }

            onEntered: recWindowControls.entered()

            onExited:
            {
                recWindowControls.exitedMove()
                recWindowControls.exited()
            }
        }
    }

    states:
    [
        State
        {
            name: "onHover"
            when: msaWindowControls.containsMouse == true

            PropertyChanges
            {
                target: lsvWindowControls
                highlight: cmpIconHighlight
            }
        }
    ]
}
