/* Webcamoid, camera capture application.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
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
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia
import Ak

Dialog {
    id: root
    standardButtons: Dialog.Close
    width: 800
    height: 600
    modal: true

    property int previewSize: 200
    property alias location: galleryModel.location

    function formatBytes(bytes)
    {
        if (bytes === 0)
            return "0 Bytes"

        const k = 1024
        const sizes = ["B", "KiB", "MiB", "GiB"]
        const i = Math.floor(Math.log(bytes) / Math.log(k))

        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + " " + sizes[i]
    }

    header: ToolBar {
        visible: galleryModel.selectedCount > 0

        RowLayout {
            anchors.fill: parent
            anchors.margins: 8

            ToolButton {
                text: "✕"
                onClicked: galleryModel.clearSelection()
            }

            Column {
                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: true

                Label {
                    text: qsTr("%1 selected").arg(galleryModel.selectedCount)
                    font.bold: true
                    Layout.fillWidth: true
                }
                Label {
                    text: formatBytes(galleryModel.totalSelectedSize)
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }
            }

            Row {
                spacing: 8

                ToolButton {
                    text: qsTr("Share")
                    onClicked: console.log("Share", galleryModel.selectedCount, "pictures")
                }

                ToolButton {
                    text: qsTr("Delete")
                    onClicked: confirmDialog.open()
                }

                ToolButton {
                    text: "⋮"
                    onClicked: selectionMenu.open()

                    Menu {
                        id: selectionMenu

                        Action {
                            text: qsTr("Select all")
                            enabled: galleryModel.selectedCount < galleryModel.rowCount()
                            onTriggered: galleryModel.selectAll()
                        }

                        Action {
                            text: qsTr("Deselect all")
                            enabled: galleryModel.selectedCount > 0
                            onTriggered: galleryModel.clearSelection()
                        }
                    }
                }
            }
        }
    }

    GridView {
        id: grid
        cellWidth: previewSize
        cellHeight: previewSize
        cacheBuffer: 0
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter

        width: {
            if (parent.width <= 0)
                return 0

            var columnas = Math.floor(parent.width / cellWidth)

            if (columnas === 0)
                return parent.width

            return columnas * cellWidth
        }

        model: AkMediaGalleryModel {
            id: galleryModel
        }

        delegate: Item {
            width: GridView.view.cellWidth
            height: GridView.view.cellHeight

            // Thumbnail container
            Item {
                anchors.fill: parent
                anchors.margins: 1
                clip: true

                // Picture thumbnail
                Image {
                    id: photoPreview
                    visible: galleryModel.location === MediaGalleryModel.Location_Pictures
                    source: visible? model.url: ""
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    cache: false
                    sourceSize.width: parent.width
                    sourceSize.height: parent.height
                    anchors.fill: parent

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: photoPreview.status === Image.Loading
                        visible: running
                    }
                }

                // Video thumbnail
                MediaPlayer {
                    id: videoPlayer
                    source: videoOutput.visible? model.url: ""
                    autoPlay: false
                    videoOutput: videoOutput

                    // Get first video frame
                    Component.onCompleted: {
                        if (galleryModel.location === MediaGalleryModel.Location_Movies) {
                            videoPlayer.play()
                            videoPlayer.pause()

                            // Seek some time forward to get a more
                            // representative frame (10% of the video)
                            if (duration > 0)
                                seek(Math.min(5000, duration * 0.1))
                        }
                    }
                }

                VideoOutput {
                    id: videoOutput
                    anchors.fill: parent
                    visible: galleryModel.location === MediaGalleryModel.Location_Movies
                    fillMode: VideoOutput.PreserveAspectCrop

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: videoPlayer.playbackState === MediaPlayer.LoadingState
                        visible: running
                    }
                }

                // video play icon
                Rectangle {
                    visible: galleryModel.location === MediaGalleryModel.Location_Movies
                    anchors.centerIn: parent
                    width: 60
                    height: 60
                    radius: 30
                    color: "#80000000"

                    Text {
                        anchors.centerIn: parent
                        text: "▶"
                        color: "white"
                        font.pixelSize: 40
                    }
                }
            }

            // selection checkmark
            Rectangle {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 8
                width: 40
                height: 40
                radius: 20
                color: model.selected? "#2196F3": "transparent"
                border.color: "white"
                border.width: 2
                visible: galleryModel.selectedCount > 0

                Text {
                    anchors.centerIn: parent
                    text: "✓"
                    color: "white"
                    font.pixelSize: 28
                    visible: model.selected
                }
            }

            TapHandler {
                onTapped: {
                    if (galleryModel.selectedCount > 0)
                        galleryModel.toggleSelected(index)
                        else
                            mediaViewer.openAt(index)
                }
                onLongPressed: galleryModel.toggleSelected(index)
            }
        }
        Text {
            anchors.centerIn: parent
            text: qsTr("The pictures directory is empty")
            font.pixelSize: 20
            visible: grid.count === 0
        }
    }

    Dialog {
        id: confirmDialog
        title: qsTr("Confirm delete")
        standardButtons: Dialog.Yes | Dialog.No

        ColumnLayout {
            spacing: 12

            Label {
                text: qsTr("Delete %n picture(s) permanently?", "", galleryModel.selectedCount)
                wrapMode: Text.WordWrap
                width: confirmDialog.availableWidth
                Layout.fillWidth: true

            }
            Label {
                text: qsTr("This action can't be undone.")
                font.pixelSize: 12
                Layout.fillWidth: true
            }
        }

        onAccepted: galleryModel.deleteSelected()
    }

    MediaViewerDialog {
        id: mediaViewer
        width: parent.width
        height: parent.height
        model: galleryModel
    }
}
