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
    id: recDialogBox
    width: 128
    height: 56
    color: Qt.rgba(0, 0, 0, 0)

    property color backgroundUp: Qt.rgba(0.25, 0.25, 0.25, 1)
    property color backgroundDown: Qt.rgba(0, 0, 0, 1)
    property color dialogBorder: Qt.rgba(0.12, 0.12, 0.12, 1)
    property color textColor: Qt.rgba(1, 1, 1, 1)
    property real markSize: 8
    property string text: ""

    onTextChanged: {
        txtTitle.text = recDialogBox.text
        recDialogBox.width = txtTitle.width + recDialogBox.height - txtTitle.height
    }

    Rectangle
    {
        id: recMark
        width: recDialogBox.markSize * Math.SQRT2
        height: recDialogBox.markSize * Math.SQRT2
        color: recDialogBox.dialogBorder
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        rotation: 45
    }

    Rectangle
    {
        id: recTitle
        radius: 16
        anchors.bottomMargin: -recDialogBox.markSize * Math.SQRT1_2
        anchors.bottom: recMark.top
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.left: parent.left
        border.width: 4
        border.color: recDialogBox.dialogBorder

        gradient: Gradient
        {
            GradientStop
            {
                position: 0
                color: recDialogBox.backgroundUp
            }

            GradientStop
            {
                position: 1
                color: recDialogBox.backgroundDown
            }
        }

        Text
        {
            id: txtTitle
            text: recDialogBox.text
            color: recDialogBox.textColor
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
