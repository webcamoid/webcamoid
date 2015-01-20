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
import QtQuick.Dialogs 1.2
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

ColumnLayout {
    id: recRecordConfig

    function updateFields()
    {
        txtDescription.text = Webcamoid.curRecordingFormat
        txtSuffix.text = Webcamoid.recordingFormatSuffix(Webcamoid.curRecordingFormat).join()
        txtParams.text = Webcamoid.recordingFormatParams(Webcamoid.curRecordingFormat)
    }

    function makeFilters()
    {
        var filters = []

        for (var format in Webcamoid.recordingFormats) {
            var suffix = Webcamoid.recordingFormatSuffix(Webcamoid.recordingFormats[format])
            var filter = Webcamoid.recordingFormats[format]
                         + " (*."
                         + suffix.join(" *.")
                         + ")"

            filters.push(filter)
        }

        return filters
    }

    function makeFileName()
    {
        var defaultSuffix = Webcamoid.recordingFormatSuffix(Webcamoid.curRecordingFormat)[0]

        return fileDialog.folder + "/Video " + Webcamoid.currentTime() + "." + defaultSuffix
    }

    Connections {
        target: Webcamoid
        onCurRecordingFormatChanged: updateFields()
        onRecordingFormatsChanged: updateFields()

        onRecordingChanged: {
            if (recording) {
                lblRecordLabel.text = qsTr("Stop recording video")
                imgRecordIcon.source = "qrc:/Webcamoid/share/icons/stoprecord.svg"
            }
            else {
                lblRecordLabel.text = qsTr("Start recording video")
                imgRecordIcon.source = "qrc:/Webcamoid/share/icons/startrecord.svg"
            }
        }
    }

    Label {
        color: Qt.rgba(1, 1, 1, 1)
        text: qsTr("Description")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        id: txtDescription
        text: Webcamoid.curRecordingFormat
        placeholderText: qsTr("Insert format description")
        readOnly: true
        Layout.fillWidth: true
    }

    Label {
        color: Qt.rgba(1, 1, 1, 1)
        text: qsTr("Suffix")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        id: txtSuffix
        text: Webcamoid.recordingFormatSuffix(Webcamoid.curRecordingFormat).join()
        placeholderText: qsTr("Supported file suffix")
        readOnly: true
        Layout.fillWidth: true
    }

    Label {
        color: Qt.rgba(1, 1, 1, 1)
        text: qsTr("Params")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        id: txtParams
        text: Webcamoid.recordingFormatParams(Webcamoid.curRecordingFormat)
        placeholderText: qsTr("Encoding parameters")
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

            onClicked: dlgAddRecordingFormat.visible = true
        }

        Button {
            id: btnRemove
            iconName: "remove"
            text: qsTr("Remove")

            onClicked: Webcamoid.removeRecordingFormat(Webcamoid.curRecordingFormat)
        }
    }

    Rectangle {
        Layout.fillHeight: true
    }

    Label {
        id: lblRecordLabel
        text: qsTr("Start recording video")
        font.bold: true
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Rectangle {
        id: rectangle1
        Layout.fillWidth: true
        Layout.preferredHeight: 48

        gradient: Gradient {
            GradientStop {
                id: gradTop
                position: 0
                color: "#1f1f1f"
            }

            GradientStop {
                id: gradMiddle
                position: 0.5
                color: "#000000"
            }

            GradientStop {
                id: gradBottom
                position: 1
                color: "#1f1f1f"
            }
        }

        Image {
            id: imgRecordIcon
            width: 32
            height: 32
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            source: "qrc:/Webcamoid/share/icons/startrecord.svg"
        }

        MouseArea {
            id: msaRecord
            hoverEnabled: true
            anchors.fill: parent

            onEntered: {
                gradTop.color = "#2e2e2e"
                gradMiddle.color = "#0f0f0f"
                gradBottom.color = "#2e2e2e"
            }
            onExited: {
                gradTop.color = "#1f1f1f"
                gradMiddle.color = "#000000"
                gradBottom.color = "#1f1f1f"
                imgRecordIcon.scale = 1
            }
            onPressed: imgRecordIcon.scale = 0.75
            onReleased: imgRecordIcon.scale = 1
            onClicked: {
                if (Webcamoid.recording)
                    Webcamoid.resetRecording();
                else
                    fileDialog.visible = true
            }
        }
    }

    AddRecordingFormat {
        id: dlgAddRecordingFormat
        editMode: true
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Save video as...")
        folder: Webcamoid.standardLocations("movies")[0]
        selectExisting: false
        selectMultiple: false
        selectedNameFilter: Webcamoid.curRecordingFormat
        nameFilters: recRecordConfig.makeFilters()

        onAccepted: Webcamoid.setRecording(true, fileUrl);
    }
}
