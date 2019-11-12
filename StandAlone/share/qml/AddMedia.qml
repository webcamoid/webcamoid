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
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

ApplicationWindow {
    id: recAddMedia
    title: qsTr("Add new media")
    flags: Qt.Dialog
    modality: Qt.ApplicationModal
    width: 350
    height: 200

    property bool editMode: false

    function defaultDescription(url)
    {
        return MediaSource.streams.indexOf(url) < 0?
                    Webcamoid.fileNameFromUri(url):
                    MediaSource.description(url)
    }

    onVisibleChanged: {
        if (!visible)
            return

        txtDescription.text = recAddMedia.editMode?
                    MediaSource.description(MediaSource.stream): ""
        txtMedia.text = recAddMedia.editMode?
                    MediaSource.stream: ""
    }

    ColumnLayout {
        anchors.rightMargin: 8
        anchors.leftMargin: 8
        anchors.bottomMargin: 8
        anchors.topMargin: 8
        anchors.fill: parent

        Label {
            id: lblDescription
            text: qsTr("Description")
            font.bold: true
            Layout.fillWidth: true
        }
        TextField {
            id: txtDescription
            placeholderText: qsTr("Insert a media description")
            text: recAddMedia.editMode? MediaSource.description(MediaSource.stream): ""
            Layout.fillWidth: true
        }

        Label {
            id: lblMedia
            text: qsTr("Media file")
            font.bold: true
            Layout.fillWidth: true
        }
        RowLayout {
            TextField {
                id: txtMedia
                placeholderText: qsTr("Select a media file")
                text: recAddMedia.editMode? MediaSource.stream: ""
                Layout.fillWidth: true
            }

            Button {
                id: btnAddMedia
                text: qsTr("Search")
                icon.source: "image://icons/edit-find"

                onClicked: fileDialog.open()
            }
        }

        Label {
            Layout.fillHeight: true
        }

        RowLayout {
            id: rowControls

            Label {
                Layout.fillWidth: true
            }

            Button {
                id: btnOk
                text: qsTr("Ok")
                icon.source: "image://icons/ok"

                onClicked: {
                    if (txtMedia.text.length > 0) {
                        var uris = MediaSource.uris;

                        if (recAddMedia.editMode
                            && MediaSource.stream !== txtMedia.text.toString())
                            delete uris[MediaSource.stream];

                        if (txtDescription.text.length < 1)
                            txtDescription.text = recAddMedia.defaultDescription(txtMedia.text);

                        uris[txtMedia.text] = txtDescription.text;
                        MediaSource.uris = uris;
                        MediaSource.stream = txtMedia.text;
                    }

                    recAddMedia.visible = false
                }
            }

            Button {
                id: btnCancel
                text: qsTr("Cancel")
                icon.source: "image://icons/cancel"

                onClicked: recAddMedia.visible = false
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Choose the file to add as media")
        selectExisting: true
        selectFolder: false
        selectMultiple: false
        selectedNameFilter: nameFilters[nameFilters.length - 2]
        nameFilters: ["3GP Video (*.3gp)",
                      "AVI Video (*.avi)",
                      "Flash Video (*.flv)",
                      "Animated GIF (*.gif)",
                      "MKV Video (*.mkv)",
                      "Animated PNG (*.mng)",
                      "QuickTime Video (*.mov)",
                      "MP4 Video (*.mp4 *.m4v)",
                      "MPEG Video (*.mpg *.mpeg)",
                      "Ogg Video (*.ogg)",
                      "RealMedia Video (*.rm)",
                      "DVD Video (*.vob)",
                      "WebM Video (*.webm)",
                      "Windows Media Video (*.wmv)",
                      "All Video Files (*.3gp *.avi *.flv *.gif *.mkv *.mng"
                      + " *.mov *.mp4 *.m4v *.mpg *.mpeg *.ogg *.rm *.vob"
                      + " *.webm *.wmv)",
                      "All Files (*)"]

        onAccepted: {
            txtMedia.text = Webcamoid.urlToLocalFile(fileDialog.fileUrl)
            txtDescription.text = recAddMedia.defaultDescription(fileDialog.fileUrl.toString())
        }
    }
}
