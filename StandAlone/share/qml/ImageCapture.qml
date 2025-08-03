/* Webcamoid, camera capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as LABS
import Ak

Page {
    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: layout.height
        clip: true

        readonly property string filePrefix: Ak.platform() == "windows"?
                                                 "file:///":
                                                 "file://"

        GridLayout {
            id: layout
            width: scrollView.width
            columns: 3

            property bool isPathCustomizable: Ak.platform() != "android"

            Label {
                id: txtImagesDirectory
                text: qsTr("Images directory")
                visible: layout.isPathCustomizable
                height: layout.isPathCustomizable? 0: undefined
            }
            TextField {
                text: recording.imagesDirectory
                Accessible.name: txtImagesDirectory.text
                selectByMouse: true
                Layout.fillWidth: true
                visible: layout.isPathCustomizable
                height: layout.isPathCustomizable? 0: undefined

                onTextChanged: recording.imagesDirectory = text
            }
            Button {
                text: qsTr("Search")
                Accessible.description: qsTr("Search directory to save images")
                visible: layout.isPathCustomizable
                height: layout.isPathCustomizable? 0: undefined

                onClicked: {
                    mediaTools.makedirs(recording.imagesDirectory)
                    folderDialog.open()
                }
            }
            Label {
                id: txtFileFormat
                text: qsTr("File format")
            }
            ComboBox {
                Accessible.description: txtFileFormat.text
                textRole: "description"
                Layout.fillWidth: true
                Layout.columnSpan: 2
                model: ListModel {
                }

                Component.onCompleted: {
                    model.clear()

                    for (let i in recording.availableImageFormats) {
                        let fmt = recording.availableImageFormats[i]

                        model.append({
                            format: fmt,
                            description: recording.imageFormatDescription(fmt)
                        })
                    }

                    currentIndex =
                        recording.availableImageFormats.indexOf(recording.imageFormat)
                }
                onCurrentIndexChanged:
                    recording.imageFormat =
                        recording.availableImageFormats[currentIndex]
            }
            Label {
                id: txtQuality
                text: qsTr("Quality")
            }
            Slider {
                id: sldQuality
                from: spbQuality.from
                to: spbQuality.to
                value: recording.imageSaveQuality
                stepSize: spbQuality.stepSize
                Layout.fillWidth: true
                Accessible.name: txtQuality.text

                onValueChanged: recording.imageSaveQuality = value
            }
            SpinBox {
                id: spbQuality
                from: -1
                to: 100
                value: recording.imageSaveQuality
                stepSize: 1
                Accessible.name: txtQuality.text

                onValueChanged: recording.imageSaveQuality = value
            }
        }
    }
    LABS.FolderDialog {
        id: folderDialog
        title: qsTr("Select the folder to save your photos")
        folder: scrollView.filePrefix + recording.imagesDirectory

        onAccepted: {
            recording.imagesDirectory = mediaTools.urlToLocalFile(currentFolder)
        }
    }
}
