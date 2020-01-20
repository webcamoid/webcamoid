/* Webcamoid, webcam capture application.
 * Copyright (C) 2015  Gonzalo Exequiel Pedone
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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3

ColumnLayout {
    id: recMediaConfig

    function showControls(stream)
    {
        txtMedia.text = stream
        txtDescription.text = MediaSource.description(stream)
        btnEdit.enabled = stream in MediaSource.uris
        btnRemove.enabled = btnEdit.enabled

        MediaSource.removeInterface("itmMediaControls")
        MediaSource.embedControls("itmMediaControls", stream)
    }

    Connections {
        target: MediaSource

        onStreamChanged: showControls(stream)
    }

    Connections {
        target: Webcamoid

        onInterfaceLoaded: showControls(MediaSource.stream)
    }

    Component.onCompleted: showControls(MediaSource.stream)

    Label {
        text: qsTr("Description")
        font.bold: true
    }
    TextField {
        id: txtDescription
        placeholderText: qsTr("Description")
        readOnly: true
        Layout.fillWidth: true
    }
    Label {
        text: qsTr("Media ID")
        font.bold: true
    }
    TextField {
        id: txtMedia
        placeholderText: qsTr("Media ID")
        readOnly: true
        Layout.fillWidth: true
    }

    RowLayout {
        id: rowControls
        Layout.fillWidth: true

        Label {
            Layout.fillWidth: true
        }

        Button {
            id: btnEdit
            text: qsTr("Edit")
            icon.source: "image://icons/edit"

            onClicked: dlgAddMedia.visible = true
        }

        Button {
            id: btnRemove
            text: qsTr("Remove")
            icon.source: "image://icons/no"

            onClicked: {
                var uris = MediaSource.uris
                delete uris[MediaSource.stream]
                MediaSource.uris = uris
            }
        }
    }
    RowLayout {
        id: itmMediaControls
        objectName: "itmMediaControls"
        Layout.fillWidth: true
    }

    AddMedia {
        id: dlgAddMedia
        editMode: true
        anchors.centerIn: Overlay.overlay
    }
}
