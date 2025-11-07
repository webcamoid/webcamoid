/* Webcamoid, camera capture application.
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
import AkControls as AK

AK.MenuOption {
    id: root
    title: qsTr("Debug Log")
    subtitle: qsTr("Information to help fixing bugs in %1.").arg(mediaTools.applicationName)
    icon: "image://icons/bug"

    property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
    property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: pathsConfigs.height
        clip: true

        property int skipLines: 0
        readonly property string filePrefix: Ak.platform() == "windows"?
                                                 "file:///":
                                                 "file://"

        onVisibleChanged: {
            if (visible)
                debugLog.text = mediaTools.readLog(skipLines)
        }

        Connections {
            target: mediaTools

            function onLogUpdated(messageType, lastLine)
            {
                if (scrollView.visible)
                    debugLog.text += lastLine + "\n"
            }
        }

        GridLayout {
            id: pathsConfigs
            columns: 3
            width: scrollView.width

            property bool isPathCustomizable: Ak.platform() != "android"

            Label {
                id: txtFilesDirectory
                text: qsTr("Logs directory")
                visible: pathsConfigs.isPathCustomizable
                height: pathsConfigs.isPathCustomizable? 0: undefined
                Layout.leftMargin: root.leftMargin
            }
            TextField {
                text: mediaTools.documentsDirectory
                Accessible.name: txtFilesDirectory.text
                selectByMouse: true
                Layout.fillWidth: true
                visible: pathsConfigs.isPathCustomizable
                height: pathsConfigs.isPathCustomizable? 0: undefined

                onTextChanged: mediaTools.documentsDirectory = text
            }
            Button {
                text: qsTr("Search")
                visible: pathsConfigs.isPathCustomizable
                height: pathsConfigs.isPathCustomizable? 0: undefined
                Layout.rightMargin: root.rightMargin
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
                    Layout.leftMargin: root.leftMargin

                    onClicked: {
                        scrollView.skipLines +=
                            debugLog.text.split("\n").length
                        debugLog.clear()
                    }
                }
                Button {
                    text: qsTr("Save")
                    Accessible.description: qsTr("Save the debug log")
                    icon.source: "image://icons/save"
                    flat: true

                    onClicked: {
                        let ok = mediaTools.saveLog()

                        if (ok && pathsConfigs.isPathCustomizable)
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
}
