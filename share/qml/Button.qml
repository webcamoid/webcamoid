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
    id: recButton
    width: 32
    height: 32
    radius: Math.sqrt(Math.pow(width, 2) + Math.pow(height, 2)) / (5 * Math.SQRT2)
    smooth: true
    border.color: borderColorNormal

    property url icon: ""
    property real disabledFactor: 2
    property color borderColorNormal: Qt.darker(Qt.rgba(0, 0, 0, 0), (recButton.enabled)? 0: recButton.disabledFactor)
    property color borderColorHover: Qt.darker(Qt.rgba(0.5, 0.5, 1, 1), (recButton.enabled)? 0: recButton.disabledFactor)
    property color borderColorPressed: Qt.darker(Qt.rgba(1, 1, 1, 1), (recButton.enabled)? 0: recButton.disabledFactor)
    property color backFirstColor: Qt.darker(Qt.rgba(0.25, 0.25, 0.25, 1), (recButton.enabled)? 0: recButton.disabledFactor)
    property color backSecondColor: Qt.darker(Qt.rgba(0, 0, 0, 1), (recButton.enabled)? 0: recButton.disabledFactor)
    property real mouseX: 0
    property real mouseY: 0
    property bool isPressed: false
    property bool enabled: true

    gradient: Gradient
    {
        GradientStop
        {
            id: gradientstop1
            position: 0
            color: recButton.backFirstColor
        }

        GradientStop
        {
            id: gradientstop2
            position: 1
            color: recButton.backSecondColor
        }
    }

    signal canceled
    signal clicked
    signal doubleClicked
    signal entered
    signal exited
    signal positionChanged
    signal pressAndHold
    signal pressed
    signal released

    MouseArea
    {
        id: msaIcon
        anchors.fill: parent
        hoverEnabled: true

        onCanceled:
        {
            recButton.mouseX = mouseX
            recButton.mouseY = mouseY

            if (recButton.enabled)
                recButton.canceled()
        }

        onClicked:
        {
            recButton.mouseX = mouseX
            recButton.mouseY = mouseY

            if (recButton.enabled)
                recButton.clicked()
        }

        onDoubleClicked:
        {
            recButton.mouseX = mouseX
            recButton.mouseY = mouseY

            if (recButton.enabled)
                recButton.doubleClicked()
        }

        onEntered:
        {
            recButton.mouseX = mouseX
            recButton.mouseY = mouseY

            if (recButton.enabled)
            {
                recButton.border.color = recButton.borderColorHover
                recButton.entered()
            }
        }

        onExited:
        {
            recButton.mouseX = mouseX
            recButton.mouseY = mouseY

            if (recButton.enabled)
            {
                recButton.border.color = recButton.borderColorNormal
                recButton.exited()
            }
        }

        onPositionChanged:
        {
            recButton.mouseX = mouseX
            recButton.mouseY = mouseY

            if (recButton.enabled)
                recButton.positionChanged(mouse)
        }

        onPressAndHold:
        {
            recButton.mouseX = mouseX
            recButton.mouseY = mouseY

            if (recButton.enabled)
                recButton.pressAndHold(mouse)
        }

        onPressed:
        {
            recButton.mouseX = mouseX
            recButton.mouseY = mouseY

            if (recButton.enabled)
            {
                recButton.isPressed = true
                recButton.border.color = recButton.borderColorPressed
                recButton.pressed()
            }
        }

        onReleased:
        {
            recButton.mouseX = mouseX
            recButton.mouseY = mouseY

            if (recButton.enabled)
            {
                recButton.isPressed = false
                recButton.border.color = recButton.borderColorHover
                recButton.released()
            }
        }
    }

    Image
    {
        id: imgIcon
        width: 0.75 * recButton.width
        height: 0.75 * recButton.height
        smooth: true
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        source: recButton.icon
        opacity: (recButton.enabled)? 1: 0.5
    }

    states:
    [
        State
        {
            name: "Pressed"
            when: msaIcon.pressed && recButton.enabled

            PropertyChanges
            {
                target: gradientstop1
                position: 0
                color: recButton.backSecondColor
            }

            PropertyChanges
            {
                target: gradientstop2
                position: 1
                color: recButton.backFirstColor
            }

            PropertyChanges
            {
                target: msaIcon
            }

            PropertyChanges
            {
                target: imgIcon
                scale: 0.75
            }
        }
    ]
}
