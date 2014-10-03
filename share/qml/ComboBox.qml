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

Button
{
    id: recComboBox
    width: 100
    height: 24
    radius: 8
    borderColorNormal: Qt.rgba(0.5, 0.5, 0.5, 1)

    property bool isExpanded: false
    property real currentIndex: 0
    property string currentText: ""
    property string currentValue: ""

    signal itemSelected(int index, string text, string value)

    onEntered: recExpand.border.color = recComboBox.borderColorHover
    onExited: recExpand.border.color = recComboBox.borderColorNormal
    onPressed: recExpand.border.color = recComboBox.borderColorPressed
    onReleased: recExpand.border.color = recComboBox.borderColorHover
    onClicked: recComboBox.isExpanded = !recComboBox.isExpanded

    function updateOptions(options)
    {
        recComboBox.currentText = (options[recComboBox.currentIndex])? options[recComboBox.currentIndex][0]: ""
        recComboBox.currentValue = (options[recComboBox.currentIndex])? options[recComboBox.currentIndex][1]: ""
        lswOptions.updateOptions(options)
    }

    Text
    {
        id: txtOption
        y: 24
        text: recComboBox.currentText
        color: Qt.rgba(1, 1, 1, 1)
        anchors.right: recExpand.left
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    Rectangle
    {
        id: recExpand
        width: 8
        height: 8
        color: Qt.rgba(0, 0, 0, 0)
        radius: 3
        smooth: true
        anchors.rightMargin: 4
        border.width: 1
        border.color: Qt.rgba(0.5, 0.5, 0.5, 1)
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
    }

    ListWidget
    {
        id: lswOptions
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: parent.top
        radius: recComboBox.radius
        opacity: 0

        onItemSelected:
        {
            recComboBox.currentIndex = index
            recComboBox.currentText = text
            recComboBox.currentValue = value
            recComboBox.isExpanded = false
            recComboBox.itemSelected(index, text, value)
        }

        onEscapePressed:
        {
            if (recComboBox.isExpanded)
                recComboBox.isExpanded = false

            setIndex(recComboBox.currentIndex)
        }
    }

    states:
    [
        State
        {
            name: "State1"
            when: recComboBox.isExpanded == true

            PropertyChanges
            {
                target: lswOptions
                opacity: 1
            }
        }
    ]
}
