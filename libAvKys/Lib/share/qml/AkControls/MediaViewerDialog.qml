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
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import Ak

Dialog {
    id: detailDialog
    width: AkUnit.create(400 * AkTheme.controlScale, "dp").pixels
    height: AkUnit.create(300 * AkTheme.controlScale, "dp").pixels
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0
    modal: true

    property bool hideControls: false
    property alias model: rep.model

    signal share(url mediaUrl)

    function openAtIndex(index) {
        // Save the original duration to restore it later.
        var oldDuration = swipeView.contentItem.highlightMoveDuration

        // Deactivate the movement animation
        swipeView.contentItem.highlightMoveDuration = 0

        // Switch to the desired index.
        swipeView.currentIndex = index

        // Restored the original duration so that normal transitions continue to be animated.
        swipeView.contentItem.highlightMoveDuration = oldDuration

        hideControls = false
        swipeView.forceActiveFocus()
        open()
    }

    function openAtUrl(url) {
        if (!rep.model || rep.model.rowCount() === 0)
            return

        const targetUrl = Qt.resolvedUrl(url)

        for (let i = 0; i < rep.model.rowCount(); ++i) {
            const itemUrlRaw = rep.model.data(rep.model.index(i, 0), AkMediaGalleryModel.UrlRole)
            const itemUrl = Qt.resolvedUrl(itemUrlRaw)

            if (itemUrl === targetUrl) {
                openAtIndex(i)

                return
            }
        }
    }

    function formatTime(milliseconds) {
        if (isNaN(milliseconds) || milliseconds <= 0)
            return "00:00"

        let totalSeconds = Math.floor(milliseconds / 1000)
        let hours = Math.floor(totalSeconds / 3600)
        let minutes = Math.floor((totalSeconds % 3600) / 60)
        let seconds = totalSeconds % 60

        let h = hours.toString().padStart(2, "0")
        let m = minutes.toString().padStart(2, "0")
        let s = seconds.toString().padStart(2, "0")

        return hours > 0 ? `${h}:${m}:${s}` : `${m}:${s}`
    }

    header: ToolBar {
        id: toolBar
        implicitHeight: AkUnit.create(64 * AkTheme.controlScale, "dp").pixels
        opacity: detailDialog.hideControls? 0.0: 1.0
        visible: opacity != 0.0

        Behavior on opacity {
            NumberAnimation {
                duration: 300
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: AkUnit.create(8 * AkTheme.controlScale, "dp").pixels

            ToolButton {
                icon.source: "image://icons/left-arrow"
                implicitWidth: implicitHeight

                onClicked: detailDialog.close()
            }

            Item {
                Layout.fillWidth: true
            }

            ToolButton {
                icon.source: "image://icons/share"
                implicitWidth: implicitHeight

                onClicked: rep.model.share(rep.model.urlAt(swipeView.currentIndex))
            }

            ToolButton {
                icon.source: "image://icons/points-menu"
                implicitWidth: implicitHeight

                onClicked: detailMenu.open()

                Menu {
                    id: detailMenu
                    implicitWidth: AkUnit.create(256 * AkTheme.controlScale, "dp").pixels

                    MenuItem {
                        text: qsTr("Use as")

                        onClicked: rep.model.useAs(rep.model.urlAt(swipeView.currentIndex))
                    }
                    MenuItem {
                        text: qsTr("Open with")

                        onClicked: Qt.openUrlExternally(rep.model.urlAt(swipeView.currentIndex))
                    }
                    MenuItem {
                        text: qsTr("Delete")

                        onClicked: {
                            rep.model.deleteSelectedAt(swipeView.currentIndex)
                            detailDialog.close()
                        }
                    }
                    MenuItem {
                        text: qsTr("Move to")

                        onClicked: rep.model.moveTo(rep.model.urlAt(swipeView.currentIndex))
                    }
                    MenuItem {
                        text: qsTr("Copy to")

                        onClicked: rep.model.copyTo(rep.model.urlAt(swipeView.currentIndex))
                    }
                }
            }
        }
    }

    SwipeView {
        id: swipeView
        anchors.fill: parent
        interactive: !isZoomed
        orientation: Qt.Horizontal
        clip: true

        property bool isZoomed: false
        property int zoomedItem: 0

        onCurrentIndexChanged: {
            if (isZoomed) {
                rep.itemAt(zoomedItem).resetZoom()
                isZoomed = false
            }
        }

        Keys.onLeftPressed: decrementCurrentIndex()
        Keys.onRightPressed: incrementCurrentIndex()

        Repeater {
            id: rep

            Item {
                width: swipeView.width
                height: swipeView.height

                BusyIndicator {
                    anchors.centerIn: parent
                    running: (model.type === AkMediaGalleryModel.Type_Picture
                              && detailImage.status === Image.Loading)
                             || (model.type === AkMediaGalleryModel.Type_Movie
                                 && (videoPlayer.playbackState === MediaPlayer.LoadingState
                                     || !videoPlayer.hasVideo))
                }

                // Picture viewer
                Item {
                    id: pictureViewer
                    visible: model.type === AkMediaGalleryModel.Type_Picture
                    anchors.fill: parent

                    function resetZoom()
                    {
                        detailImage.scale = 1.0
                        detailImage.x = 0
                        detailImage.y = 0
                    }

                    AnimatedImage {
                        id: detailImage
                        width: swipeView.width
                        height: swipeView.height
                        source: visible? model.url: ""
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        cache: false

                        // Auto-center when scale <= 1.0 (snap back)
                        Binding {
                            target: detailImage
                            property: "x"
                            value: 0
                            when: detailImage.scale <= 1.0
                        }
                        Binding {
                            target: detailImage
                            property: "y"
                            value: 0
                            when: detailImage.scale <= 1.0
                        }

                        // Smooth animations for x and y when releasing the dragging
                        Behavior on x {
                            NumberAnimation {
                                duration: 250
                                easing.type: Easing.OutExpo
                            }
                        }
                        Behavior on y {
                            NumberAnimation {
                                duration: 250
                                easing.type: Easing.OutExpo
                            }
                        }

                        // Support zooming with the fingers
                        PinchArea {
                            anchors.fill: parent
                            pinch.target: detailImage
                            pinch.minimumScale: 1.0
                            pinch.maximumScale: 5.0
                            pinch.dragAxis: Pinch.XAndYAxis

                            // Panning with a finger when zoom > 1
                            DragHandler {
                                target: detailImage
                                enabled: detailImage.scale > 1.0
                                onGrabChanged: (transition, point) => {
                                    if (transition == PointerDevice.UngrabPassive)
                                        detailImage.clampPosition()
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                scrollGestureEnabled: true
                                onWheel: (wheel) => {
                                    if (wheel.angleDelta.y > 0)
                                        detailImage.scale = Math.min(detailImage.scale + 0.2, 5.0)
                                    else
                                        detailImage.scale = Math.max(detailImage.scale - 0.2, 1.0)

                                    swipeView.isZoomed = detailImage.scale > 1.0
                                    swipeView.zoomedItem = swipeView.currentIndex
                                    detailImage.clampPosition()
                                }
                                onClicked: detailDialog.hideControls = !detailDialog.hideControls
                                onDoubleClicked: {
                                    // Zoom reset + auto-center
                                    detailImage.scale = 1.0
                                    swipeView.isZoomed = false
                                }
                            }

                            // Update the zoom state when pinching and clamping
                            onPinchUpdated: {
                                swipeView.isZoomed = detailImage.scale > 1.0
                                swipeView.zoomedItem = swipeView.currentIndex
                            }
                            onPinchFinished: {
                                swipeView.isZoomed = detailImage.scale > 1.0
                                swipeView.zoomedItem = swipeView.currentIndex
                                detailImage.clampPosition()
                            }
                        }

                        // Limit the pan to visible bounds
                        function clampPosition() {
                            if (detailImage.scale <= 1.0) {
                                detailImage.x = 0
                                detailImage.y = 0

                                return
                            }

                            var scaledWidth = detailImage.paintedWidth * detailImage.scale
                            var scaledHeight = detailImage.paintedHeight * detailImage.scale

                            var extraWidth = Math.max(0, scaledWidth - parent.width)
                            var extraHeight = Math.max(0, scaledHeight - parent.height)

                            detailImage.x = Math.max(-extraWidth / 2, Math.min(extraWidth / 2, detailImage.x))
                            detailImage.y = Math.max(-extraHeight / 2, Math.min(extraHeight / 2, detailImage.y))
                        }
                    }
                }


                // Video viewer
                Item {
                    id: videoViewer
                    visible: model.type === AkMediaGalleryModel.Type_Movie
                    anchors.fill: parent

                    function updateState()
                    {
                        if (index === swipeView.currentIndex)
                            play()
                        else
                            stop()
                    }

                    MediaPlayer {
                        id: videoPlayer
                        source: videoOutput.visible? model.url: ""
                        loops: MediaPlayer.Infinite
                        videoOutput: videoOutput
                        audioOutput: AudioOutput {}

                        onPlaybackStateChanged: {
                            if (playbackState === MediaPlayer.PlayingState) {
                                autoHideTimer.restart()
                            } else {
                                autoHideTimer.stop()
                                detailDialog.hideControls = false
                            }
                        }
                    }

                    VideoOutput {
                        id: videoOutput
                        anchors.fill: parent
                        fillMode: VideoOutput.PreserveAspectFit
                    }

                    // Play/Pause button

                    Item {
                        width: AkUnit.create(100 * AkTheme.controlScale, "dp").pixels
                        height: AkUnit.create(100 * AkTheme.controlScale, "dp").pixels
                        opacity: detailDialog.hideControls? 0.0: 1.0
                        visible: opacity > 0.0
                        anchors.centerIn: parent

                        Behavior on opacity {
                            NumberAnimation {
                                duration: 300
                            }
                        }

                        Rectangle {
                            anchors.fill: parent
                            radius: width / 2
                            color: "#80000000"
                        }

                        Text {
                            anchors.centerIn: parent
                            text: videoPlayer.playbackState === MediaPlayer.PlayingState? "⏸": "▶"
                            color: "white"
                            font.pixelSize: 60
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    MouseArea {
                        anchors.fill: parent

                        onClicked: {
                            if (videoPlayer.playbackState === MediaPlayer.PlayingState)
                                videoPlayer.pause()
                            else
                                videoPlayer.play()
                        }
                    }

                    // Video controls
                    Rectangle {
                        visible: !detailDialog.hideControls
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: AkUnit.create(60 * AkTheme.controlScale, "dp").pixels
                        color: "#80000000"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: AkUnit.create(12 * AkTheme.controlScale, "dp").pixels

                            Label {
                                text: formatTime(videoPlayer.position)
                                color: "white"
                                font.pixelSize: 14
                            }

                            Slider {
                                Layout.fillWidth: true
                                from: 0
                                to: videoPlayer.duration > 0? videoPlayer.duration: 1
                                value: videoPlayer.position

                                onMoved: videoPlayer.position = value
                                onPressedChanged: {
                                    if (videoPlayer.playbackState === MediaPlayer.PlayingState)
                                        autoHideTimer.restart()
                                }
                            }

                            Label {
                                text: formatTime(videoPlayer.duration)
                                color: "white"
                                font.pixelSize: 14
                            }
                        }
                    }

                    // This timer auto-hide the video controls on playback
                    Timer {
                        id: autoHideTimer
                        interval: 4000
                        repeat: false

                        onTriggered: detailDialog.hideControls = true
                    }

                    // Automatic play/stop when switching the media
                    Connections {
                        target: swipeView

                        function onCurrentIndexChanged() {
                            if (index === swipeView.currentIndex)
                                videoPlayer.play()
                            else
                                videoPlayer.stop()
                        }

                        function onVisibleChanged() {
                            if (swipeView.visible && index === swipeView.currentIndex)
                                videoPlayer.play()
                            else
                                videoPlayer.stop()
                        }
                    }
                }
            }
        }
    }
}
