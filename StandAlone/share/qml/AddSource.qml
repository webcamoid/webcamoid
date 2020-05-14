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

Dialog {
    id: addSource
    title: qsTr("Add source")
    standardButtons: Dialog.Ok | Dialog.Cancel
    width: AkUnit.create(420 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(320 * AkTheme.controlScale, "dp").pixels
    modal: true

    property bool editMode: false

    function isFile(url)
    {
        if (RegExp("^file://", "gi").test(url))
            return true

        return !RegExp("^[a-z][a-z0-9+-.]*://", "gi").test(url)
    }

    function defaultDescription(url)
    {
        return videoLayer.inputs.indexOf(url) < 0?
                    mediaTools.fileNameFromUri(url):
                    videoLayer.description(url)
    }

    ColumnLayout {
        anchors.fill: parent

        TabBar {
            id: tabBar
            Layout.fillWidth: true

            TabButton {
                text: qsTr("File")
            }
            TabButton {
                text: qsTr("URL")
            }
        }
        StackLayout {
            id: stack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            ScrollView {
                id: fileScrollView
                width: stack.width
                height: stack.height
                contentHeight: fileLayout.height
                clip: true

                ColumnLayout {
                    id: fileLayout
                    width: fileScrollView.width

                    Label {
                        text: qsTr("Description")
                        font.bold: true
                        Layout.fillWidth: true
                    }
                    TextField {
                        id: fileDescription
                        placeholderText: qsTr("Source title")
                        text: addSource.editMode?
                                  videoLayer.description(videoLayer.videoInput):
                                  ""
                        Layout.fillWidth: true
                        selectByMouse: true
                    }
                    Label {
                        text: qsTr("Path")
                        font.bold: true
                        Layout.fillWidth: true
                    }
                    RowLayout {
                        TextField {
                            id: filePath
                            placeholderText: qsTr("File path")
                            text: addSource.editMode? videoLayer.videoInput: ""
                            Layout.fillWidth: true
                            selectByMouse: true
                        }

                        Button {
                            text: qsTr("Search")
                            icon.source: "image://icons/search"

                            onClicked: fileDialog.open()
                        }
                    }
                }
            }
            ScrollView {
                id: urlScrollView
                width: stack.width
                height: stack.height
                contentHeight: urlLayout.height
                clip: true

                ColumnLayout {
                    id: urlLayout
                    width: urlScrollView.width

                    Label {
                        text: qsTr("Description")
                        font.bold: true
                        Layout.fillWidth: true
                    }
                    TextField {
                        id: urlDescription
                        placeholderText: qsTr("Source title")
                        text: addSource.editMode?
                                  videoLayer.description(videoLayer.videoInput):
                                  ""
                        Layout.fillWidth: true
                        selectByMouse: true
                    }
                    Label {
                        text: qsTr("URL")
                        font.bold: true
                        Layout.fillWidth: true
                    }
                    TextField {
                        id: urlPath
                        placeholderText: "https://example-site.com/video.webm"
                        text: addSource.editMode? videoLayer.videoInput: ""
                        Layout.fillWidth: true
                        selectByMouse: true
                    }
                }
            }
        }
    }

    onVisibleChanged: {
        if (!visible)
            return

        fileDescription.text = addSource.editMode?
                    videoLayer.description(videoLayer.videoInput): ""
        urlDescription.text = fileDescription.text
        filePath.text = ""
        urlPath.text = ""
        tabBar.currentIndex = 0

        if (addSource.editMode) {
            if (addSource.isFile(videoLayer.videoInput)) {
                filePath.text = videoLayer.videoInput
            } else {
                urlPath.text = videoLayer.videoInput
                tabBar.currentIndex = 1
            }
        }
    }

    onAccepted: {
        let description = ""
        let uri = ""

        if (tabBar.currentIndex == 0) {
            description = fileDescription.text
            uri = filePath.text
        } else {
            description = urlDescription.text
            uri = urlPath.text
        }

        if (uri.length > 0) {
            if (description.length < 1)
                description = addSource.defaultDescription(uri)

            if (editMode)
                videoLayer.removeInputStream(videoLayer.videoInput)

            videoLayer.setInputStream(uri, description)
            videoLayer.videoInput = uri
        }
    }

    LABS.FileDialog {
        id: fileDialog
        title: qsTr("Choose the file to add as source")
        fileMode: LABS.FileDialog.OpenFile
        selectedNameFilter.index: 0
        nameFilters: [qsTr("All Video Files")
                      + " (*.3gp *.avi *.flv *.gif *.mkv *.mng  *.mov *.mp4"
                      + " *.m4v *.mpg *.mpeg *.ogg *.rm *.vob *.webm *.wmv)",
                      qsTr("3GP Video") + " (*.3gp)",
                      qsTr("AVI Video") + " (*.avi)",
                      //: Adobe FLV Flash video
                      qsTr("Flash Video") + " (*.flv)",
                      qsTr("Animated GIF") + " (*.gif)",
                      qsTr("MKV Video") + " (*.mkv)",
                      qsTr("Animated PNG") + " (*.mng)",
                      qsTr("QuickTime Video") + " (*.mov)",
                      qsTr("MP4 Video") + " (*.mp4 *.m4v)",
                      qsTr("MPEG Video") + " (*.mpg *.mpeg)",
                      qsTr("Ogg Video") + " (*.ogg)",
                      //: Don't translate "RealMedia", leave it as is.
                      qsTr("RealMedia Video") + " (*.rm)",
                      qsTr("DVD Video") + " (*.vob)",
                      qsTr("WebM Video") + " (*.webm)",
                      //: Also known as WMV, is a video file format.
                      qsTr("Windows Media Video") + " (*.wmv)",
                      qsTr("All Files") + " (*)"]

        onAccepted: {
            filePath.text = mediaTools.urlToLocalFile(fileDialog.file)
            urlPath.text = ""
            fileDescription.text =
                    addSource.defaultDescription(fileDialog.file.toString())
            urlDescription.text = fileDescription.text
        }
    }
}
