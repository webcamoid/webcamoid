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
    id: recWindow
    radius: 2 * recWindow.borderSize
    border.width: 1
    border.color: Qt.rgba(0.5, 0.5, 0.5, 1)
    width: 640
    height: 480
    color: Qt.rgba(0, 0, 0, 1)

    property bool resizing: false
    property real borderSize: 8

    signal viewPortSizeChanged(int width, int height)

    signal mouseDoubleClicked(real mouseX, real mouseY, variant pressedButtons)
    signal mousePositionChanged(real mouseX, real mouseY, variant pressedButtons)
    signal mousePressed(real mouseX, real mouseY, variant pressedButtons)
    signal mouseReleased(real mouseX, real mouseY, variant pressedButtons)

    signal enteredResizeTopRight
    signal beginResizeTopRight
    signal resizeTopRight
    signal exitedResizeTopRight

    signal enteredResizeTopLeft
    signal beginResizeTopLeft
    signal resizeTopLeft
    signal exitedResizeTopLeft

    signal enteredResizeBottomLeft
    signal beginResizeBottomLeft
    signal resizeBottomLeft
    signal exitedResizeBottomLeft

    signal enteredResizeBottomRight
    signal beginResizeBottomRight
    signal resizeBottomRight
    signal exitedResizeBottomRight

    signal enteredResizeLeft
    signal beginResizeLeft
    signal resizeLeft
    signal exitedResizeLeft

    signal enteredResizeRight
    signal beginResizeRight
    signal resizeRight
    signal exitedResizeRight

    signal enteredResizeTop
    signal beginResizeTop
    signal resizeTop
    signal exitedResizeTop

    signal enteredResizeBottom
    signal beginResizeBottom
    signal resizeBottom
    signal exitedResizeBottom

    Image
    {
        id: imgBackground
        objectName: "WindowBackground"
        cache: false
        smooth: true
        fillMode: Image.PreserveAspectFit
        anchors.fill: parent
//        source: "image://webcam/image"

        MouseArea
        {
            id: msaBackground
            hoverEnabled: true
            width: imgBackground.paintedWidth
            height: imgBackground.paintedHeight
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter

            onDoubleClicked: recWindow.mouseDoubleClicked(mouseX, mouseY, Qt.LeftButton)
            onPositionChanged: recWindow.mousePositionChanged(mouseX, mouseY, Qt.LeftButton)
            onPressed: recWindow.mousePressed(mouseX, mouseY, Qt.LeftButton)
            onReleased: recWindow.mouseReleased(mouseX, mouseY, Qt.LeftButton)

            onWidthChanged: recWindow.viewPortSizeChanged(msaBackground.width, msaBackground.height)
            onHeightChanged: recWindow.viewPortSizeChanged(msaBackground.width, msaBackground.height)
        }
    }

    MouseArea
    {
        id: msaResizeTopRight
        width: 2 * recWindow.borderSize
        height: 2 * recWindow.borderSize
        hoverEnabled: true
        anchors.right: parent.right
        anchors.top: parent.top

        onEntered: parent.enteredResizeTopRight()

        onPressed:
        {
            parent.beginResizeTopRight()
            recWindow.resizing = true
        }

        onPositionChanged:
        {
            if (recWindow.resizing)
                parent.resizeTopRight()
        }

        onReleased: recWindow.resizing = false
        onExited: parent.exitedResizeTopRight()
    }

    MouseArea
    {
        id: msaResizeTopLeft
        width: 2 * recWindow.borderSize
        height: 2 * recWindow.borderSize
        hoverEnabled: true
        anchors.left: parent.left
        anchors.top: parent.top

        onEntered: parent.enteredResizeTopLeft()

        onPressed:
        {
            parent.beginResizeTopLeft()
            recWindow.resizing = true
        }

        onPositionChanged:
        {
            if (recWindow.resizing)
                parent.resizeTopLeft()
        }

        onReleased: recWindow.resizing = false
        onExited: parent.exitedResizeTopLeft()
    }

    MouseArea
    {
        id: msaResizeBottomLeft
        width: 2 * recWindow.borderSize
        height: 2 * recWindow.borderSize
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        hoverEnabled: true

        onEntered: parent.enteredResizeBottomLeft()

        onPressed:
        {
            parent.beginResizeBottomLeft()
            recWindow.resizing = true
        }

        onPositionChanged:
        {
            if (recWindow.resizing)
                parent.resizeBottomLeft()
        }

        onReleased: recWindow.resizing = false
        onExited: parent.exitedResizeBottomLeft()
    }

    MouseArea
    {
        id: msaResizeBottomRight
        width: 2 * recWindow.borderSize
        height: 2 * recWindow.borderSize
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        hoverEnabled: true

        onEntered: parent.enteredResizeBottomRight()

        onPressed:
        {
            parent.beginResizeBottomRight()
            recWindow.resizing = true
        }

        onPositionChanged:
        {
            if (recWindow.resizing)
                parent.resizeBottomRight()
        }

        onReleased: recWindow.resizing = false
        onExited: parent.exitedResizeBottomRight()
    }

    MouseArea
    {
        id: msaResizeLeft
        width: recWindow.borderSize
        hoverEnabled: true
        anchors.top: msaResizeTopLeft.bottom
        anchors.bottom: msaResizeBottomLeft.top
        anchors.left: parent.left

        onEntered: parent.enteredResizeLeft()

        onPressed:
        {
            parent.beginResizeLeft()
            recWindow.resizing = true
        }

        onPositionChanged:
        {
            if (recWindow.resizing)
                parent.resizeLeft()
        }

        onReleased: recWindow.resizing = false
        onExited: parent.exitedResizeLeft()
    }

    MouseArea
    {
        id: msaResizeRight
        width: recWindow.borderSize
        hoverEnabled: true
        anchors.top: msaResizeTopRight.bottom
        anchors.bottom: msaResizeBottomRight.top
        anchors.right: parent.right

        onEntered: parent.enteredResizeRight()

        onPressed:
        {
            parent.beginResizeRight()
            recWindow.resizing = true
        }

        onPositionChanged:
        {
            if (recWindow.resizing)
                parent.resizeRight()
        }

        onReleased: recWindow.resizing = false
        onExited: parent.exitedResizeRight()
    }

    MouseArea
    {
        id: msaResizeTop
        height: recWindow.borderSize
        hoverEnabled: true
        anchors.right: msaResizeTopRight.left
        anchors.left: msaResizeTopLeft.right
        anchors.top: parent.top

        onEntered: parent.enteredResizeTop()

        onPressed:
        {
            parent.beginResizeTop()
            recWindow.resizing = true
        }

        onPositionChanged:
        {
            if (recWindow.resizing)
                parent.resizeTop()
        }

        onReleased: recWindow.resizing = false
        onExited: parent.exitedResizeTop()
    }

    MouseArea
    {
        id: msaResizeBottom
        height: recWindow.borderSize
        hoverEnabled: true
        anchors.right: msaResizeBottomRight.left
        anchors.left: msaResizeBottomLeft.right
        anchors.bottom: parent.bottom

        onEntered: parent.enteredResizeBottom()

        onPressed:
        {
            parent.beginResizeBottom()
            recWindow.resizing = true
        }

        onPositionChanged:
        {
            if (recWindow.resizing)
                parent.resizeBottom()
        }

        onReleased: recWindow.resizing = false
        onExited: parent.exitedResizeBottom()
    }
}
