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
import Qt.labs.platform 1.1 as LABS
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import Ak 1.0

ColumnLayout {
    id: recRecordConfig

    function updateFields()
    {
        txtDescription.text = Recording.formatDescription(Recording.format)
    }

    function makeFilters()
    {
        let filters = []
        let recordingFormats = Recording.availableFormats;
        let allSuffix = ""

        for (let format in recordingFormats) {
            let suffix = Recording.formatSuffix(recordingFormats[format])
            let filter = Recording.formatDescription(recordingFormats[format])
                         + " (*." + suffix.join(" *.") + ")"

            if (format > 0)
                allSuffix += " "

            allSuffix += "*." + suffix.join(" *.")

            filters.push(filter)
        }

        filters = ["All Picture Files (" + allSuffix + ")"]
                  .concat(filters)
                  .concat(["All Files (*)"])

        return filters
    }

    function defaultSuffix()
    {
        return Recording.formatSuffix(Recording.format)[0]
    }

    function makeFileName()
    {
        return qsTr("Video %1.%2").arg(Webcamoid.currentTime()).arg(defaultSuffix())
    }

    Connections {
        target: Recording

        onFormatChanged: updateFields()
        onStateChanged: {
            if (state === AkElement.ElementStatePlaying) {
                lblRecordLabel.text = qsTr("Stop video recording")
                imgRecordIcon.icon.source = "image://icons/record-stop"
            } else {
                lblRecordLabel.text = qsTr("Start video recording")
                imgRecordIcon.icon.source = "image://icons/record-start"
            }
        }

    }

    Component.onCompleted: {
        Recording.removeInterface("itmRecordControls");
        Recording.embedControls("itmRecordControls", "");
    }

    Label {
        text: qsTr("Description")
        font.bold: true
    }
    TextField {
        id: txtDescription
        text: Recording.formatDescription(Recording.format)
        placeholderText: qsTr("Description")
        readOnly: true
        Layout.fillWidth: true
    }
    RowLayout {
        id: itmRecordControls
        objectName: "itmRecordControls"
        Layout.fillWidth: true
    }

    Label {
        id: lblRecordLabel
        text: qsTr("Start recording video")
        font.bold: true
        anchors.horizontalCenter: parent.horizontalCenter
    }
    Button {
        id: imgRecordIcon
        icon.width: 32
        icon.height: 32
        icon.source: "image://icons/record-start"
        Layout.fillWidth: true
        Layout.preferredHeight: 48

        onClicked: {
            if (Recording.state === AkElement.ElementStatePlaying)
                Recording.state = AkElement.ElementStateNull
            else
                fileDialog.open()
        }
    }

    LABS.FileDialog {
        id: fileDialog
        title: qsTr("Save video as…")
        folder: "file://" + Webcamoid.standardLocations("movies")[0]
        currentFile: folder + "/" + recRecordConfig.makeFileName()
        defaultSuffix: recRecordConfig.defaultSuffix()
        fileMode: LABS.FileDialog.SaveFile
        selectedNameFilter.index: 0
        nameFilters: recRecordConfig.makeFilters()

        onAccepted: {
            Recording.videoFileName = currentFile
            Recording.state = AkElement.ElementStatePlaying
        }
    }
}
