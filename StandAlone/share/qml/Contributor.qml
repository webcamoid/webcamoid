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

import QtQuick 2.5
import QtQuick.Controls 1.4

Image {
    id: imgContributor
    source: avatar
    fillMode: Image.PreserveAspectFit
    width: avatarSize
    height: avatarSize

    property int avatarSize: 96
    property string name: ""
    property string avatar: ""
    property string website: ""

    Rectangle {
        color: "black"
        opacity: 0.75
        height: Math.max(24, childrenRect.height + 4)
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: msaContributor.containsMouse

        Label {
            text: imgContributor.name
            color: "white"
            font.pointSize: 8
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    MouseArea {
        id: msaContributor
        cursorShape: Qt.PointingHandCursor
        hoverEnabled : true
        anchors.fill: parent

        onClicked: Qt.openUrlExternally(imgContributor.website)
    }
}
