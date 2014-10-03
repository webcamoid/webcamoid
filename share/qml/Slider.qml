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
    id: recSlider
    width: 16
    height: 256
    radius: width / 2

    property real minValue: 0
    property real maxValue: 99
    property real value: 0
    property bool showUpDown: false

    signal beginMove

    gradient: Gradient
    {
        GradientStop
        {
            position: 0
            color: Qt.rgba(0.12, 0.12, 0.12, 1)
        }

        GradientStop
        {
            position: 1
            color: Qt.rgba(0, 0, 0, 1)
        }
    }

    function updateValue()
    {
        if (recSlider.value < recSlider.minValue)
            recSlider.value = recSlider.minValue

        if (recSlider.value > recSlider.maxValue)
            recSlider.value = recSlider.maxValue

        var k = (recSliderArea.height - btnSlider.height) / (recSlider.maxValue - recSlider.minValue)
        btnSlider.y = Math.round(k * (recSlider.value - recSlider.minValue))
    }

    function setValue(value)
    {
        recSlider.value = value

        if (recSlider.value < recSlider.minValue)
            recSlider.value = recSlider.minValue

        if (recSlider.value > recSlider.maxValue)
            recSlider.value = recSlider.maxValue

        if (recSlider.maxValue == recSlider.minValue)
        {
            btnSlider.y = 0

            return
        }

        var k = (recSliderArea.height - btnSlider.height) / (recSlider.maxValue - recSlider.minValue)
        btnSlider.y = Math.round(k * (recSlider.value - recSlider.minValue))
    }

//    signal valueChanged(int value)

    Timer
    {
        id: tmrSlider
        interval: 100
        repeat: true
        triggeredOnStart: true

        onTriggered:
        {
            var tmpValue = 0
            var k = (recSliderArea.height - btnSlider.height) / (recSlider.maxValue - recSlider.minValue)

            if (btnUp.isPressed)
            {
                tmpValue = recSlider.value - 1

                if (tmpValue >= recSlider.minValue && tmpValue <= recSlider.maxValue)
                    recSlider.value = tmpValue
                else
                    return

                btnSlider.y = Math.round(k * (recSlider.value - recSlider.minValue))
                recSlider.valueChanged(recSlider.value)
            }

            if (btnDown.isPressed)
            {
                tmpValue = recSlider.value + 1

                if (tmpValue >= recSlider.minValue && tmpValue <= recSlider.maxValue)
                    recSlider.value = tmpValue
                else
                    return

                btnSlider.y = Math.round(k * (recSlider.value - recSlider.minValue))
                recSlider.valueChanged(recSlider.value)
            }
        }
    }

    Button
    {
        id: btnUp
        height: recSlider.showUpDown? width: 0
        smooth: true
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.left: parent.left
        radius: width / 2
        visible: recSlider.showUpDown

        onPressed:
        {
            tmrSlider.start()
            recSlider.beginMove()
        }

        onReleased: tmrSlider.stop()
    }

    Button
    {
        id: btnDown
        height: recSlider.showUpDown? width: 0
        smooth: true
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        radius: width / 2
        visible: recSlider.showUpDown

        onPressed:
        {
            tmrSlider.start()
            recSlider.beginMove()
        }

        onReleased: tmrSlider.stop()
    }

    Rectangle
    {
        id: recSliderArea
        color: Qt.rgba(0, 0, 0, 0)
        anchors.bottom: btnDown.top
        anchors.top: btnUp.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        onHeightChanged: recSlider.updateValue()

        MouseArea
        {
            id: msaSliderArea
            anchors.fill: parent

            onClicked:
            {
                var k = (recSliderArea.height - btnSlider.height) / (recSlider.maxValue - recSlider.minValue)
                var tmpValue = Math.round(mouseY / k + recSlider.minValue)

                if (tmpValue >= recSlider.minValue && tmpValue <= recSlider.maxValue)
                    recSlider.value = tmpValue
                else
                    return

                btnSlider.y = Math.round(k * (recSlider.value - recSlider.minValue))
                recSlider.valueChanged(recSlider.value)
            }
        }

        Button
        {
            id: btnSlider
            height: getHeight()
            smooth: true
            anchors.right: parent.right
            anchors.left: parent.left
            radius: width / 2

            property color backFirstColor: Qt.rgba(0.75, 0.75, 0.75, 1)
            property color backSecondColor: Qt.rgba(0.5, 0.5, 0.5, 1)
            property real oldMouseY: 0

            function getHeight()
            {
                var height = recSliderArea.height / (recSlider.maxValue - recSlider.minValue + 1)

                if (height < btnSlider.width)
                    height = btnSlider.width

                return height
            }

            Component.onCompleted: recSlider.updateValue()

            onPressed:
            {
                btnSlider.oldMouseY = mouseY
                recSlider.beginMove()
            }

            onPositionChanged:
            {
                if (!isPressed)
                    return

                var k = (recSliderArea.height - btnSlider.height) / (recSlider.maxValue - recSlider.minValue)
                var dy = mouseY - btnSlider.oldMouseY
                var dvalue = Math.round(dy / k)
                var tmpValue = recSlider.value + dvalue

                if (tmpValue >= recSlider.minValue && tmpValue <= recSlider.maxValue)
                    recSlider.value = tmpValue
                else
                    return

                btnSlider.y = Math.round(k * (recSlider.value - recSlider.minValue))
                recSlider.valueChanged(recSlider.value)
            }
        }
    }
}
