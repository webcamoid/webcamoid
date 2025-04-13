/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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
import Qt.labs.settings 1.0
import Ak

Page {
    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: pathsConfigs.height
        clip: true

        readonly property string filePrefix: Ak.platform() == "windows"?
                                                 "file:///":
                                                 "file://"

        Component.onCompleted: {
            debugLog.text = mediaTools.log
        }

        Connections {
            target: mediaTools

            function onLogUpdated(messageType, lastLine)
            {
                debugLog.text += lastLine + "\n"
            }
        }

        GridLayout {
            id: pathsConfigs
            columns: 3
            width: scrollView.width

            Label {
                id: txtFilesDirectory
                text: qsTr("Logs directory")
            }
            TextField {
                text: mediaTools.documentsDirectory
                Accessible.name: txtFilesDirectory.text
                selectByMouse: true
                Layout.fillWidth: true

                onTextChanged: mediaTools.documentsDirectory = text
            }
            Button {
                text: qsTr("Search")
                Accessible.description: qsTr("Search directory to save logs")

                onClicked: {
                    mediaTools.makedirs(mediaTools.documentsDirectory)
                    folderDialog.open()
                }
            }
            RowLayout {
                Layout.columnSpan: 3
                Layout.fillWidth: true

                Button {
                    text: qsTr("Clear")
                    Accessible.description: qsTr("Clear the debug log")
                    icon.source: "image://icons/reset"
                    flat: true

                    onClicked: debugLog.clear()
                }
                Button {
                    text: qsTr("Save")
                    Accessible.description: qsTr("Save the debug log")
                    icon.source: "image://icons/save"
                    flat: true

                    onClicked: {
                        mediaTools.saveLog()
                        logSavedDialog.open()
                    }
                }
            }
            TextArea {
                id: debugLog
                wrapMode: Text.WordWrap
                readOnly: true
                Layout.columnSpan: 3
                Layout.fillWidth: true
            }
        }
    }

    LogSavedDialog {
        id: logSavedDialog
        anchors.centerIn: Overlay.overlay
    }
    LABS.FolderDialog {
        id: folderDialog
        title: qsTr("Select the folder to save the logs")
        folder: scrollView.filePrefix + mediaTools.documentsDirectory

        onAccepted: {
            mediaTools.documentsDirectory =
                mediaTools.urlToLocalFile(currentFolder)
        }
    }
}
