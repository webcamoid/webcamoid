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

import QtQuick 2.7
import QtQuick.Dialogs 1.2
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import AkQml 1.0

ColumnLayout {
    id: recRecordConfig

    function updateFields()
    {
        txtDescription.text = Recording.formatDescription(Recording.format)
    }

    function makeFilters()
    {
        var filters = []
        var recordingFormats = Recording.availableFormats;

        for (var format in recordingFormats) {
            var suffix = Recording.formatSuffix(recordingFormats[format])
            var filter = Recording.formatDescription(recordingFormats[format])
                         + " (*."
                         + suffix.join(" *.")
                         + ")"

            filters.push(filter)
        }

        return filters
    }

    function makeDefaultFilter()
    {
        var suffix = Recording.formatSuffix(Recording.format)
        var filter = Recording.formatDescription(Recording.format)
                     + " (*."
                     + suffix.join(" *.")
                     + ")"

        return filter
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
                imgRecordIcon.icon.source = "image://icons/webcamoid-record-stop"
            } else {
                lblRecordLabel.text = qsTr("Start video recording")
                imgRecordIcon.icon.source = "image://icons/webcamoid-record-start"
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
        icon.source: "image://icons/webcamoid-record-start"
        Layout.fillWidth: true
        Layout.preferredHeight: 48

        onClicked: {
            if (Recording.state === AkElement.ElementStatePlaying)
                Recording.state = AkElement.ElementStateNull
            else {
                var filters = recRecordConfig.makeFilters()

                var fileUrl = Webcamoid.saveFileDialog(qsTr("Save video as…"),
                                         recRecordConfig.makeFileName(),
                                         Webcamoid.standardLocations("movies")[0],
                                         "." + recRecordConfig.defaultSuffix(),
                                         recRecordConfig.makeDefaultFilter())

                if (fileUrl !== "") {
                    Recording.videoFileName = fileUrl
                    Recording.state = AkElement.ElementStatePlaying
                }
            }
        }
    }
}
