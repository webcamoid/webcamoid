/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import Qt.labs.platform as LABS
import Ak
import AkControls as AK
import Webcamoid

Dialog {
    id: root
    title: qsTr("ONVIF local server advanced options")
    standardButtons: Dialog.Close
    width: physicalWidth <= 100 || physicalHeight <= 100?
               wdgMainWidget.width: wdgMainWidget.width * 0.5
    height: physicalWidth <= 100 || physicalHeight <= 100?
                wdgMainWidget.height: wdgMainWidget.height * 0.5
    modal: true

    property real physicalWidth: wdgMainWidget.width / Screen.pixelDensity
    property real physicalHeight: wdgMainWidget.height / Screen.pixelDensity
    readonly property string filePrefix: Ak.platform() == "windows"?
                                            "file:///":
                                            "file://"

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: dialogLayout.height
        clip: true

        property int leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        property int rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels

        ColumnLayout {
            id: dialogLayout
            width: scrollView.width

            // The bitrates, the keyframes interval and the sampling rate are
            // shared with the other outputs and are configured in the
            // encoding options.

            // Placeholder: what the clients see and hear while the program
            // is not sending video or audio.

            Label {
                text: qsTr("Placeholder")
                font: AkTheme.fontSettings.h6
                Layout.leftMargin: scrollView.leftMargin
                Layout.rightMargin: scrollView.rightMargin
                Layout.topMargin: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels
                Layout.bottomMargin: AkUnit.create(4 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("Image shown while there is no video. If it is empty, random noise is shown. The audio is silence.")
                wrapMode: Text.WordWrap
                Layout.leftMargin: scrollView.leftMargin
                Layout.rightMargin: scrollView.rightMargin
                Layout.fillWidth: true
            }
            RowLayout {
                Layout.leftMargin: scrollView.leftMargin
                Layout.rightMargin: scrollView.rightMargin
                Layout.bottomMargin: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels
                Layout.fillWidth: true

                TextField {
                    id: placeholderField
                    text: localStreaming.placeholderImage
                    placeholderText: qsTr("Image file")
                    selectByMouse: true
                    Layout.fillWidth: true
                    Accessible.name: qsTr("Placeholder image")

                    onEditingFinished: {
                        localStreaming.placeholderImage = text
                        text = localStreaming.placeholderImage
                    }

                    Connections {
                        target: localStreaming

                        function onPlaceholderImageChanged(placeholderImage)
                        {
                            placeholderField.text = placeholderImage
                        }
                    }
                }
                Button {
                    text: qsTr("Browse")
                    icon.source: "image://icons/search"
                    flat: true
                    display: AbstractButton.IconOnly
                    enabled: localStreaming.location !== ""
                    implicitWidth: implicitHeight
                    ToolTip.visible: hovered
                    ToolTip.text: help
                    Accessible.name: text
                    Accessible.description: help

                    readonly property string help:
                        qsTr("Search an image to be used as placeholder")

                    onClicked: placeholderDialog.open()
                }
                Button {
                    text: qsTr("Clear")
                    icon.source: "image://icons/no"
                    flat: true
                    enabled: localStreaming.placeholderImage !== ""
                    display: AbstractButton.IconOnly
                    implicitWidth: implicitHeight
                    ToolTip.visible: hovered
                    ToolTip.text: help
                    Accessible.name: text
                    Accessible.description: help

                    readonly property string help:
                        qsTr("Remove the placeholder image")

                    onClicked: localStreaming.placeholderImage = ""
                }
            }
        }
    }

    LABS.FileDialog {
        id: placeholderDialog
        title: qsTr("Select the placeholder image")
        nameFilters: [
            qsTr("Image files (*.png *.jpg *.jpeg *.bmp *.gif *.webp)"),
            qsTr("All files (*)")
        ]
        folder: root.filePrefix + recording.imagesDirectory

        onAccepted: {
            localStreaming.placeholderImage =
                String(file).replace(root.filePrefix, "")
        }
    }
}
