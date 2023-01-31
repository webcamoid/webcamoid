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
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import Qt.labs.platform 1.1 as LABS
import Ak 1.0

Dialog {
    id: addEdit
    standardButtons: Dialog.Ok | Dialog.Cancel
    width: AkUnit.create(420 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(320 * AkTheme.controlScale, "dp").pixels
    modal: true

    property bool editMode: false

    signal edited()

    onVisibleChanged: tabBar.currentItem.forceActiveFocus()

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

    function openOptions(device)
    {
        title = device?
                    qsTr("Edit Source"):
                    qsTr("Add Source")
        addEdit.editMode = device != ""
        fileDescription.text = videoLayer.description(device)
        urlDescription.text = fileDescription.text
        filePath.text = ""
        urlPath.text = ""
        tabBar.currentIndex = 0

        if (device) {
            if (addEdit.isFile(device)) {
                filePath.text = device
            } else {
                urlPath.text = device
                tabBar.currentIndex = 1
            }
        }

        open()
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
                        id: txtDescriptionFile
                        text: qsTr("Description")
                        font.bold: true
                        Layout.fillWidth: true
                    }
                    TextField {
                        id: fileDescription
                        placeholderText: qsTr("Source title")
                        Accessible.name: txtDescriptionFile.text
                        text: addEdit.editMode?
                                  videoLayer.description(videoLayer.videoInput):
                                  ""
                        selectByMouse: true
                        Layout.fillWidth: true
                    }
                    Label {
                        id: txtPath
                        text: qsTr("Path")
                        font.bold: true
                        Layout.fillWidth: true
                    }
                    RowLayout {
                        TextField {
                            id: filePath
                            placeholderText: qsTr("File path")
                            Accessible.name: txtPath.text
                            text: addEdit.editMode? videoLayer.videoInput: ""
                            selectByMouse: true
                            Layout.fillWidth: true
                        }

                        Button {
                            text: qsTr("Search")
                            Accessible.description: qsTr("Search file to use as source")
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
                        id: txtDescriptionUrl
                        text: qsTr("Description")
                        font.bold: true
                        Layout.fillWidth: true
                    }
                    TextField {
                        id: urlDescription
                        placeholderText: qsTr("Source title")
                        Accessible.name: txtDescriptionUrl.text
                        text: addEdit.editMode?
                                  videoLayer.description(videoLayer.videoInput):
                                  ""
                        selectByMouse: true
                        Layout.fillWidth: true
                    }
                    Label {
                        id: txtUrl
                        text: qsTr("URL")
                        font.bold: true
                        Layout.fillWidth: true
                    }
                    TextField {
                        id: urlPath
                        placeholderText: "https://example-site.com/video.webm"
                        Accessible.name: txtUrl.text
                        text: addEdit.editMode? videoLayer.videoInput: ""
                        selectByMouse: true
                        Layout.fillWidth: true
                    }
                }
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
                description = addEdit.defaultDescription(uri)

            if (editMode)
                videoLayer.removeInputStream(videoLayer.videoInput)

            videoLayer.setInputStream(uri, description)
            videoLayer.videoInput = uri

            if (editMode)
                addEdit.edited()
        }
    }

    LABS.FileDialog {
        id: fileDialog
        title: qsTr("Choose the file to add as source")
        fileMode: LABS.FileDialog.OpenFile
        selectedNameFilter.index: 0
        nameFilters: videoLayer.videoSourceFileFilters

        onAccepted: {
            filePath.text = mediaTools.urlToLocalFile(fileDialog.file)
            urlPath.text = ""
            fileDescription.text =
                    addEdit.defaultDescription(filePath.text)
            urlDescription.text = fileDescription.text
        }
    }
}
