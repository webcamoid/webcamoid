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
import QtQuick.Layouts 1.1

GridLayout {
    id: recMediaConfig
    columns: 1

    Connections {
        target: Webcamoid
        onCurStreamChanged: {
            txtMedia.text = Webcamoid.curStream
            txtDescription.text = Webcamoid.streamDescription(Webcamoid.curStream)
            btnEdit.enabled = Webcamoid.canModify(Webcamoid.curStream)
            btnRemove.enabled = Webcamoid.canModify(Webcamoid.curStream)

            Webcamoid.removeCameraControls("itmMediaControls");

            if (Webcamoid.isCamera(Webcamoid.curStream))
                Webcamoid.embedCameraControls("itmMediaControls", Webcamoid.curStream);
        }

        onInterfaceLoaded: {
            Webcamoid.removeCameraControls("itmMediaControls");

            if (Webcamoid.isCamera(Webcamoid.curStream))
                Webcamoid.embedCameraControls("itmMediaControls", Webcamoid.curStream);
        }
    }

    Label {
        id: lblDescription
        color: Qt.rgba(1, 1, 1, 1)
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
        color: Qt.rgba(1, 1, 1, 1)
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
            iconName: "edit"
            text: qsTr("Edit")

            onClicked: dlgAddMedia.visible = true
        }

        Button {
            id: btnRemove
            iconName: "remove"
            text: qsTr("Remove")

            onClicked: Webcamoid.removeStream(Webcamoid.curStream)
        }
    }

    RowLayout {
        id: itmMediaControls
        objectName: "itmMediaControls"
        Layout.fillWidth: true
        Layout.fillHeight: true
    }

    AddMedia {
        id: dlgAddMedia
        editMode: true
    }
}
