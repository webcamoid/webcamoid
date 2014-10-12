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
    id: recMediaConfig
    color: Qt.rgba(0, 0, 0, 1)
    clip: true
    width: 200
    height: 400

    Connections {
        target: Webcamoid
        onCurStreamChanged: {
            txtMedia.text = Webcamoid.curStream
            txtDescription.text = Webcamoid.streamDescription(Webcamoid.curStream)
            btnEdit.enabled = Webcamoid.canModify(Webcamoid.curStream)
            btnRemove.enabled = Webcamoid.canModify(Webcamoid.curStream)
        }
    }

    Label {
        id: lblDescription
        color: Qt.rgba(1, 1, 1, 1)
        text: qsTr("Description")
        font.bold: true
        anchors.rightMargin: 16
        anchors.leftMargin: 16
        anchors.topMargin: 16
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.left: parent.left
    }

    TextField {
        id: txtDescription
        anchors.top: lblDescription.bottom
        anchors.topMargin: 16
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.left: parent.left
        anchors.leftMargin: 16
        placeholderText: qsTr("Insert media description")
        readOnly: true
    }

    Label {
        id: lblMedia
        color: Qt.rgba(1, 1, 1, 1)
        text: qsTr("Media file")
        font.bold: true
        anchors.top: txtDescription.bottom
        anchors.topMargin: 16
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.left: parent.left
        anchors.leftMargin: 16
    }

    TextField {
        id: txtMedia
        anchors.top: lblMedia.bottom
        anchors.topMargin: 16
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.left: parent.left
        anchors.leftMargin: 16
        placeholderText: qsTr("Select media file")
        readOnly: true
    }

    Row {
        id: rowControls
        layoutDirection: Qt.RightToLeft
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.topMargin: 16
        anchors.top: txtMedia.bottom

        Button {
            id: btnRemove
            iconName: "remove"
            text: qsTr("Remove")

            onClicked: Webcamoid.removeStream(Webcamoid.curStream)
        }

        Button {
            id: btnEdit
            iconName: "edit"
            text: qsTr("Edit")

            onClicked: dlgAddMedia.visible = true
        }
    }

    Item {
        id: itmMediaControls
        objectName: "itmMediaControls"
        anchors.top: rowControls.bottom
        anchors.topMargin: 16
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 16
    }

    AddMedia {
        id: dlgAddMedia
        editMode: true
    }
}
