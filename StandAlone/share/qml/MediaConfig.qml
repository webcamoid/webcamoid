/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
import QtQuick.Layouts 1.1

GridLayout {
    id: recMediaConfig
    columns: 1

    function showControls(stream)
    {
        txtMedia.text = stream
        txtDescription.text = MediaSource.description(stream)
        btnEdit.enabled = stream in MediaSource.uris
        btnRemove.enabled = btnEdit.enabled

        MediaSource.removeInterface("itmMediaControls");
        MediaSource.embedControls("itmMediaControls", stream);
    }

    Connections {
        target: MediaSource

        onStreamChanged: showControls(stream)
    }

    Connections {
        target: Webcamoid

        onInterfaceLoaded: showControls(MediaSource.stream)
    }

    Label {
        id: lblDescription
        text: qsTr("Description")
        font.bold: true
        Layout.fillWidth: true
    }

    TextField {
        id: txtDescription
        placeholderText: qsTr("Insert media description")
        readOnly: true
        Layout.fillWidth: true
    }

    Label {
        id: lblMedia
        text: qsTr("Media file")
        font.bold: true
        Layout.fillWidth: true
    }

    TextField {
        id: txtMedia
        placeholderText: qsTr("Select media file")
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
            iconName: "edit"
            iconSource: "image://icons/edit"

            onClicked: dlgAddMedia.visible = true
        }

        Button {
            id: btnRemove
            text: qsTr("Remove")
            iconName: "remove"
            iconSource: "image://icons/remove"

            onClicked: {
                var uris = MediaSource.uris
                delete uris[MediaSource.stream]
                MediaSource.uris = uris
            }
        }
    }

    ScrollView {
        id: scrollControls
        Layout.fillWidth: true
        Layout.fillHeight: true

        contentItem: RowLayout {
            id: itmMediaControls
            objectName: "itmMediaControls"
            width: scrollControls.viewport.width
        }
    }

    AddMedia {
        id: dlgAddMedia
        editMode: true
    }
}
