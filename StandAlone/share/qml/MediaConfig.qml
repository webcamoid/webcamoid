/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

import QtQuick 2.5
import QtQuick.Controls 1.4
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
            iconSource: "qrc:/icons/hicolor/scalable/edit.svg"

            onClicked: dlgAddMedia.visible = true
        }

        Button {
            id: btnRemove
            text: qsTr("Remove")
            iconName: "remove"
            iconSource: "qrc:/icons/hicolor/scalable/remove.svg"

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
