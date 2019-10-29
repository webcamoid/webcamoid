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
        target: Webcamoid

        onInterfaceLoaded: {
            Recording.removeInterface("itmRecordControls");
            Recording.embedControls("itmRecordControls", "");
        }
    }
    Connections {
        target: Recording

        onFormatChanged: updateFields()
        onStateChanged: {
            if (state === AkElement.ElementStatePlaying) {
                lblRecordLabel.text = qsTr("Stop recording video")
                imgRecordIcon.source = "image://icons/webcamoid-record-stop"
            } else {
                lblRecordLabel.text = qsTr("Start recording video")
                imgRecordIcon.source = "image://icons/webcamoid-record-start"
            }
        }

    }

    Label {
        text: qsTr("Description")
        font.bold: true
        Layout.fillWidth: true
    }
    TextField {
        id: txtDescription
        text: Recording.formatDescription(Recording.format)
        placeholderText: qsTr("Insert format description")
        readOnly: true
        Layout.fillWidth: true
    }
    ScrollView {
        id: scrollControls
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
            clip: true
        contentHeight: itmRecordControls.height
        Layout.fillWidth: true
        Layout.fillHeight: true

        RowLayout {
            id: itmRecordControls
            objectName: "itmRecordControls"
            width: scrollControls.width
                   - (scrollControls.ScrollBar.vertical.visible?
                          scrollControls.ScrollBar.vertical.width: 0)
        }
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
            source: "image://icons/webcamoid-record-start"
            sourceSize: Qt.size(width, height)
        }

        MouseArea {
            id: msaRecord
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
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
                if (Recording.state === AkElement.ElementStatePlaying)
                    Recording.state = AkElement.ElementStateNull
                else {
                    var filters = recRecordConfig.makeFilters()

                    var fileUrl = Webcamoid.saveFileDialog(qsTr("Save video as..."),
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
}
