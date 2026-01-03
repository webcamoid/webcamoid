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
    width: AkUnit.create(800 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(300 * AkTheme.controlScale, "dp").pixels
    modal: true

    property int previewSize: AkUnit.create(200 * AkTheme.controlScale, "dp").pixels
    property alias directory: galleryModel.directory
    property alias model: grid.model

    signal openMedia(url mediaUrl)

    function openAtUrl(url)
    {
        open()
        openMedia(url)
    }

    function formatBytes(bytes)
    {
        if (bytes === 0)
            return "0 Bytes"

        const k = 1024
        const sizes = ["B", "KiB", "MiB", "GiB"]
        const i = Math.floor(Math.log(bytes) / Math.log(k))

        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + " " + sizes[i]
    }

    onClosed: grid.model.clearSelection()

    header: ToolBar {
        implicitHeight: AkUnit.create(64 * AkTheme.controlScale, "dp").pixels
        visible: grid.model.selectedCount > 0

        RowLayout {
            anchors.fill: parent
            anchors.margins: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels

            ToolButton {
                text: "✕"
                onClicked: grid.model.clearSelection()
            }

            Column {
                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: true

                Label {
                    text: qsTr("%1 selected").arg(grid.model.selectedCount)
                    font.bold: true
                    color: AkTheme.palette.active.highlightedText
                    Layout.fillWidth: true
                }
                Label {
                    text: formatBytes(grid.model.totalSelectedSize)
                    font: AkTheme.fontSettings.subtitle1
                    color: AkTheme.palette.active.highlightedText
                    Layout.fillWidth: true
                }
            }

            Row {
                spacing: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels

                ToolButton {
                    text: qsTr("Share")
                    onClicked: {
                        grid.model.share(grid.model.selectedUrls)
                        grid.model.clearSelection()
                    }
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

                        MenuItem {
                            text: qsTr("Select all")
                            enabled: grid.model.selectedCount < grid.model.rowCount()

                            onClicked: grid.model.selectAll()
                        }

                        MenuItem {
                            text: qsTr("Deselect all")
                            enabled: grid.model.selectedCount > 0

                            onClicked: grid.model.clearSelection()
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
        clip: true

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
                anchors.margins: AkUnit.create(1 * AkTheme.controlScale, "dp").pixels
                clip: true

                // Picture thumbnail
                Image {
                    id: photoPreview
                    visible: model.type === AkMediaGalleryModel.Type_Picture
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
                    videoOutput: videoOutput

                    // Get first video frame
                    Component.onCompleted: {
                        if (model.type === AkMediaGalleryModel.Type_Movie) {
                            play()
                            pause()

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
                    visible: model.type === AkMediaGalleryModel.Type_Movie
                    fillMode: VideoOutput.PreserveAspectCrop

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: videoPlayer.playbackState === MediaPlayer.LoadingState
                        visible: running
                    }
                }

                // video play icon
                Rectangle {
                    visible: model.type === AkMediaGalleryModel.Type_Movie
                    anchors.centerIn: parent
                    width: AkUnit.create(60 * AkTheme.controlScale, "dp").pixels
                    height: AkUnit.create(60 * AkTheme.controlScale, "dp").pixels
                    radius: AkUnit.create(30 * AkTheme.controlScale, "dp").pixels
                    color: "#80000000"

                    Text {
                        anchors.centerIn: parent
                        text: "▶"
                        color: "white"
                        font.pixelSize: AkUnit.create(40 * AkTheme.controlScale, "dp").pixels
                    }
                }
            }

            // selection checkmark
            Rectangle {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 8
                width: AkUnit.create(40 * AkTheme.controlScale, "dp").pixels
                height: AkUnit.create(40 * AkTheme.controlScale, "dp").pixels
                radius: AkUnit.create(20 * AkTheme.controlScale, "dp").pixels
                color: model.selected? "#2196F3": "transparent"
                border.color: "white"
                border.width: AkUnit.create(2 * AkTheme.controlScale, "dp").pixels
                visible: grid.model.selectedCount > 0

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
                    if (grid.model.selectedCount > 0)
                        grid.model.toggleSelected(index)
                    else
                        root.openMedia(model.url)
                }
                onLongPressed: grid.model.toggleSelected(index)
            }
        }
        Label {
            anchors.centerIn: parent
            text: qsTr("The directory is empty")
            font.pixelSize: 20
            visible: grid.count === 0
        }
    }

    Dialog {
        id: confirmDialog
        title: qsTr("Confirm delete")
        standardButtons: Dialog.Yes | Dialog.No
        anchors.centerIn: Overlay.overlay

        ColumnLayout {
            spacing: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels

            Label {
                text: qsTr("Delete %n picture(s) permanently?", "", grid.model.selectedCount)
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

        onAccepted: grid.model.deleteSelected()
    }
}
