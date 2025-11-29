/* Webcamoid, camera capture application.
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

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import Ak
import AkControls as AK
import Webcamoid

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

    readonly property string filePrefix: Ak.platform() == "windows"?
                                             "file:///":
                                             "file://"

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
        recording.takePhoto()

        if (recording.copyToClipboard())
            console.debug("Capture snapshot to Clipboard successful")
        else
            console.debug("Capture snapshot to Clipboard failed")
    }

    function pathToUrl(path)
    {
        if (path.length < 1)
            return ""

        return wdgMainWidget.filePrefix + path
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
        if (Ak.platform() != "android") {
            x = (Screen.width - mediaTools.windowWidth) / 2
            y = (Screen.height - mediaTools.windowHeight) / 2
        }
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

        function onVideoInputChanged(videoInput)
        {
            if (recording.state == AkElement.ElementStatePlaying
                && captureSettingsDialog.useFlash
                && flash.isHardwareFlash
                && videoLayer.deviceType(videoInput) == VideoLayer.InputCamera) {
                videoLayer.torchMode = VideoLayer.Torch_On
            }
        }

        function onVcamCliInstallStarted()
        {
            runCommandDialog.start()
            runCommandDialog.open()
        }

        function onVcamCliInstallLineReady(line)
        {
            runCommandDialog.writeLine(line)
        }

        function onVcamCliInstallFinished()
        {
            runCommandDialog.stop()
        }
    }

    HoverHandler {
        id: hoverHandler
        acceptedDevices: PointerDevice.Mouse
    }

    footer: Label {
        height: mediaTools.adBannerHeight
        clip: true
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
        y: k * (mainLayout.y
                + (mainLayout.height - photoPreview.height) / 2)
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
        y: k * (mainLayout.y
                + (mainLayout.height - videoPreview.height) / 2)
        width: k * (videoPreview.width - parent.width) + parent.width
        height: k * (videoPreview.height - parent.height) + parent.height
        visible: false

        property real k: 0
    }
    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        opacity: hoverHandler.hovered || !mediaTools.hideControlsOnPointerOut || Ak.platform() == "android"? 1: 0
        visible: opacity > 0

        readonly property real smallButton: AkUnit.create(48 * AkTheme.controlScale, "dp").pixels
        readonly property real bigButton: AkUnit.create(64 * AkTheme.controlScale, "dp").pixels
        readonly property real previewSize: AkUnit.create(48 * AkTheme.controlScale, "dp").pixels
        readonly property int animationTime: 200

        Behavior on opacity {
            NumberAnimation {
                duration: 250
                easing.type: Easing.InOutQuad
            }
        }

        RowLayout {
            id: topControls
            Layout.topMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.fillWidth: true

            Button {
                id: leftControls
                icon.source: "image://icons/menu"
                text: qsTr("Main menu")
                display: AbstractButton.IconOnly
                implicitWidth: implicitHeight
                ToolTip.visible: hovered
                ToolTip.text: text
                Accessible.name: text
                Accessible.description: qsTr("Open main menu")

                onClicked: settings.popup()
            }
            Item {
                Layout.fillWidth: true
            }
            RecordingNotice {
                visible: recording.state == AkElement.ElementStatePlaying
            }
            Item {
                Layout.fillWidth: true
            }
            Button {
                id: rightControls
                icon.source: "image://icons/settings"
                text: qsTr("Capture options")
                display: AbstractButton.IconOnly
                implicitWidth: implicitHeight
                ToolTip.visible: hovered
                ToolTip.text: text
                Accessible.name: text
                Accessible.description: qsTr("Open capture options menu")
                enabled: videoLayer.state == AkElement.ElementStatePlaying

                onClicked: localSettings.popup()
            }
        }
        Item {
            Layout.fillHeight: true
        }
        Item {
            id: bottomControls
            height: AkUnit.create(64 * AkTheme.controlScale, "dp").pixels
            Layout.bottomMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.rightMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
            Layout.fillWidth: true

            AK.ImageButton {
                id: photoPreview
                text: qsTr("Open last photo")
                icon.source: pathToUrl(recording.lastPhotoPreview)
                width: mainLayout.previewSize
                height: mainLayout.previewSize
                fillMode: AkColorizedImage.PreserveAspectCrop
                cache: false
                visible: photoPreview.status == Image.Ready
                y: (parent.height - height) / 2
                ToolTip.visible: hovered
                ToolTip.text: text
                Accessible.name: text
                Accessible.description: qsTr("Open last photo taken")

                onClicked: {
                    if (photoPreview.status == Image.Ready) {
                        if (recording.latestPhotoUri.length > 1) {
                            Qt.openUrlExternally(recording.latestPhotoUri)
                        } else {
                            let url = "" + photoPreview.icon.source

                            if (!url.startsWith(wdgMainWidget.filePrefix))
                                url = wdgMainWidget.filePrefix + url

                            Qt.openUrlExternally(url)
                        }
                    }
                }
            }
            RoundButton {
                id: photoButton
                icon.source: "image://icons/photo"
                width: mainLayout.bigButton
                height: mainLayout.bigButton
                x: (parent.width - width) / 2
                y: (parent.height - height) / 2
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Take a photo")
                Accessible.name:
                    mainLayout.state == ""?
                        qsTr("Take a photo"):
                        qsTr("Image capture mode")
                Accessible.description:
                    mainLayout.state == ""?
                        qsTr("Make a capture and save it to an image file"):
                        qsTr("Put %1 in image capture mode").arg(mediaTools.applicationName)
                focus: true
                enabled: recording.state == AkElement.ElementStateNull
                         && (videoLayer.state == AkElement.ElementStatePlaying
                             || mainLayout.state == "Video")

                onClicked: {
                    if (mainLayout.state == "Video") {
                        mainLayout.state = ""
                    } else {
                        mediaTools.showAd(MediaTools.AdType_Interstitial);

                        if (!captureSettingsDialog.useFlash
                            || videoLayer.deviceType(videoLayer.videoInput) != VideoLayer.InputCamera) {
                            savePhoto()

                            return
                        }

                        if (captureSettingsDialog.delay == 0) {
                            if (captureSettingsDialog.useFlash)
                                flash.shot()
                            else
                                savePhoto()

                            return
                        }

                        if (updateProgress.running) {
                            updateProgress.stop()
                            pgbPhotoShot.value = 0
                        } else {
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
                width: mainLayout.smallButton
                height: mainLayout.smallButton
                x: parent.width - width
                y: (parent.height - height) / 2
                ToolTip.visible: hovered
                ToolTip.text: recording.state == AkElement.ElementStateNull?
                                  qsTr("Record video"):
                                  qsTr("Stop video recording")
                Accessible.name:
                    mainLayout.state == ""?
                        qsTr("Video capture mode"):
                    recording.state == AkElement.ElementStateNull?
                        qsTr("Record video"):
                        qsTr("Stop video recording")
                Accessible.description:
                    mainLayout.state == ""?
                        qsTr("Put %1 in video recording mode").arg(mediaTools.applicationName):
                    recording.state == AkElement.ElementStateNull?
                        qsTr("Start recording to a video file"):
                        qsTr("Stop current video recording")
                enabled: videoLayer.state == AkElement.ElementStatePlaying
                         || mainLayout.state == ""

                onClicked: {
                    if (mainLayout.state == "") {
                        mainLayout.state = "Video"
                    } else if (recording.state == AkElement.ElementStateNull) {
                        mediaTools.showAd(MediaTools.AdType_Interstitial);

                        if (captureSettingsDialog.useFlash
                            && flash.isHardwareFlash
                            && videoLayer.deviceType(videoLayer.videoInput) == VideoLayer.InputCamera) {
                            videoLayer.torchMode = VideoLayer.Torch_On
                        }

                        recording.state = AkElement.ElementStatePlaying
                    } else {
                        recording.state = AkElement.ElementStateNull
                        videoLayer.torchMode = VideoLayer.Torch_Off
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
                    if (videoPreview.status == Image.Ready) {
                        if (recording.latestVideoUri.length > 1) {
                            Qt.openUrlExternally(recording.latestVideoUri)
                        } else {
                            let url = recording.lastVideo

                            if (!url.startsWith(wdgMainWidget.filePrefix))
                                url = wdgMainWidget.filePrefix + url

                            Qt.openUrlExternally(url)
                        }
                    }
                }
            }
        }
        ProgressBar {
            id: pgbPhotoShot
            visible: updateProgress.running
            Layout.fillWidth: true

            property double start: 0

            onValueChanged: {
                if (value >= 1) {
                    updateProgress.stop()
                    value = 0

                    if (captureSettingsDialog.useFlash)
                        flash.shot()
                    else
                        savePhoto()
                }
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
                    width: mainLayout.smallButton
                    height: mainLayout.smallButton
                    x: 0
                }
                PropertyChanges {
                    target: videoButton
                    width: mainLayout.bigButton
                    height: mainLayout.bigButton
                    x: (parent.width - width) / 2
                }
                PropertyChanges {
                    target: videoPreview
                    width: mainLayout.previewSize
                    height: mainLayout.previewSize
                    visible: true
                }
                PropertyChanges {
                    target: localSettings
                    videoSettings: true
                }
                PropertyChanges {
                    target: captureSettingsDialog
                    videoSettings: true
                }
            }
        ]

        transitions: Transition {
            PropertyAnimation {
                target: photoPreview
                properties: "width,height,visible"
                duration: mainLayout.animationTime
            }
            PropertyAnimation {
                target: photoButton
                properties: "radius,x"
                duration: mainLayout.animationTime
            }
            PropertyAnimation {
                target: videoButton
                properties: "radius,x"
                duration: mainLayout.animationTime
            }
            PropertyAnimation {
                target: videoPreview
                properties: "width,height,visible"
                duration: mainLayout.animationTime
            }
        }
    }
    SettingsMenu {
        id: settings
        width: AkUnit.create(250 * AkTheme.controlScale, "dp").pixels

        onOpenAudioSettings: mainPanel.openAudioSettings()
        onOpenVideoSettings: mainPanel.openVideoSettings()
        onOpenVideoEffectsPanel: mainPanel.openVideoEffects()
        onOpenSettings: {
            mediaTools.showAd(MediaTools.AdType_Interstitial);
            settingsDialog.open();
        }
        onOpenDonationsDialog: Qt.openUrlExternally(mediaTools.projectDonationsUrl)
        onOpenAboutDialog: aboutDialog.open()
    }
    LocalSettingsMenu {
        id: localSettings
        width: AkUnit.create(250 * AkTheme.controlScale, "dp").pixels

        onCopyToClipboard: {
            mediaTools.showAd(MediaTools.AdType_Interstitial);
            snapshotToClipboard();
        }
        onOpenCaptureSettings: captureSettingsDialog.open()
        onOpenRecordingSettings: settingsDialog.openAtIndex(1)
    }
    MainPanel {
        id: mainPanel

        onOpenErrorDialog: (title, message) =>
            videoOutputError.openError(title, message)
        onOpenVideoEffectsDialog: {
            mediaTools.showAd(MediaTools.AdType_Interstitial);
            videoEffectsDialog.open()
        }
    }
    Rectangle {
        id: flashRectangle
        color: "white"
        anchors.fill: parent
        visible: false
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
            pgbPhotoShot.value = (new Date().getTime() - pgbPhotoShot.start)
                                 / captureSettingsDialog.delay
        }
    }
    Flash {
        id: flash

        onShotStarted: {
            if (isHardwareFlash)
                videoLayer.torchMode = VideoLayer.Torch_On
            else if (Ak.platform() == "android")
                flashRectangle.visible = true
        }
        onTriggered: savePhoto()
        onShotFinished: {
            if (isHardwareFlash)
                videoLayer.torchMode = VideoLayer.Torch_Off
            else if (Ak.platform() == "android")
                flashRectangle.visible = false
        }
    }
    RunCommandDialog {
        id: runCommandDialog
        title: qsTr("Installing virtual camera")
        message: qsTr("Running commands")
        anchors.centerIn: Overlay.overlay
    }
    CaptureSettingsDialog {
        id: captureSettingsDialog
        anchors.centerIn: Overlay.overlay
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
}
