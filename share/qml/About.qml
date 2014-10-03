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
    id: recAbout
    radius: 10
    opacity: 0.75
    border.width: 1
    border.color: Qt.rgba(0.5, 0.5, 0.5, 1)
    width: 445
    height: 300
    color: Qt.rgba(0, 0, 0, 1)

    Rectangle
    {
        id: recLicense
        color: Qt.rgba(0, 0, 0, 0)
        anchors.rightMargin: 10
        anchors.leftMargin: 10
        anchors.bottomMargin: 10
        anchors.topMargin: 10
        anchors.fill: parent

        Text
        {
            id: txtDescription
            color: Qt.rgba(1, 1, 1, 1)
            font.bold: true
            text: "Carnival LiveCam, Augmented reality made easy."
        }

        Text
        {
            id: txtCopyleft
            color: Qt.rgba(1, 1, 1, 1)
            font.italic: true
            anchors.top: txtDescription.bottom
            text: "Copyright (C) 2011  Gonzalo Exequiel Pedone"
        }

        Text
        {
            id: txtLicense
            anchors.topMargin: 14
            anchors.top: txtCopyleft.bottom
            color: Qt.rgba(1, 1, 1, 1)
            text: "This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with This program.  If not, see <http://www.gnu.org/licenses/>.

Email   : hipersayan DOT x AT gmail DOT com
Web-Site: https://github.com/hipersayanX/Carnival-LiveCam"
        }

    }
}
