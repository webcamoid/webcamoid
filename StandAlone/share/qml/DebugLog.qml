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

    readonly property bool rtl: Qt.application.layoutDirection === Qt.RightToLeft

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: layout.height
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

        ColumnLayout {
            id: layout
            width: scrollView.width
            layoutDirection: root.rtl? Qt.RightToLeft: Qt.LeftToRight

            property bool isPathCustomizable: Ak.platform() != "android"

            Label {
                id: txtFilesDirectory
                text: qsTr("Logs directory")
                font.bold: true
                visible: layout.isPathCustomizable
                height: layout.isPathCustomizable? 0: undefined
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true
            }
            AK.ActionTextField {
                icon.source: "image://icons/search"
                labelText: mediaTools.documentsDirectory
                placeholderText: txtFilesDirectory.text
                buttonText: qsTr("Search directory to save logs")
                visible: layout.isPathCustomizable
                height: layout.isPathCustomizable? 0: undefined
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin
                Layout.fillWidth: true

                onLabelTextChanged: mediaTools.documentsDirectory = labelText
                onButtonClicked: {
                    mediaTools.makedirs(mediaTools.documentsDirectory)
                    folderDialog.open()
                }
            }
            Button {
                text: qsTr("Clear")
                icon.source: "image://icons/reset"
                flat: true
                Accessible.description: qsTr("Clear the debug log")
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin

                onClicked: {
                    scrollView.skipLines +=
                        debugLog.text.split("\n").length
                    debugLog.clear()
                }
            }
            Button {
                text: qsTr("Save")
                icon.source: "image://icons/save"
                flat: true
                Accessible.description: qsTr("Save the debug log")
                Layout.leftMargin: root.leftMargin
                Layout.rightMargin: root.rightMargin

                onClicked: {
                    let ok = mediaTools.saveLog()

                    if (ok && layout.isPathCustomizable)
                        logSavedDialog.open()
                }
            }
            TextArea {
                id: debugLog
                wrapMode: Text.WordWrap
                readOnly: true
                horizontalAlignment: Text.AlignLeft
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
                    mediaTools.urlToLocalFolder(currentFolder)
            }
        }
    }
}
