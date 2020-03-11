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
                text: qsTr("Videos directory")
            }
            TextField {
                Layout.fillWidth: true
                text: Recording.videoDirectory
                selectByMouse: true

                onTextChanged: Recording.videoDirectory = text
            }
            Button {
                text: qsTr("Search")

                onClicked: {
                    Webcamoid.makedirs(Recording.videoDirectory)
                    folderDialog.open()
                }
            }
            Label {
                text: qsTr("Record audio")
            }
            Switch {
                Layout.fillWidth: true
                Layout.columnSpan: 2
                LayoutMirroring.enabled: true
                LayoutMirroring.childrenInherit: true
            }
            Label {
                text: qsTr("File format")
            }
            ComboBox {
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Configure")
                flat: true
            }
            Label {
                text: qsTr("Video codec")
            }
            ComboBox {
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Configure")
                flat: true
            }
            Label {
                text: qsTr("Audio codec")
            }
            ComboBox {
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Configure")
                flat: true
            }
        }
    }
    LABS.FolderDialog {
        id: folderDialog
        title: qsTr("Select the folder to save your videos")
        folder: "file://" + Recording.videoDirectory

        onAccepted: Recording.videoDirectory =
                    currentFolder.toString().replace("file://", "")
    }
}
