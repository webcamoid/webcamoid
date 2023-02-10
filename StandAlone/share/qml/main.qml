/* Webcamoid, webcam capture application.
 * Copyright (C) 2015  Gonzalo Exequiel Pedone
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
import QtQuick.Window 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0 as LABS
import Ak 1.0
import AkControls 1.0 as AK
import Webcamoid 1.0

ApplicationWindow {
    id: wdgMainWidget
    title: mediaTools.applicationName
           + " "
           + version()
           + " - "
           + videoLayer.description(videoLayer.videoInput)
    visible: true
    width: mediaTools.windowWidth
    height: mediaTools.windowHeight

    function version()
    {
        if (mediaTools.isDailyBuild) {
            let versionStr = qsTr("Daily Build")

            if (mediaTools.projectGitShortCommit.length > 0)
                versionStr += " (" + mediaTools.projectGitShortCommit + ")"

            return versionStr
        }

        return mediaTools.applicationVersion
    }

    function savePhoto()
    {
        recording.takePhoto()
        recording.savePhoto(qsTr("%1/Picture %2.%3")
                            .arg(recording.imagesDirectory)
                            .arg(mediaTools.currentTime())
                            .arg(recording.imageFormat))
        photoPreviewSaveAnimation.start()
    }

    function snapshotToClipboard()
    {
        var success = false
        snapToClipboard.focus = false
        recording.takePhoto()
        success = recording.copyToClipboard()
        console.debug("Capture snapshot to Clipboard ", success ? "successful" : "failed")
    }

    function pathToUrl(path)
    {
        if (path.length < 1)
            return ""

        return "file://" + path
    }

    function adjustControlScale()
    {
        let physicalWidth = width / Screen.pixelDensity
        let physicalHeight = height / Screen.pixelDensity

        if (physicalWidth <= 100 || physicalHeight <= 100)
            AkTheme.controlScale = 1.0;
        else
            AkTheme.controlScale = 1.6;
    }

    onWidthChanged: {
        adjustControlScale()
        mediaTools.windowWidth = width
    }
    onHeightChanged: {
        adjustControlScale()
        mediaTools.windowHeight = height
    }

    Component.onCompleted: {
        x = (Screen.width - mediaTools.windowWidth) / 2
        y = (Screen.height - mediaTools.windowHeight) / 2
        chkFlash.updateVisibility()
    }

    Connections {
        target: mediaTools

        function onNewInstanceOpened()
        {
            wdgMainWidget.raise();
            wdgMainWidget.requestActivate()
        }
    }

    Connections {
        target: videoLayer

        function onVideoInputChanged()
        {
            chkFlash.updateVisibility()
        }
    }

    VideoDisplay {
        id: videoDisplay
        objectName: "videoDisplay"
        visible: videoLayer.state == AkElement.ElementStatePlaying
        smooth: true
        anchors.fill: parent
    }
    Image {
        id: photoPreviewThumbnail
        source: pathToUrl(recording.lastPhotoPreview)
        sourceSize: Qt.size(width, height)
        cache: false
        smooth: true
        mipmap: true
        fillMode: Image.PreserveAspectFit
        x: k * AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        y: k * (controlsLayout.y
                + (controlsLayout.height - photoPreview.height) / 2)
        width: k * (photoPreview.width - parent.width) + parent.width
        height: k * (photoPreview.height - parent.height) + parent.height
        visible: false

        property real k: 0
    }
    Image {
        id: videoPreviewThumbnail
        source: pathToUrl(recording.lastVideoPreview)
        sourceSize: Qt.size(width, height)
        cache: false
        smooth: true
        mipmap: true
        fillMode: Image.PreserveAspectFit
        x: k * (parent.width
                - videoPreview.width
                - AkUnit.create(16 * AkTheme.controlScale, "dp").pixels)
        y: k * (controlsLayout.y
                + (controlsLayout.height - videoPreview.height) / 2)
        width: k * (videoPreview.width - parent.width) + parent.width
        height: k * (videoPreview.height - parent.height) + parent.height
        visible: false

        property real k: 0
    }
    ColumnLayout {
        id: leftControls
        width: AkUnit.create(childrenRect.width * AkTheme.controlScale, "dp").pixels
        anchors.top: parent.top
        anchors.topMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        anchors.left: parent.left
        anchors.leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        state: cameraControls.state

        Button {
            icon.source: "image://icons/video-effects"
            display: AbstractButton.IconOnly
            flat: true
            Accessible.name: qsTr("Video effects")
            Accessible.description: qsTr("Open video effects panel")

            onClicked: videoEffectsPanel.open()
        }
        Switch {
            id: chkFlash
            text: qsTr("Use flash")
            checked: true
            visible: false
            Accessible.name: text
            Accessible.description: qsTr("Use flash when taking a photo")

            function updateVisibility()
            {
                let modes = videoLayer.supportedFlashModes(videoLayer.videoInput)
                flash.isHardwareFlash = modes.length > 0

                if (cameraControls.state == "Video") {
                    visible = flash.isHardwareFlash
                } else {
                    visible =
                        videoLayer.deviceType(videoLayer.videoInput) == VideoLayer.InputCamera
                }
            }
        }
        ComboBox {
            id: cbxTimeShot
            textRole: "text"
            Layout.fillWidth: true
            visible: chkFlash.visible && cameraControls.state != "Video"
            Accessible.name: qsTr("Photo timer")
            Accessible.description: qsTr("The time to wait before the photo is taken")
            model: ListModel {
                id: lstTimeOptions

                ListElement {
                    text: qsTr("Now")
                    time: 0
                }
            }

            Component.onCompleted: {
                for (var i = 5; i < 35; i += 5)
                    lstTimeOptions.append({text: qsTr("%1 seconds").arg(i),
                                           time: i})
            }
        }
    }

    Button {
        id: snapToClipboard
        icon.source: "image://icons/paperclip"
        display: AbstractButton.IconOnly
        flat: true
        anchors.top: parent.top
        anchors.topMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        anchors.horizontalCenter: parent.horizontalCenter
        Accessible.name: qsTr("Snapshot to Clipboard")
        Accessible.description: qsTr("Captures a snapshot and copies it into the clipboard")
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Capture Snapshot to Clipboard")

        onClicked: snapshotToClipboard()
    }

    Button {
        id: rightControls
        icon.source: "image://icons/settings"
        display: AbstractButton.IconOnly
        flat: true
        anchors.top: parent.top
        anchors.topMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        anchors.right: parent.right
        anchors.rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        Accessible.name: qsTr("Sources and outputs settings")
        Accessible.description: qsTr("Open sources and outputs settings menu")

        onClicked: settings.popup()
    }
    SettingsMenu {
        id: settings
        width: AkUnit.create(250 * AkTheme.controlScale, "dp").pixels

        onOpenAudioSettings: audioVideoPanel.openAudioSettings()
        onOpenVideoSettings: audioVideoPanel.openVideoSettings()
        onOpenSettings: settingsDialog.open()
        onOpenDonationsDialog: Qt.openUrlExternally(mediaTools.projectDonationsUrl)
        onOpenAboutDialog: aboutDialog.open()
    }
    RecordingNotice {
        anchors.top: parent.top
        anchors.topMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        anchors.horizontalCenter: parent.horizontalCenter
        visible: recording.state == AkElement.ElementStatePlaying
    }
    ColumnLayout {
        id: controlsLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        Item {
            id: cameraControls
            Layout.margins:
                AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            height: AkUnit.create(64 * AkTheme.controlScale, "dp").pixels
            Layout.fillWidth: true

            readonly property real smallButton: AkUnit.create(48 * AkTheme.controlScale, "dp").pixels
            readonly property real bigButton: AkUnit.create(64 * AkTheme.controlScale, "dp").pixels
            readonly property real previewSize: AkUnit.create(48 * AkTheme.controlScale, "dp").pixels
            readonly property int animationTime: 200

            AK.ImageButton {
                id: photoPreview
                text: qsTr("Open last photo")
                icon.source: pathToUrl(recording.lastPhotoPreview)
                width: cameraControls.previewSize
                height: cameraControls.previewSize
                fillMode: AkColorizedImage.PreserveAspectCrop
                cache: false
                visible: photoPreview.status == Image.Ready
                y: (parent.height - height) / 2
                ToolTip.visible: hovered
                ToolTip.text: text
                Accessible.name: text
                Accessible.description: qsTr("Open last photo taken")

                onClicked: {
                    if (photoPreview.status == AkColorizedImage.Ready)
                        Qt.openUrlExternally(photoPreview.icon.source)
                }
            }
            RoundButton {
                id: photoButton
                icon.source: "image://icons/photo"
                width: cameraControls.bigButton
                height: cameraControls.bigButton
                x: (parent.width - width) / 2
                y: (parent.height - height) / 2
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Take a photo")
                Accessible.name:
                    cameraControls.state == ""?
                        qsTr("Take a photo"):
                        qsTr("Image capture mode")
                Accessible.description:
                    cameraControls.state == ""?
                        qsTr("Make a capture and save it to an image file"):
                        qsTr("Put %1 in image capture mode").arg(mediaTools.applicationName)
                focus: true
                enabled: recording.state == AkElement.ElementStateNull
                         && (videoLayer.state == AkElement.ElementStatePlaying
                             || cameraControls.state == "Video")

                onClicked: {
                    if (cameraControls.state == "Video") {
                        cameraControls.state = ""
                        chkFlash.updateVisibility()
                    } else {
                        if (!chkFlash.visible) {
                            savePhoto()

                            return
                        }

                        if (cbxTimeShot.currentIndex == 0) {
                            if (chkFlash.checked)
                                flash.shot()
                            else
                                savePhoto()

                            return
                        }

                        if (updateProgress.running) {
                            updateProgress.stop()
                            pgbPhotoShot.value = 0
                            cbxTimeShot.enabled = true
                            chkFlash.enabled = true
                        } else {
                            cbxTimeShot.enabled = false
                            chkFlash.enabled = false
                            pgbPhotoShot.start = new Date().getTime()
                            updateProgress.start()
                        }
                    }
                }
            }
            RoundButton {
                id: videoButton
                icon.source: recording.state == AkElement.ElementStateNull?
                                 "image://icons/video":
                                 "image://icons/record-stop"
                width: cameraControls.smallButton
                height: cameraControls.smallButton
                x: parent.width - width
                y: (parent.height - height) / 2
                ToolTip.visible: hovered
                ToolTip.text: recording.state == AkElement.ElementStateNull?
                                  qsTr("Record video"):
                                  qsTr("Stop video recording")
                Accessible.name:
                    cameraControls.state == ""?
                        qsTr("Video capture mode"):
                    recording.state == AkElement.ElementStateNull?
                        qsTr("Record video"):
                        qsTr("Stop video recording")
                Accessible.description:
                    cameraControls.state == ""?
                        qsTr("Put %1 in video recording mode").arg(mediaTools.applicationName):
                    recording.state == AkElement.ElementStateNull?
                        qsTr("Start recording to a video file"):
                        qsTr("Stop current video recording")
                enabled: videoLayer.state == AkElement.ElementStatePlaying
                         || cameraControls.state == ""

                onClicked: {
                    if (cameraControls.state == "") {
                        cameraControls.state = "Video"
                        chkFlash.updateVisibility()
                    } else if (recording.state == AkElement.ElementStateNull) {
                        recording.state = AkElement.ElementStatePlaying
                    } else {
                        recording.state = AkElement.ElementStateNull
                        videoPreviewSaveAnimation.start()
                    }
                }
            }
            AK.ImageButton {
                id: videoPreview
                text: qsTr("Open last video")
                icon.source: pathToUrl(recording.lastVideoPreview)
                width: 0
                height: 0
                fillMode: AkColorizedImage.PreserveAspectCrop
                cache: false
                visible: false
                x: parent.width - width
                y: (parent.height - height) / 2
                ToolTip.visible: hovered
                ToolTip.text: text
                Accessible.name: text
                Accessible.description: qsTr("Open last recorded video")

                onClicked: {
                    if (videoPreview.status == Image.Ready)
                        Qt.openUrlExternally("file://" + recording.lastVideo)
                }
            }

            states: [
                State {
                    name: "Video"

                    PropertyChanges {
                        target: photoPreview
                        width: 0
                        height: 0
                        visible: false
                    }
                    PropertyChanges {
                        target: photoButton
                        width: cameraControls.smallButton
                        height: cameraControls.smallButton
                        x: 0
                    }
                    PropertyChanges {
                        target: videoButton
                        width: cameraControls.bigButton
                        height: cameraControls.bigButton
                        x: (parent.width - width) / 2
                    }
                    PropertyChanges {
                        target: videoPreview
                        width: cameraControls.previewSize
                        height: cameraControls.previewSize
                        visible: true
                    }
                }
            ]

            transitions: Transition {
                PropertyAnimation {
                    target: photoPreview
                    properties: "width,height,visible"
                    duration: cameraControls.animationTime
                }
                PropertyAnimation {
                    target: photoButton
                    properties: "radius,x"
                    duration: cameraControls.animationTime
                }
                PropertyAnimation {
                    target: videoButton
                    properties: "radius,x"
                    duration: cameraControls.animationTime
                }
                PropertyAnimation {
                    target: videoPreview
                    properties: "width,height,visible"
                    duration: cameraControls.animationTime
                }
            }
        }
        ProgressBar {
            id: pgbPhotoShot
            Layout.fillWidth: true
            visible: updateProgress.running

            property double start: 0

            onValueChanged: {
                if (value >= 1) {
                    updateProgress.stop()
                    value = 0
                    cbxTimeShot.enabled = true
                    chkFlash.enabled = true

                    if (chkFlash.checked)
                        flash.shot()
                    else
                        savePhoto()
                }
            }
        }
    }
    VideoEffectsPanel {
        id: videoEffectsPanel

        onOpenVideoEffectsDialog: videoEffectsDialog.open()
    }
    AudioVideoPanel {
        id: audioVideoPanel

        onOpenErrorDialog: videoOutputError.openError(title, message)
    }

    SequentialAnimation {
        id: photoPreviewSaveAnimation

        PropertyAnimation {
                   target: photoPreviewThumbnail
                   property: "k"
                   to: 0
                   duration: 0
        }
        PropertyAnimation {
            target: photoPreview
            property: "visible"
            to: false
            duration: 0
        }
        PropertyAnimation {
            target: photoPreviewThumbnail
            property: "visible"
            to: true
            duration: 0
        }
        PropertyAnimation {
            target: photoPreviewThumbnail
            property: "k"
            to: 1
            duration: 500
        }
        PropertyAnimation {
            target: photoPreviewThumbnail
            property: "visible"
            to: false
            duration: 0
        }
        PropertyAnimation {
            target: photoPreview
            property: "visible"
            to: true
            duration: 0
        }
    }
    SequentialAnimation {
        id: videoPreviewSaveAnimation

        PropertyAnimation {
            target: videoPreviewThumbnail
            property: "k"
            to: 0
            duration: 0
        }
        PropertyAnimation {
            target: videoPreview
            property: "visible"
            to: false
            duration: 0
        }
        PropertyAnimation {
            target: videoPreviewThumbnail
            property: "visible"
            to: true
            duration: 0
        }
        PropertyAnimation {
            target: videoPreviewThumbnail
            property: "k"
            to: 1
            duration: 500
        }
        PropertyAnimation {
            target: videoPreviewThumbnail
            property: "visible"
            to: false
            duration: 0
        }
        PropertyAnimation {
            target: videoPreview
            property: "visible"
            to: true
            duration: 0
        }
    }
    Timer {
        id: updateProgress
        interval: 100
        repeat: true

        onTriggered: {
            var timeout = 1000 * lstTimeOptions.get(cbxTimeShot.currentIndex).time
            pgbPhotoShot.value = (new Date().getTime() - pgbPhotoShot.start) / timeout
        }
    }
    Flash {
        id: flash

        onShotStarted: {
            if (isHardwareFlash)
                videoLayer.flashMode = VideoLayer.FlashMode_Torch
        }
        onTriggered: savePhoto()
        onShotFinished: {
            if (isHardwareFlash)
                videoLayer.flashMode = VideoLayer.FlashMode_Off
        }
    }
    VideoEffectsDialog {
        id: videoEffectsDialog
        width: parent.width
        height: parent.height
    }
    SettingsDialog {
        id: settingsDialog
        width: parent.width
        height: parent.height
    }
    VideoOutputError {
        id: videoOutputError

        anchors.centerIn: Overlay.overlay
    }
    UpdatesDialog {
        id: updatesDialog

        anchors.centerIn: Overlay.overlay
    }
    AboutDialog {
        id: aboutDialog

        anchors.centerIn: Overlay.overlay
    }
    LABS.Settings {
        category: "GeneralConfigs"

        property alias useFlash: chkFlash.checked
        property alias photoTimeout: cbxTimeShot.currentIndex
    }
}
