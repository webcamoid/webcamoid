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
import Webcamoid 1.0

ApplicationWindow {
    id: wdgMainWidget
    title: mediaTools.applicationName
           + " "
           + mediaTools.applicationVersion
           + " - "
           + videoLayer.description(videoLayer.videoInput)
    visible: true
    x: (Screen.width - mediaTools.windowWidth) / 2
    y: (Screen.height - mediaTools.windowHeight) / 2
    width: mediaTools.windowWidth
    height: mediaTools.windowHeight

    function notifyUpdate(versionType)
    {
        if (updates.notifyNewVersion
            && versionType == Updates.VersionTypeOld) {
            trayIcon.show();
            trayIcon.showMessage(qsTr("New version available!"),
                                 qsTr("Download %1 %2 NOW!")
                                    .arg(mediaTools.applicationName())
                                    .arg(updates.latestVersion));
            notifyTimer.start();
        }
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

    function pathToUrl(path)
    {
        if (path.length < 1)
            return ""

        return "file://" + path
    }

    Timer {
        id: notifyTimer
        repeat: false
        triggeredOnStart: false
        interval: 10000

        onTriggered: trayIcon.hide()
    }

    onWidthChanged: mediaTools.windowWidth = width
    onHeightChanged: mediaTools.windowHeight = height
    onClosing: trayIcon.hide()

    Component.onCompleted: {
        notifyUpdate(updates.versionType);
        chkFlash.updateVisibility()
    }

    Connections {
        target: updates

        onVersionTypeChanged: notifyUpdate(versionType);
    }
    Connections {
        target: trayIcon

        onMessageClicked: Qt.openUrlExternally(mediaTools.projectDownloadsUrl())
    }
    Connections {
        target: videoLayer

        onVideoInputChanged: chkFlash.updateVisibility()
    }

    VideoDisplay {
        id: videoDisplay
        objectName: "videoDisplay"
        visible: videoLayer.state === AkElement.ElementStatePlaying
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
        width: AkUnit.create(150 * AkTheme.controlScale, "dp").pixels
        anchors.top: parent.top
        anchors.topMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        anchors.left: parent.left
        anchors.leftMargin: AkUnit.create(16 * AkTheme.controlScale, "dp").pixels
        state: cameraControls.state

        Button {
            icon.source: "image://icons/video-effects"
            display: AbstractButton.IconOnly
            flat: true

            onClicked: videoEffectsPanel.open()
        }
        Switch {
            id: chkFlash
            text: qsTr("Use flash")
            checked: true
            Layout.fillWidth: true
            visible: false

            function updateVisibility()
            {
                visible =
                        videoLayer.deviceType(videoLayer.videoInput) == VideoLayer.InputCamera
            }
        }
        ComboBox {
            id: cbxTimeShot
            textRole: "text"
            Layout.fillWidth: true
            visible: chkFlash.visible
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

        states: [
            State {
                name: "Video"

                PropertyChanges {
                    target: chkFlash
                    visible: false
                }
            }
        ]

        transitions: Transition {
            PropertyAnimation {
                target: chkFlash
                properties: "visible"
                duration: cameraControls.animationTime
            }
        }
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

        onClicked: settings.popup()
    }
    SettingsMenu {
        id: settings
        width: AkUnit.create(250 * AkTheme.controlScale, "dp").pixels

        onOpenAudioSettings: audioVideoPanel.openAudioSettings()
        onOpenVideoSettings: audioVideoPanel.openVideoSettings()
        onOpenSettings: settingsDialog.open()
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

            readonly property int animationTime: 200

            Image {
                id: photoPreview
                source: pathToUrl(recording.lastPhotoPreview)
                width: AkUnit.create(32 * AkTheme.controlScale, "dp").pixels
                height: AkUnit.create(32 * AkTheme.controlScale, "dp").pixels
                sourceSize: Qt.size(width, height)
                asynchronous: true
                cache: false
                smooth: true
                mipmap: true
                fillMode: Image.PreserveAspectCrop
                y: (parent.height - height) / 2

                MouseArea {
                    cursorShape: enabled?
                                     Qt.PointingHandCursor:
                                     Qt.ArrowCursor
                    anchors.fill: parent
                    enabled: photoPreview.visible
                             && photoPreview.status == Image.Ready

                    onClicked: {
                        if (photoPreview.status == Image.Ready)
                            Qt.openUrlExternally(photoPreview.source)
                    }
                }
            }
            RoundButton {
                id: photoButton
                icon.source: "image://icons/photo"
                width: AkUnit.create(64 * AkTheme.controlScale, "dp").pixels
                height: AkUnit.create(64 * AkTheme.controlScale, "dp").pixels
                x: (parent.width - width) / 2
                y: (parent.height - height) / 2
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Take a photo")
                focus: true
                enabled: recording.state == AkElement.ElementStateNull
                         && (videoLayer.state === AkElement.ElementStatePlaying
                             || cameraControls.state == "Video")

                onClicked: {
                    if (cameraControls.state == "Video") {
                        cameraControls.state = ""
                    } else {
                        if (!chkFlash.visible) {
                            savePhoto()

                            return
                        }

                        if (cbxTimeShot.currentIndex == 0) {
                            if (chkFlash.checked)
                                flash.show()
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
                width: AkUnit.create(48 * AkTheme.controlScale, "dp").pixels
                height: AkUnit.create(48 * AkTheme.controlScale, "dp").pixels
                x: parent.width - width
                y: (parent.height - height) / 2
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Record video")
                enabled: videoLayer.state === AkElement.ElementStatePlaying
                         || cameraControls.state == ""

                onClicked: {
                    if (cameraControls.state == "") {
                        cameraControls.state = "Video"
                    } else if (recording.state == AkElement.ElementStateNull) {
                        recording.state = AkElement.ElementStatePlaying
                    } else {
                        recording.state = AkElement.ElementStateNull
                        videoPreviewSaveAnimation.start()
                    }
                }
            }
            Image {
                id: videoPreview
                source: pathToUrl(recording.lastVideoPreview)
                width: 0
                height: 0
                sourceSize: Qt.size(width, height)
                asynchronous: true
                cache: false
                smooth: true
                mipmap: true
                fillMode: Image.PreserveAspectCrop
                visible: false
                x: parent.width - width
                y: (parent.height - height) / 2

                MouseArea {
                    cursorShape: enabled?
                                     Qt.PointingHandCursor:
                                     Qt.ArrowCursor
                    anchors.fill: parent
                    enabled: videoPreview.visible
                             && videoPreview.status == Image.Ready

                    onClicked: {
                        if (videoPreview.status == Image.Ready)
                            Qt.openUrlExternally("file://" + recording.lastVideo)
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
                        width: AkUnit.create(48 * AkTheme.controlScale, "dp").pixels
                        height: AkUnit.create(48 * AkTheme.controlScale, "dp").pixels
                        x: 0
                    }
                    PropertyChanges {
                        target: videoButton
                        width: AkUnit.create(64 * AkTheme.controlScale, "dp").pixels
                        height: AkUnit.create(64 * AkTheme.controlScale, "dp").pixels
                        x: (parent.width - width) / 2
                    }
                    PropertyChanges {
                        target: videoPreview
                        width: AkUnit.create(32 * AkTheme.controlScale, "dp").pixels
                        height: AkUnit.create(32 * AkTheme.controlScale, "dp").pixels
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
                        flash.show()
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

        onTriggered: savePhoto()
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

        onOpenVideoFormatDialog: videoFormatOptions.open()
        onOpenVideoCodecDialog: videoCodecOptions.open()
        onOpenAudioCodecDialog: audioCodecOptions.open()
    }
    VideoFormatOptions {
        id: videoFormatOptions
        width: parent.width
        height: parent.height
    }
    VideoCodecOptions {
        id: videoCodecOptions
        width: parent.width
        height: parent.height
    }
    AudioCodecOptions {
        id: audioCodecOptions
        width: parent.width
        height: parent.height
    }
    LABS.Settings {
        category: "GeneralConfigs"

        property alias useFlash: chkFlash.checked
        property alias photoTimeout: cbxTimeShot.currentIndex
    }
}
