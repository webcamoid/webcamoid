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
import AkControls as AK

AK.MenuOption {
    id: root
    title: qsTr("Image Capture")
    subtitle: qsTr("Configure photogragy quality and formats.")
    icon: "image://icons/photo"

    property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: layout.height
        clip: true

        readonly property string filePrefix: Ak.platform() == "windows"?
                                                 "file:///":
                                                 "file://"

        ColumnLayout {
            id: layout
            width: scrollView.width

            property bool isPathCustomizable: Ak.platform() != "android"

            Label {
                id: txtImagesDirectory
                text: qsTr("Images directory")
                font.bold: true
                visible: layout.isPathCustomizable
                height: layout.isPathCustomizable? 0: undefined
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
            }
            AK.ActionTextField {
                icon.source: "image://icons/search"
                labelText: recording.imagesDirectory
                placeholderText: txtImagesDirectory.text
                buttonText: qsTr("Select the save directory")
                visible: layout.isPathCustomizable
                height: layout.isPathCustomizable? 0: undefined
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true

                onLabelTextChanged: recording.imagesDirectory = labelText
                onButtonClicked: {
                    mediaTools.makedirs(recording.imagesDirectory)
                    folderDialog.open()
                }
            }
            AK.LabeledComboBox {
                label: qsTr("File format")
                textRole: "description"
                Accessible.description: label
                Layout.fillWidth: true
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
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
                font.bold: true
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
            }
            RowLayout {
                Slider {
                    id: sldQuality
                    from: spbQuality.from
                    to: spbQuality.to
                    value: recording.imageSaveQuality
                    stepSize: spbQuality.stepSize
                    Layout.leftMargin: root.leftMargin
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
                    Layout.rightMargin: root.rightMargin

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
}
