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

import QtQuick 2.0
import QtQuick.Window 2.0
import QtQuick.Dialogs 1.2
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

ApplicationWindow {
    id: recAddMedia
    title: qsTr("Add new media")
    color: pallete.window
    flags: Qt.Dialog
    modality: Qt.ApplicationModal

    property bool editMode: false

    function defaultDescription(url)
    {
        return Webcamoid.streams.indexOf(url) < 0?
                    Webcamoid.fileNameFromUri(url):
                    Webcamoid.streamDescription(url)
    }

    SystemPalette {
        id: pallete
    }

    ColumnLayout {
        anchors.rightMargin: 8
        anchors.leftMargin: 8
        anchors.bottomMargin: 8
        anchors.topMargin: 8
        anchors.fill: parent

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
            text: recAddMedia.editMode? Webcamoid.streamDescription(Webcamoid.curStream): ""
            Layout.fillWidth: true
        }

        Label {
            id: lblMedia
            color: Qt.rgba(1, 1, 1, 1)
            text: qsTr("Media file")
            font.bold: true
            Layout.fillWidth: true
        }

        RowLayout {
            TextField {
                id: txtMedia
                placeholderText: qsTr("Select media file")
                text: recAddMedia.editMode? Webcamoid.curStream: ""
                Layout.fillWidth: true
            }

            Button {
                id: btnAddMedia
                width: 30
                text: qsTr("...")

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
                iconName: "ok"
                text: qsTr("Ok")

                onClicked: {
                    if (txtMedia.text.length > 0) {
                        if (recAddMedia.editMode
                            && Webcamoid.curStream !== txtMedia.text.toString())
                            Webcamoid.removeStream(Webcamoid.curStream)

                        if (txtDescription.text.length < 1)
                            txtDescription.text = recAddMedia.defaultDescription(txtMedia.text)

                        Webcamoid.setStream(txtMedia.text, txtDescription.text)
                        Webcamoid.curStream = txtMedia.text
                    }

                    recAddMedia.visible = false
                }
            }

            Button {
                id: btnCancel
                iconName: "cancel"
                text: qsTr("Cancel")

                onClicked: recAddMedia.visible = false
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Choose the file to add as media")
        selectExisting : true
        selectFolder : false
        selectMultiple : false
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
            txtMedia.text = fileDialog.fileUrl
            txtDescription.text = recAddMedia.defaultDescription(fileDialog.fileUrl.toString())
        }
    }
}
