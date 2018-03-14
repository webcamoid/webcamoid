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

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import AkQmlControls 1.0

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
        text: qsTr("Media UID")
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

        AkButton {
            id: btnEdit
            label: qsTr("Edit")
            iconRc: "image://icons/edit"

            onClicked: dlgAddMedia.visible = true
        }

        AkButton {
            id: btnRemove
            label: qsTr("Remove")
            iconRc: "image://icons/remove"

            onClicked: {
                var uris = MediaSource.uris
                delete uris[MediaSource.stream]
                MediaSource.uris = uris
            }
        }
    }

    AkScrollView {
        id: scrollControls
        clip: true
        contentHeight: itmMediaControls.height
        Layout.fillWidth: true
        Layout.fillHeight: true

        RowLayout {
            id: itmMediaControls
            objectName: "itmMediaControls"
            width: scrollControls.width
                   - (scrollControls.ScrollBar.vertical.visible?
                          scrollControls.ScrollBar.vertical.width: 0)
        }
    }

    AddMedia {
        id: dlgAddMedia
        editMode: true
    }
}
