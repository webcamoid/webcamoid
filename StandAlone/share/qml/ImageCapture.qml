/* Webcamoid, webcam capture application.
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

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import Qt.labs.platform 1.1 as LABS
import Ak 1.0

Page {
    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: layout.height
        clip: true

        GridLayout {
            id: layout
            width: scrollView.width
            columns: 3

            Label {
                text: qsTr("Images directory")
            }
            TextField {
                Layout.fillWidth: true
                text: recording.imagesDirectory
                selectByMouse: true

                onTextChanged: recording.imagesDirectory = text
            }
            Button {
                text: qsTr("Search")

                onClicked: {
                    mediaTools.makedirs(recording.imagesDirectory)
                    folderDialog.open()
                }
            }
            Label {
                text: qsTr("File format")
            }
            ComboBox {
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
                text: qsTr("Quality")
            }
            Slider {
                id: sldQuality
                from: spbQuality.from
                to: spbQuality.to
                value: recording.imageSaveQuality
                stepSize: spbQuality.stepSize
                Layout.fillWidth: true

                onValueChanged: recording.imageSaveQuality = value
            }
            SpinBox {
                id: spbQuality
                from: -1
                to: 100
                value: recording.imageSaveQuality
                stepSize: 1

                onValueChanged: recording.imageSaveQuality = value
            }
        }
    }
    LABS.FolderDialog {
        id: folderDialog
        title: qsTr("Select the folder to save your photos")
        folder: "file://" + recording.imagesDirectory

        onAccepted: recording.imagesDirectory =
                    currentFolder.toString().replace("file://", "")
    }
}
